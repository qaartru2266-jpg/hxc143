#include "app_imu_calib.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <time.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "axis6_interface.h"
#include "app_control.h"
#include "app_sdcard.h"

#define TAG "app_imu_calib"

#define IMU_CALIB_NVS_NAMESPACE "imu_calib"
#define IMU_CALIB_KEY_ACC "acc_bias"
#define IMU_CALIB_KEY_GYRO "gyro_bias"

#define CALIB_SAMPLE_RATE_HZ 25
#define CALIB_SAMPLE_PERIOD_MS (1000 / CALIB_SAMPLE_RATE_HZ)
#define CALIB_GYRO_NORM_STD_THRESHOLD 20.0f

#define CALIB_FILE "/sdcard/imu_calib.csv"

static float s_acc_bias[3] = {0.0f, 0.0f, 0.0f};
static float s_gyro_bias[3] = {0.0f, 0.0f, 0.0f};
static bool s_bias_loaded = false;
static const char *imu_calib_skip_ws(const char *s);

static void imu_calib_set_bias_zero(void)
{
    memset(s_acc_bias, 0, sizeof(s_acc_bias));
    memset(s_gyro_bias, 0, sizeof(s_gyro_bias));
}

static const char *imu_calib_skip_ws(const char *s)
{
    if (!s) {
        return "";
    }
    while (*s == ' ' || *s == '\t') {
        ++s;
    }
    return s;
}


static esp_err_t imu_calib_load_bias(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        (void)nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        imu_calib_set_bias_zero();
        s_bias_loaded = true;
        return err;
    }

    nvs_handle_t handle;
    err = nvs_open(IMU_CALIB_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        imu_calib_set_bias_zero();
        s_bias_loaded = true;
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            return ESP_OK;
        }
        return err;
    }

    size_t len = sizeof(s_acc_bias);
    err = nvs_get_blob(handle, IMU_CALIB_KEY_ACC, s_acc_bias, &len);
    if (err != ESP_OK || len != sizeof(s_acc_bias)) {
        imu_calib_set_bias_zero();
    }

    len = sizeof(s_gyro_bias);
    err = nvs_get_blob(handle, IMU_CALIB_KEY_GYRO, s_gyro_bias, &len);
    if (err != ESP_OK || len != sizeof(s_gyro_bias)) {
        memset(s_gyro_bias, 0, sizeof(s_gyro_bias));
    }

    nvs_close(handle);
    s_bias_loaded = true;
    return ESP_OK;
}

