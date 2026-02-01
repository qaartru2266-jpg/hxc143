#include "app_force.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include "app_datalog.h"
#include "app_gps.h"
#include "app_ml.h"

static const char *TAG = "app_force";

static portMUX_TYPE s_force_lock = portMUX_INITIALIZER_UNLOCKED;
static bool s_force_active = false;
static int s_force_mode = -1;
static int64_t s_force_deadline_ms = 0;
static uint32_t s_force_version = 0;

static void force_log_enter(int mode, const char *reason, uint32_t duration_sec)
{
    ESP_LOGI(TAG, "FORCE enter: mode=%d reason=%s duration=%us",
             mode, reason ? reason : "ui", (unsigned int)duration_sec);
}

static void force_log_exit(const char *reason)
{
    ESP_LOGI(TAG, "FORCE exit: reason=%s", reason ? reason : "cancel");
}

static void force_apply_active_state(bool active)
{
    if (active) {
        app_ml_set_infer_enabled(false);
        app_datalog_set_pred_logging_enabled(false);
        app_datalog_set_raw_logging_enabled(false);
        app_gps_set_force_active(true);
    } else {
        app_ml_set_infer_enabled(true);
        app_datalog_set_pred_logging_enabled(true);
        app_datalog_set_raw_logging_enabled(true);
        app_gps_set_force_active(false);
    }
}

void app_force_enter(int mode, uint32_t duration_sec, const char *reason)
{
    if (mode < 0 || mode > 5) {
        return;
    }
    if (duration_sec == 0) {
        duration_sec = APP_FORCE_DEFAULT_DURATION_SEC;
    }

    int64_t now_ms = esp_timer_get_time() / 1000;
    portENTER_CRITICAL(&s_force_lock);
    s_force_active = true;
    s_force_mode = mode;
    s_force_deadline_ms = now_ms + (int64_t)duration_sec * 1000;
    s_force_version++;
    portEXIT_CRITICAL(&s_force_lock);

    force_apply_active_state(true);
    force_log_enter(mode, reason, duration_sec);
}

void app_force_cancel(const char *reason)
{
    bool was_active = false;
    portENTER_CRITICAL(&s_force_lock);
    was_active = s_force_active;
    if (s_force_active) {
        s_force_active = false;
        s_force_deadline_ms = 0;
        s_force_version++;
    }
    portEXIT_CRITICAL(&s_force_lock);

    if (!was_active) {
        return;
    }

    force_apply_active_state(false);
    force_log_exit(reason);
}

void app_force_poll(void)
{
    int64_t deadline_ms = 0;
    portENTER_CRITICAL(&s_force_lock);
    if (s_force_active) {
        deadline_ms = s_force_deadline_ms;
    }
    portEXIT_CRITICAL(&s_force_lock);

    if (deadline_ms == 0) {
        return;
    }

    int64_t now_ms = esp_timer_get_time() / 1000;
    if (now_ms >= deadline_ms) {
        app_force_cancel("timeout");
    }
}

bool app_force_is_active(void)
{
    bool active = false;
    portENTER_CRITICAL(&s_force_lock);
    active = s_force_active;
    portEXIT_CRITICAL(&s_force_lock);
    return active;
}

int app_force_get_mode(void)
{
    int mode = -1;
    portENTER_CRITICAL(&s_force_lock);
    if (s_force_active) {
        mode = s_force_mode;
    }
    portEXIT_CRITICAL(&s_force_lock);
    return mode;
}

int app_force_get_current_mode(int pred_mode)
{
    int mode = pred_mode;
    portENTER_CRITICAL(&s_force_lock);
    if (s_force_active) {
        mode = s_force_mode;
    }
    portEXIT_CRITICAL(&s_force_lock);
    return mode;
}

uint32_t app_force_get_version(void)
{
    uint32_t version;
    portENTER_CRITICAL(&s_force_lock);
    version = s_force_version;
    portEXIT_CRITICAL(&s_force_lock);
    return version;
}
