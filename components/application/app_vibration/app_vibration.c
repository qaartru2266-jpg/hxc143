#include "app_vibration.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#define VIB_GPIO          GPIO_NUM_3
#define VIB_ACTIVE_LEVEL  1
#define VIB_MAX_RUN_MS    2000

static TaskHandle_t vib_task = NULL;
static vib_pattern_t cur_pat = {200, 800};
static volatile bool s_stop_req = false;

static void vib_on(void)  { gpio_set_level(VIB_GPIO, VIB_ACTIVE_LEVEL); }
static void vib_off(void) { gpio_set_level(VIB_GPIO, !VIB_ACTIVE_LEVEL); }

static bool vib_task_running(void) { return vib_task != NULL; }

static void vib_task_fn(void *arg)
{
    (void)arg;
    int64_t start_us = esp_timer_get_time();
    for (;;) {
        if (s_stop_req) {
            break;
        }
        if (VIB_MAX_RUN_MS > 0) {
            int64_t elapsed_ms = (esp_timer_get_time() - start_us) / 1000;
            if (elapsed_ms >= VIB_MAX_RUN_MS) {
                break;
            }
        }
        vib_on();
        vTaskDelay(pdMS_TO_TICKS(cur_pat.on_ms));
        vib_off();
        vTaskDelay(pdMS_TO_TICKS(cur_pat.off_ms));
    }
    vib_off();
    vib_task = NULL;
    s_stop_req = false;
    vTaskDelete(NULL);
}

esp_err_t app_vibration_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << VIB_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&io);
    if (ret != ESP_OK) return ret;
    vib_off();
    return ESP_OK;
}

esp_err_t app_vibration_start(vib_pattern_t pat)
{
    cur_pat = pat;
    if (vib_task) return ESP_OK;

    s_stop_req = false;
    BaseType_t ok = xTaskCreate(vib_task_fn, "vib_task", 2048, NULL, 5, &vib_task);
    return ok == pdPASS ? ESP_OK : ESP_FAIL;
}

void app_vibration_stop(void)
{
    if (vib_task) {
        s_stop_req = true;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vib_off();
}

void app_vibration_pulse_ms(uint32_t on_ms)
{
    if (vib_task_running()) return;
    vib_on();
    vTaskDelay(pdMS_TO_TICKS(on_ms));
    vib_off();
}