static esp_err_t imu_calib_store_bias(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(IMU_CALIB_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_blob(handle, IMU_CALIB_KEY_ACC, s_acc_bias, sizeof(s_acc_bias));
    if (err == ESP_OK) {
        err = nvs_set_blob(handle, IMU_CALIB_KEY_GYRO, s_gyro_bias, sizeof(s_gyro_bias));
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

static void imu_calib_log_bias(const char *prefix)
{
    ESP_LOGI(TAG, "%s acc_bias=(%.2f, %.2f, %.2f) gyro_bias=(%.2f, %.2f, %.2f)",
             prefix,
             s_acc_bias[0], s_acc_bias[1], s_acc_bias[2],
             s_gyro_bias[0], s_gyro_bias[1], s_gyro_bias[2]);
}

void app_imu_calib_init(void)
{
    if (s_bias_loaded) {
        return;
    }
    esp_err_t err = imu_calib_load_bias();
    if (err == ESP_OK) {
        imu_calib_log_bias("bias loaded");
    } else {
        ESP_LOGW(TAG, "bias load failed, use zeros");
    }
}

void app_imu_calib_start_cmd(void)
{
    // Console moved to app_quiet.
}

void app_imu_calib_apply(int16_t *ax, int16_t *ay, int16_t *az,
                         int16_t *gx, int16_t *gy, int16_t *gz)
{
    if (!s_bias_loaded) {
        app_imu_calib_init();
    }

    if (ax) {
        *ax = (int16_t)lrintf((float)(*ax) - s_acc_bias[0]);
    }
    if (ay) {
        *ay = (int16_t)lrintf((float)(*ay) - s_acc_bias[1]);
    }
    if (az) {
        *az = (int16_t)lrintf((float)(*az) - s_acc_bias[2]);
    }
    if (gx) {
        *gx = (int16_t)lrintf((float)(*gx) - s_gyro_bias[0]);
    }
    if (gy) {
        *gy = (int16_t)lrintf((float)(*gy) - s_gyro_bias[1]);
    }
    if (gz) {
        *gz = (int16_t)lrintf((float)(*gz) - s_gyro_bias[2]);
    }
}

static void imu_calib_write_csv(const char *status)
{
    struct stat st;
    bool need_header = (stat(CALIB_FILE, &st) != 0 || st.st_size == 0);

    if (!app_sdcard_lock_fs(portMAX_DELAY)) {
        ESP_LOGW(TAG, "sd fs lock failed, skip calib csv");
        return;
    }

    FILE *f = fopen(CALIB_FILE, "a");
    if (!f) {
        ESP_LOGW(TAG, "open calib file failed errno=%d", errno);
        app_sdcard_unlock_fs();
        return;
    }

    if (need_header) {
        fprintf(f, "timestamp,acc_bias_x,acc_bias_y,acc_bias_z,gyro_bias_x,gyro_bias_y,gyro_bias_z,status\r\n");
    }

    time_t now = time(NULL);
    if (now <= 0) {
        now = (time_t)(esp_timer_get_time() / 1000000);
    }
    fprintf(f, "%lld,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%s\r\n",
            (long long)now,
            s_acc_bias[0], s_acc_bias[1], s_acc_bias[2],
            s_gyro_bias[0], s_gyro_bias[1], s_gyro_bias[2],
            status ? status : "UNKNOWN");
    fflush(f);
    (void)fsync(fileno(f));
    fclose(f);
    app_sdcard_unlock_fs();
}

esp_err_t app_imu_calib_run(int seconds)
{
    if (seconds <= 0) {
        ESP_LOGW(TAG, "invalid seconds: %d", seconds);
        return ESP_ERR_INVALID_ARG;
    }

    app_control_stop_all();
    vTaskDelay(pdMS_TO_TICKS(200));

    i2c_master_init();
    qmi8658_init();

    int total_samples = seconds * CALIB_SAMPLE_RATE_HZ;
    double acc_sum[3] = {0.0, 0.0, 0.0};
    double gyro_sum[3] = {0.0, 0.0, 0.0};
    double gyro_norm_sum = 0.0;
    double gyro_norm_sq_sum = 0.0;

    TickType_t last_wake = xTaskGetTickCount();
    for (int i = 0; i < total_samples; ++i) {
        t_sQMI8658 sample = {0};
        qmi8658_Read_AccAndGry(&sample);

        acc_sum[0] += sample.acc_x;
        acc_sum[1] += sample.acc_y;
        acc_sum[2] += sample.acc_z;
        gyro_sum[0] += sample.gyr_x;
        gyro_sum[1] += sample.gyr_y;
        gyro_sum[2] += sample.gyr_z;

        float gx = (float)sample.gyr_x;
        float gy = (float)sample.gyr_y;
        float gz = (float)sample.gyr_z;
        double gyro_norm = sqrt((double)gx * gx + (double)gy * gy + (double)gz * gz);
        gyro_norm_sum += gyro_norm;
        gyro_norm_sq_sum += gyro_norm * gyro_norm;

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(CALIB_SAMPLE_PERIOD_MS));
    }

    double mean_gyro_norm = gyro_norm_sum / (double)total_samples;
    double var_gyro_norm = (gyro_norm_sq_sum / (double)total_samples) - (mean_gyro_norm * mean_gyro_norm);
    if (var_gyro_norm < 0.0) {
        var_gyro_norm = 0.0;
    }
    float std_gyro_norm = (float)sqrt(var_gyro_norm);
    if (std_gyro_norm >= CALIB_GYRO_NORM_STD_THRESHOLD) {
        ESP_LOGW(TAG, "calib failed: device moving (gyro_norm_std=%.2f, mean=%.2f, th=%.2f)",
                 std_gyro_norm, (float)mean_gyro_norm, CALIB_GYRO_NORM_STD_THRESHOLD);
        imu_calib_write_csv("FAIL_MOVE");
        app_control_resume_all();
        return ESP_FAIL;
    }

    s_acc_bias[0] = (float)(acc_sum[0] / (double)total_samples);
    s_acc_bias[1] = (float)(acc_sum[1] / (double)total_samples);
    s_acc_bias[2] = (float)(acc_sum[2] / (double)total_samples);
    s_gyro_bias[0] = (float)(gyro_sum[0] / (double)total_samples);
    s_gyro_bias[1] = (float)(gyro_sum[1] / (double)total_samples);
    s_gyro_bias[2] = (float)(gyro_sum[2] / (double)total_samples);
    s_bias_loaded = true;

    esp_err_t err = imu_calib_store_bias();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "calib store failed: %s", esp_err_to_name(err));
        imu_calib_write_csv("FAIL_STORE");
        app_control_resume_all();
        return err;
    }

    imu_calib_log_bias("calib ok");
    imu_calib_write_csv("OK");
    app_control_resume_all();
    return ESP_OK;
}

esp_err_t app_imu_calib_reset(void)
{
    imu_calib_set_bias_zero();
    s_bias_loaded = true;
    esp_err_t err = imu_calib_store_bias();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "reset store failed: %s", esp_err_to_name(err));
        return err;
    }
    imu_calib_log_bias("calib reset");
    imu_calib_write_csv("RESET");
    return ESP_OK;
}

bool app_imu_calib_handle_line(const char *line)
{
    if (!line || !line[0]) {
        return false;
    }

    const char *cmd = imu_calib_skip_ws(line);
    if (strncmp(cmd, "imu_calib_reset", 15) == 0) {
        (void)app_imu_calib_reset();
        return true;
    }

    if (strncmp(cmd, "imu_calib", 9) == 0) {
        int seconds = atoi(cmd + 9);
        if (seconds <= 0) {
            ESP_LOGW(TAG, "usage: imu_calib <seconds>");
            return true;
        }
        ESP_LOGI(TAG, "imu calib start: %d seconds", seconds);
        (void)app_imu_calib_run(seconds);
        return true;
    }

    return false;
}
