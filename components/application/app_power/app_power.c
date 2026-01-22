#include "app_power.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"

#include "app_vibration.h"
#include "app_gui.h"
#include "sdkconfig.h"

#define KEY_GPIO            ((gpio_num_t)CONFIG_JOFTMODE_POWER_KEY_GPIO)
#define TOUCH_INT_GPIO      ((gpio_num_t)11)  // keep in sync with app_touch.cpp
#define SCAN_INTERVAL_MS    20
#define LONG_PRESS_MS       2000

static const char *TAG = "app_power";

#ifndef APP_POWER_DEBUG
#define APP_POWER_DEBUG 1
#endif

#if APP_POWER_DEBUG
#define POWER_LOGD(...) ESP_LOGI(TAG, __VA_ARGS__)
#else
#define POWER_LOGD(...)
#endif

static TaskHandle_t s_key_task = NULL;
static TaskHandle_t s_sleep_task = NULL;
static bool s_power_on = true;  // ��ʼΪ����״̬
static volatile bool s_low_power_mode = false;
static volatile bool s_block_key_until_release = false;

static void apply_power_state(bool on)
{
    s_power_on = on;
    s_low_power_mode = !on;
    POWER_LOGD("low power %s", s_low_power_mode ? "on" : "off");
    if (on) {
        s_block_key_until_release = true;
        app_vibration_stop();        // �������ܴ��ڵ�����
        app_vibration_pulse_ms(120); // ������ʾ
        app_gui_screen_on();
    } else {
        app_vibration_stop();
        app_vibration_pulse_ms(80);  // �ػ���ʾ
        app_gui_screen_off();
    }
}

static inline bool power_key_is_pressed(void)
{
    return gpio_get_level(KEY_GPIO) == 0;
}

static void enable_wakeup_sources(void)
{
    esp_err_t err = gpio_wakeup_enable(KEY_GPIO, GPIO_INTR_LOW_LEVEL);
    if (err != ESP_OK) {
        POWER_LOGD("key wakeup enable failed: %s", esp_err_to_name(err));
    }

    err = gpio_wakeup_enable(TOUCH_INT_GPIO, GPIO_INTR_LOW_LEVEL);
    if (err != ESP_OK) {
        POWER_LOGD("touch wakeup enable failed: %s", esp_err_to_name(err));
    }

    err = esp_sleep_enable_gpio_wakeup();
    if (err != ESP_OK) {
        POWER_LOGD("sleep gpio wakeup failed: %s", esp_err_to_name(err));
    }
}

static void awake_work(void)
{
    POWER_LOGD("awake");
}

static void prepare_for_sleep(void)
{
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    enable_wakeup_sources();
#if defined(APP_POWER_SD_CS_GPIO)
    gpio_set_level((gpio_num_t)APP_POWER_SD_CS_GPIO, 1);
    POWER_LOGD("sd cs high");
#else
    POWER_LOGD("sd cs gpio not defined, skip");
#endif
}

// Minimal sleep entry/exit loop; wake by key or touch.
static void power_task(void *arg)
{
    (void)arg;
    for (;;) {
        if (!s_low_power_mode) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        awake_work();

        if (power_key_is_pressed()) {
            TickType_t start = xTaskGetTickCount();
            TickType_t timeout = pdMS_TO_TICKS(LONG_PRESS_MS + 100);
            while (s_low_power_mode && power_key_is_pressed() &&
                   (xTaskGetTickCount() - start) < timeout) {
                vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_MS));
            }
            if (!s_low_power_mode) {
                continue;
            }
            if (power_key_is_pressed()) {
                continue;
            }
        }

        prepare_for_sleep();

        esp_err_t err = esp_light_sleep_start();
        if (err != ESP_OK) {
            POWER_LOGD("light sleep failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
        POWER_LOGD("wakeup: %s", (cause == ESP_SLEEP_WAKEUP_GPIO) ? "gpio" : "other");
        apply_power_state(true);
    }
}

static void key_task(void *arg)
{
    bool pressed_prev = false;
    uint32_t pressed_ms = 0;
    bool long_triggered = false;  // ����һ�γ�������л�

    for (;;) {
        int level = gpio_get_level(KEY_GPIO);
        bool pressed = (level == 0);  // �͵�ƽ��ʾ����
        if (s_block_key_until_release) {
            if (!pressed) {
                s_block_key_until_release = false;
                pressed_prev = false;
                pressed_ms = 0;
                long_triggered = false;
            } else {
                pressed_prev = true;
            }
            vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_MS));
            continue;
        }


        if (pressed) {
            if (pressed_prev) {
                pressed_ms += SCAN_INTERVAL_MS;
            } else {
                pressed_ms = SCAN_INTERVAL_MS;
            }

            if (!long_triggered && pressed_ms >= LONG_PRESS_MS) {
                apply_power_state(!s_power_on);  // �ﵽ��ֵ�����л�
                long_triggered = true;
            }
        } else {
            // �ɿ���λ�����봥�����
            pressed_ms = 0;
            long_triggered = false;
        }

        pressed_prev = pressed;
        vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_MS));
    }
}

esp_err_t app_power_start(void)
{
    if (s_key_task || s_sleep_task) return ESP_OK;

    gpio_config_t io = {
        .pin_bit_mask = 1ULL << KEY_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,   // �ⲿ 10k ����
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&io);
    if (ret != ESP_OK) return ret;

    apply_power_state(true);  // ���ֿ���״̬

    BaseType_t ok_key = xTaskCreate(key_task, "power_key", 2048, NULL, 5, &s_key_task);
    BaseType_t ok_sleep = xTaskCreatePinnedToCore(
        power_task, "power_task", 4096, NULL, 4, &s_sleep_task, 0);
    return (ok_key == pdPASS && ok_sleep == pdPASS) ? ESP_OK : ESP_FAIL;
}
