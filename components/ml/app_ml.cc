#include "app_ml.h"
#include "ml_runner.h"
#include "esp_log.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

static const char* TAG = "app_ml";

static app_ml_status_t s_status;
static bool s_status_valid = false;
static portMUX_TYPE s_status_lock = portMUX_INITIALIZER_UNLOCKED;

static ml_result_t s_last_result;
static bool s_last_result_valid = false;

void app_ml_update_status(const app_ml_status_t *st)
{
    if (!st) {
        return;
    }
    portENTER_CRITICAL(&s_status_lock);
    s_status = *st;
    s_status_valid = true;
    portEXIT_CRITICAL(&s_status_lock);
}

void app_ml_update_result(const ml_result_t *res)
{
    if (!res) {
        return;
    }
    s_last_result = *res;
    s_last_result_valid = true;
}

bool app_ml_get_latest_status(app_ml_status_t *out)
{
    if (!out) {
        return false;
    }
    bool ok = false;
    portENTER_CRITICAL(&s_status_lock);
    if (s_status_valid) {
        *out = s_status;
        ok = true;
    }
    portEXIT_CRITICAL(&s_status_lock);
    return ok;
}

extern "C" bool ml_get_latest_result(ml_result_t *out)
{
    if (!out) {
        return false;
    }
    if (!s_last_result_valid) {
        return false;
    }
    *out = s_last_result;
    return true;
}

extern "C" __attribute__((weak)) void app_ml_task_start(void)
{
}

bool app_ml_init(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "app_ml_init enter");

    if (!ml_init()) {
        ESP_LOGE(TAG, "ml_init FAIL");
        printf("app_ml: ml_init FAIL\n");
        return false;
    }

    app_ml_task_start();

    ESP_LOGI(TAG, "ml_init OK");
    printf("app_ml: ml_init OK\n");
    return true;
}
