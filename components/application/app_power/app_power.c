#include "app_power.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "app_vibration.h"
#include "app_gui.h"
#include "sdkconfig.h"

#define KEY_GPIO            ((gpio_num_t)CONFIG_JOFTMODE_POWER_KEY_GPIO)
#define TOUCH_INT_GPIO      ((gpio_num_t)11)  // keep in sync with app_touch.cpp
#define SCAN_INTERVAL_MS    20
#define LONG_PRESS_MS       2000
#define LONG_PRESS_RESET_MS 5000
#define SHORT_PRESS_MIN_MS  60
#define INACTIVITY_TIMEOUT_MS 30000
#define TOUCH_WAKE_HOLD_MS 120

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
static volatile TickType_t s_last_touch_tick = 0;
static volatile bool s_ignore_touch_until_release = false;
static int64_t s_touch_press_start_us = 0;

static void apply_power_state(bool on)
{
    s_power_on = on;
    s_low_power_mode = !on;
    POWER_LOGD("low power %s", s_low_power_mode ? "on" : "off");
    if (on) {
        s_last_touch_tick = xTaskGetTickCount();
        s_block_key_until_release = true;
        app_vibration_stop();        // �������ܴ��ڵ����� // ������ʾ
        app_gui_screen_on();
    } else {
        s_ignore_touch_until_release = false;
        app_vibration_stop();  // �ػ���ʾ
        app_gui_screen_off();
    }
}

static inline bool power_key_is_pressed(void)
{
    return gpio_get_level(KEY_GPIO) == 0;
}

bool app_power_on_touch(bool pressed)
{
    if (pressed) {
        s_last_touch_tick = xTaskGetTickCount();
        if (s_low_power_mode) {
            if (s_touch_press_start_us == 0) {
                s_touch_press_start_us = esp_timer_get_time();
            }
            int64_t held_ms = (esp_timer_get_time() - s_touch_press_start_us) / 1000;
            if (!s_ignore_touch_until_release && held_ms >= TOUCH_WAKE_HOLD_MS) {
                apply_power_state(true);
                s_ignore_touch_until_release = true;
            }
            return true;
        }
        if (s_ignore_touch_until_release) {
            return true;
        }
        return false;
    }

    if (s_ignore_touch_until_release) {
        s_ignore_touch_until_release = false;
    }
    s_touch_press_start_us = 0;
    return false;
}

static void power_task(void *arg)
{
    (void)arg;
    for (;;) {
        if (!s_low_power_mode) {
            TickType_t now = xTaskGetTickCount();
            if (s_power_on && (now - s_last_touch_tick) >= pdMS_TO_TICKS(INACTIVITY_TIMEOUT_MS)) {
                apply_power_state(false);
            }
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        // Low power mode: screen off only, no light sleep.
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

static void key_task(void *arg)
{
    bool pressed_prev = false;
    uint32_t pressed_ms = 0;
    bool reset_sent = false;

    for (;;) {
        bool pressed = power_key_is_pressed();
        if (s_block_key_until_release) {
            if (!pressed) {
                s_block_key_until_release = false;
                pressed_prev = false;
                pressed_ms = 0;
                reset_sent = false;
            } else {
                pressed_prev = true;
            }
            vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_MS));
            continue;
        }

        if (pressed) {
            if (!pressed_prev && s_low_power_mode) {
                apply_power_state(true);
                vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_MS));
                continue;
            }

            if (pressed_prev) {
                pressed_ms += SCAN_INTERVAL_MS;
            } else {
                pressed_ms = SCAN_INTERVAL_MS;
                reset_sent = false;
            }

            if (!reset_sent && pressed_ms >= LONG_PRESS_RESET_MS) {
                reset_sent = true;
                POWER_LOGD("power key long press reset");
                vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_MS));
                esp_restart();
            }
        } else {
            if (pressed_prev) {
                if (!reset_sent && pressed_ms >= SHORT_PRESS_MIN_MS && pressed_ms < LONG_PRESS_MS) {
                    if (s_power_on) {
                        apply_power_state(false);
                    }
                }
            }
            pressed_ms = 0;
            reset_sent = false;
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
        .pull_up_en = GPIO_PULLUP_ENABLE,   // �ⲿ 10k ����
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&io);
    if (ret != ESP_OK) return ret;

    gpio_config_t touch_io = {
        .pin_bit_mask = 1ULL << TOUCH_INT_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&touch_io);
    if (ret != ESP_OK) return ret;

    apply_power_state(true);  // ���ֿ���״̬

    BaseType_t ok_key = xTaskCreate(key_task, "power_key", 4096, NULL, 5, &s_key_task);
    BaseType_t ok_sleep = xTaskCreatePinnedToCore(
        power_task, "power_task", 4096, NULL, 4, &s_sleep_task, 0);
    return (ok_key == pdPASS && ok_sleep == pdPASS) ? ESP_OK : ESP_FAIL;
}
