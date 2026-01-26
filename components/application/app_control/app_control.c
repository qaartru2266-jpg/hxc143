#include "app_control.h"

#include "esp_log.h"
#include "app_axis6.h"
#include "app_datalog.h"
#include "app_gps.h"

#define TAG "app_control"
static bool s_all_stopped = false;

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
