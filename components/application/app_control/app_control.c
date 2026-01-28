#include "app_control.h"

#include <stdarg.h>

#include "esp_log.h"
#include "app_axis6.h"
#include "app_datalog.h"
#include "app_gps.h"

#define TAG "app_control"
static bool s_all_stopped = false;
static bool s_quiet_mode = false;
static vprintf_like_t s_log_vprintf = NULL;

static int app_control_quiet_vprintf(const char *fmt, va_list ap)
{
    (void)fmt;
    (void)ap;
    return 0;
}

void app_control_stop_all(void)
{
    app_axis6_stop();
    app_gps_stop();
    app_datalog_stop();
    s_all_stopped = true;
    ESP_LOGW(TAG, "stop all");
}

void app_control_resume_all(void)
{
    app_axis6_resume();
    app_gps_resume();
    app_datalog_resume();
    s_all_stopped = false;
    ESP_LOGW(TAG, "resume all");
}

void app_control_stop_imu(void)
{
    app_axis6_stop();
}

void app_control_resume_imu(void)
{
    app_axis6_resume();
}

void app_control_stop_gps(void)
{
    app_gps_stop();
}

void app_control_resume_gps(void)
{
    app_gps_resume();
}

void app_control_stop_datalog(void)
{
    app_datalog_stop();
}

void app_control_resume_datalog(void)
{
    app_datalog_resume();
}

bool app_control_is_stopped(void)
{
    return s_all_stopped;
}

void app_control_set_quiet(bool enable)
{
    if (s_quiet_mode == enable) {
        return;
    }
    s_quiet_mode = enable;
    if (enable) {
        if (!s_log_vprintf) {
            s_log_vprintf = esp_log_set_vprintf(app_control_quiet_vprintf);
        }
        esp_log_level_set("*", ESP_LOG_NONE);
    } else {
        esp_log_level_set("*", ESP_LOG_INFO);
        if (s_log_vprintf) {
            esp_log_set_vprintf(s_log_vprintf);
            s_log_vprintf = NULL;
        }
    }
}

bool app_control_is_quiet(void)
{
    return s_quiet_mode;
}
