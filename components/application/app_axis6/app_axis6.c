#include "stdio.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "axis6_interface.h"
#include "app_axis6.h"
#include "app_state.h"
#include "app_imu_calib.h"
#include "app_control.h"
#include "esp_timer.h"

static const char* TAG = "axis6";
static int warmup = 5;   // 跳过前5帧（约200ms），按需调，目前测试下来7帧是最好的
#define AXIS6_IMU_LOG 0  // set to 1 to enable IMU logs

t_sQMI8658 qmi8658_info;
static TaskHandle_t s_axis6_task = NULL;
static volatile bool s_axis6_enabled = true;

static void axis6_task(void* arg)
{
    i2c_master_init();
    qmi8658_init();

    TickType_t last_wake = xTaskGetTickCount();
#if !AXIS6_IMU_LOG
    int hb = 0;
#endif

    ESP_LOGW(TAG, "axis6 task started");

    while (1) {
        if (!s_axis6_enabled) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        // 阻塞读取IMU
        qmi8658_Read_AccAndGry(&qmi8658_info);

        int16_t acc_x = qmi8658_info.acc_x;
        int16_t acc_y = qmi8658_info.acc_y;
        int16_t acc_z = qmi8658_info.acc_z;
        int16_t gyr_x = qmi8658_info.gyr_x;
        int16_t gyr_y = qmi8658_info.gyr_y;
        int16_t gyr_z = qmi8658_info.gyr_z;

        app_imu_calib_apply(&acc_x, &acc_y, &acc_z, &gyr_x, &gyr_y, &gyr_z);

        if (warmup > 0) {
            warmup--;
        } else {
            app_state_imu_sample_t sample = {
                .acc_x = acc_x,
                .acc_y = acc_y,
                .acc_z = acc_z,
                .gyr_x = gyr_x,
                .gyr_y = gyr_y,
                .gyr_z = gyr_z,
                .timestamp_us = esp_timer_get_time()
            };
            app_state_set_imu_sample(&sample);
        }

#if AXIS6_IMU_LOG
        ESP_LOGI(TAG, "imu acc=(%d,%d,%d) gyr=(%d,%d,%d)",
            acc_x, acc_y, acc_z,
            gyr_x, gyr_y, gyr_z);
#else
        if (++hb >= 400) {
            hb = 0;
            UBaseType_t free_words = uxTaskGetStackHighWaterMark(NULL);
            if (!app_control_is_quiet()) {
                ESP_LOGW(TAG, "tick acc=(%d,%d,%d) gyr=(%d,%d,%d) stack_free=%u words",
                    acc_x, acc_y, acc_z,
                    gyr_x, gyr_y, gyr_z,
                    (unsigned)free_words);
            }
        }
#endif

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(20)); // 50Hz
    }
}

void app_axis6_start(void)
{
    if (s_axis6_task) {
        return;
    }
    s_axis6_enabled = true;
    xTaskCreate(axis6_task, "axis6", 8192, NULL, 10, &s_axis6_task);
}

void app_axis6_stop(void)
{
    s_axis6_enabled = false;
    ESP_LOGW(TAG, "axis6 stop requested");
}

void app_axis6_resume(void)
{
    s_axis6_enabled = true;
    ESP_LOGW(TAG, "axis6 resume");
}

bool app_axis6_is_running(void)
{
    return s_axis6_enabled;
}
