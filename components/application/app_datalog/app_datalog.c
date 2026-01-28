#include "app_datalog.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/time.h>
#include <time.h>

#include "driver/gpio.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"

#include "app_state.h"
#include "app_time.h"
#include "app_sdcard.h"

#define TAG "app_datalog"

#define MOUNT_POINT "/sdcard"
#define UNSYNCED_DIR "UNSYNCED"

#define SDCARD_SPI_HOST SPI2_HOST
#define SDCARD_PIN_MOSI GPIO_NUM_17
#define SDCARD_PIN_MISO GPIO_NUM_15
#define SDCARD_PIN_SCLK GPIO_NUM_16
#define SDCARD_PIN_CS GPIO_NUM_18
#define SDCARD_BOOT_KHZ 400

#define LOGGER_QUEUE_LENGTH 256
#define LOGGER_TASK_STACK 4096
#define LOGGER_TASK_PRIO 8

#define SAMPLER_TASK_STACK 4096
#define SAMPLER_TASK_PRIO 7
#define SAMPLER_INTERVAL_MS 40


#define RAW_FLUSH_LINES 50
#define RAW_FSYNC_FLUSHES 1

#define LOGGER_IDLE_WAIT_MS 80

#define FILE_BUF_SIZE 4096
#define DIR_BUF_SIZE 64
#define FILE_NAME_SIZE 32
#define PATH_BUF_SIZE 128
#define DATETIME_BUF_SIZE 24
#define FLOAT_BUF_SIZE 16

static QueueHandle_t s_raw_queue = NULL;
static TaskHandle_t s_logger_task = NULL;
static TaskHandle_t s_sampler_task = NULL;
static SemaphoreHandle_t s_mount_lock = NULL;
static SemaphoreHandle_t s_session_lock = NULL;
static volatile bool s_datalog_enabled = true;
static volatile bool s_datalog_stop_requested = false;
static volatile bool s_default_raw_enabled = true;

static FILE *s_raw_file = NULL;
static char s_raw_file_buf[FILE_BUF_SIZE];
static char s_raw_dir[DIR_BUF_SIZE];
static char s_raw_name[FILE_NAME_SIZE];
static uint32_t s_raw_lines_since_flush = 0;
static uint32_t s_raw_flushes_since_sync = 0;
static volatile uint32_t s_raw_dropped = 0;
static bool s_session_active = false;
static char s_session_dir[DIR_BUF_SIZE];
static char s_session_file[FILE_NAME_SIZE];
static char s_session_label[APP_DATALOG_MODE_MAX_LEN];

static int64_t s_last_imu_ts = -1;
static bool s_have_gps_cache = false;
static float s_last_speed = 0.0f;
static bool s_last_gps_valid = false;

static bool s_bus_ok = false;
static bool s_mounted = false;
static sdmmc_card_t *s_card = NULL;

static sdmmc_host_t s_host = SDSPI_HOST_DEFAULT();
static spi_bus_config_t s_buscfg = {
    .mosi_io_num = SDCARD_PIN_MOSI,
    .miso_io_num = SDCARD_PIN_MISO,
    .sclk_io_num = SDCARD_PIN_SCLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 8192,
};
static sdspi_device_config_t s_slotcfg;
static esp_vfs_fat_sdmmc_mount_config_t s_mountcfg = {
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 0
};

static void datalog_close_raw_file(void);
static bool datalog_ensure_dir(const char *dir);
static esp_err_t datalog_mount(void);

static void datalog_lock(void)
{
    if (!s_mount_lock) {
        s_mount_lock = xSemaphoreCreateMutex();
    }
    if (s_mount_lock) {
        xSemaphoreTake(s_mount_lock, portMAX_DELAY);
    }
}

static void datalog_unlock(void)
{
    if (s_mount_lock) {
        xSemaphoreGive(s_mount_lock);
    }
}

static void datalog_session_lock(void)
{
    if (!s_session_lock) {
        s_session_lock = xSemaphoreCreateMutex();
    }
    if (s_session_lock) {
        xSemaphoreTake(s_session_lock, portMAX_DELAY);
    }
}

static void datalog_session_unlock(void)
{
    if (s_session_lock) {
        xSemaphoreGive(s_session_lock);
    }
}

static bool datalog_session_is_active(void)
{
    bool active = false;
    datalog_session_lock();
    active = s_session_active;
    datalog_session_unlock();
    return active;
}

static bool datalog_remount_locked(void)
{
    if (s_raw_file) {
        datalog_close_raw_file();
    }

    if (s_mounted && s_card) {
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, s_card);
    }
    s_mounted = false;
    s_card = NULL;

    esp_err_t err = datalog_mount();
    return (err == ESP_OK || err == ESP_ERR_INVALID_STATE);
}

static bool datalog_ensure_dir_with_remount(const char *dir)
{
    if (datalog_ensure_dir(dir)) {
        return true;
    }
    if (errno != EIO) {
        return false;
    }

    ESP_LOGW(TAG, "sd io error, remounting");
    if (!datalog_remount_locked()) {
        return false;
    }
    return datalog_ensure_dir(dir);
}

static bool datalog_get_epoch_ms(int64_t *out_ms)
{
    if (!out_ms) {
        return false;
    }
    if (!app_time_is_valid()) {
        *out_ms = 0;
        return false;
    }
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        *out_ms = 0;
        return false;
    }
    *out_ms = (int64_t)tv.tv_sec * 1000 + (tv.tv_usec / 1000);
    return true;
}

static void datalog_format_datetime(int64_t epoch_ms, char *out, size_t out_sz)
{
    if (!out || out_sz == 0) {
        return;
    }
    if (epoch_ms <= 0) {
        snprintf(out, out_sz, "NA");
        return;
    }
    time_t sec = (time_t)(epoch_ms / 1000);
    struct tm tm_info;
    if (!localtime_r(&sec, &tm_info)) {
        snprintf(out, out_sz, "NA");
        return;
    }
    snprintf(out, out_sz, "%04d-%02d-%02d %02d:%02d:%02d",
             tm_info.tm_year + 1900,
             tm_info.tm_mon + 1,
             tm_info.tm_mday,
             tm_info.tm_hour,
             tm_info.tm_min,
             tm_info.tm_sec);
}

static void datalog_format_float(char *out, size_t out_sz, float value)
{
    if (!out || out_sz == 0) {
        return;
    }
    if (!isfinite(value)) {
        snprintf(out, out_sz, "NA");
        return;
    }
    snprintf(out, out_sz, "%.3f", value);
}

static void datalog_copy_token(char *dst, size_t dst_sz, const char *src)
{
    if (!dst || dst_sz == 0) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dst_sz - 1);
    dst[dst_sz - 1] = '\0';
}

static bool datalog_format_session_label(const char *label, char *out, size_t out_sz)
{
    if (!label || !out || out_sz == 0) {
        return false;
    }

    size_t max_len = out_sz - 1;
#if CONFIG_FATFS_LFN_NONE
    if (max_len > 8) {
        max_len = 8;
    }
#endif

    size_t out_len = 0;
    for (size_t i = 0; label[i] != '\0' && out_len < max_len; ++i) {
        unsigned char ch = (unsigned char)label[i];
        if (isalnum(ch)) {
            out[out_len++] = (char)tolower(ch);
        } else if (ch == '_' || ch == '-') {
            out[out_len++] = '_';
        }
    }

    out[out_len] = '\0';
    return (out_len > 0);
}

static bool datalog_format_session_filename(const char *label, uint32_t index, char *out, size_t out_sz)
{
    if (!label || !out || out_sz == 0) {
        return false;
    }

#if CONFIG_FATFS_LFN_NONE
    unsigned int width = 2;
    if (index >= 1000) {
        width = 4;
    } else if (index >= 100) {
        width = 3;
    }

    size_t label_len = strlen(label);
    if (label_len > 3) {
        label_len = 3;
    }
    if (label_len == 0) {
        return false;
    }

    int len = snprintf(out, out_sz, "%.*s_%0*u.csv", (int)label_len, label, width, (unsigned int)index);
    return (len > 0 && len < (int)out_sz);
#else
    int len = snprintf(out, out_sz, "%s_%02" PRIu32 ".csv", label, index);
    return (len > 0 && len < (int)out_sz);
#endif
}

static uint32_t datalog_find_next_session_index(const char *dir, const char *label)
{
    if (!dir || !label || label[0] == '\0') {
        return 1;
    }

    char path[PATH_BUF_SIZE];
    char file[FILE_NAME_SIZE];
    for (uint32_t i = 1; i <= 9999; ++i) {
        if (!datalog_format_session_filename(label, i, file, sizeof(file))) {
            return UINT32_MAX;
        }
        int len = snprintf(path, sizeof(path), "%s/%s", dir, file);
        if (len < 0 || len >= (int)sizeof(path)) {
            return UINT32_MAX;
        }
        struct stat st;
        if (stat(path, &st) != 0) {
            if (errno == ENOENT) {
                return i;
            }
        }
    }

    return UINT32_MAX;
}

static bool datalog_build_raw_paths(const DatalogRaw_t *raw,
                                    char *dir, size_t dir_sz,
                                    char *file, size_t file_sz)
{
    bool use_session = false;
    char session_dir[DIR_BUF_SIZE];
    char session_file[FILE_NAME_SIZE];

    datalog_session_lock();
    if (s_session_active) {
        use_session = true;
        datalog_copy_token(session_dir, sizeof(session_dir), s_session_dir);
        datalog_copy_token(session_file, sizeof(session_file), s_session_file);
    }
    datalog_session_unlock();

    if (use_session) {
        datalog_copy_token(dir, dir_sz, session_dir);
        datalog_copy_token(file, file_sz, session_file);
        return true;
    }

    if (raw && raw->datetime_local_ms > 0) {
        time_t sec = (time_t)(raw->datetime_local_ms / 1000);
        struct tm tm_info;
        if (localtime_r(&sec, &tm_info)) {
            snprintf(dir, dir_sz, MOUNT_POINT "/%04d%02d%02d",
                     tm_info.tm_year + 1900,
                     tm_info.tm_mon + 1,
                     tm_info.tm_mday);
            snprintf(file, file_sz, "raw_%02d.csv", tm_info.tm_hour);
            return true;
        }
    }

    uint32_t hour = 0;
    if (raw) {
        hour = (uint32_t)((raw->uptime_ms / 3600000ULL) % 24ULL);
    }
    snprintf(dir, dir_sz, MOUNT_POINT "/%s", UNSYNCED_DIR);
    snprintf(file, file_sz, "raw_unsynced_%02" PRIu32 ".csv", hour);
    return false;
}

static bool datalog_build_daily_dir(char *dir, size_t dir_sz)
{
    if (app_time_is_valid()) {
        time_t now = time(NULL);
        if (now > 0) {
            struct tm tm_info;
            if (localtime_r(&now, &tm_info)) {
                snprintf(dir, dir_sz, MOUNT_POINT "/%04d%02d%02d",
                         tm_info.tm_year + 1900,
                         tm_info.tm_mon + 1,
                         tm_info.tm_mday);
                return true;
            }
        }
    }
    snprintf(dir, dir_sz, MOUNT_POINT "/%s", UNSYNCED_DIR);
    return false;
}

static bool datalog_ensure_dir(const char *dir)
{
    struct stat st;
    if (stat(dir, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return true;
        }
        ESP_LOGE(TAG, "path not dir: %s", dir);
        return false;
    }

    if (mkdir(dir, 0775) != 0) {
        if (errno == EEXIST) {
            return true;
        }
        ESP_LOGE(TAG, "mkdir failed: %s errno=%d (%s)", dir, errno, strerror(errno));
        return false;
    }
    return true;
}

static esp_err_t datalog_mount(void)
{
    if (!s_bus_ok) {
        s_host.slot = SDCARD_SPI_HOST;
        s_host.max_freq_khz = SDCARD_BOOT_KHZ;

        gpio_config_t io = {
            .pin_bit_mask = (1ULL << SDCARD_PIN_MOSI) | (1ULL << SDCARD_PIN_MISO) | (1ULL << SDCARD_PIN_CS),
            .mode = GPIO_MODE_INPUT_OUTPUT,
            .pull_up_en = 1,
            .pull_down_en = 0,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io);
        gpio_set_level(SDCARD_PIN_CS, 1);

        esp_err_t err = spi_bus_initialize(s_host.slot, &s_buscfg, SPI_DMA_CH_AUTO);
        if (err == ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "SPI bus already initialized, reuse current bus");
        } else if (err != ESP_OK) {
            ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(err));
            return err;
        }
        s_bus_ok = true;
    }

    if (!s_mounted) {
        s_slotcfg = (sdspi_device_config_t)SDSPI_DEVICE_CONFIG_DEFAULT();
        s_slotcfg.gpio_cs = SDCARD_PIN_CS;
        s_slotcfg.host_id = s_host.slot;

        esp_err_t err = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &s_host, &s_slotcfg, &s_mountcfg, &s_card);
        if (err == ESP_OK || err == ESP_ERR_INVALID_STATE) {
            s_mounted = true;
            ESP_LOGI(TAG, "SD mounted at %s", MOUNT_POINT);
        } else {
            ESP_LOGW(TAG, "mount failed: %s", esp_err_to_name(err));
            return err;
        }
    }

    return ESP_OK;
}

static bool datalog_ensure_mounted(void)
{
    if (s_mounted) {
        return true;
    }
    datalog_lock();
    if (s_mounted) {
        datalog_unlock();
        return true;
    }
    esp_err_t err = datalog_mount();
    datalog_unlock();
    return (err == ESP_OK || err == ESP_ERR_INVALID_STATE);
}

static bool datalog_open_raw_file(const char *dir, const char *file)
{
    if (!datalog_ensure_dir_with_remount(dir)) {
        return false;
    }

    char path[PATH_BUF_SIZE];
    snprintf(path, sizeof(path), "%s/%s", dir, file);

    bool need_header = true;
    struct stat st;
    if (stat(path, &st) == 0 && st.st_size > 0) {
        need_header = false;
    }

    FILE *f = fopen(path, "a");
    if (!f && errno == EIO) {
        ESP_LOGW(TAG, "fopen io error, remounting");
        if (datalog_remount_locked()) {
            f = fopen(path, "a");
        }
    }
    if (!f) {
        ESP_LOGE(TAG, "fopen failed: %s errno=%d (%s)", path, errno, strerror(errno));
        return false;
    }

    if (setvbuf(f, s_raw_file_buf, _IOFBF, sizeof(s_raw_file_buf)) != 0) {
        ESP_LOGW(TAG, "setvbuf failed, continue unbuffered");
    }

    if (need_header) {
        const char *header =
            "datetime_local,uptime_ms,acc_x,acc_y,acc_z,gyro_x,gyro_y,gyro_z,"
            "speed_mps,turn_rate_deg_s,gps_valid\r\n";
        if (fprintf(f, "%s", header) < 0) {
            ESP_LOGE(TAG, "write header failed errno=%d (%s)", errno, strerror(errno));
            fclose(f);
            return false;
        }
        fflush(f);
        (void)fsync(fileno(f));
    }

    s_raw_file = f;
    datalog_copy_token(s_raw_dir, sizeof(s_raw_dir), dir);
    datalog_copy_token(s_raw_name, sizeof(s_raw_name), file);
    s_raw_lines_since_flush = 0;
    s_raw_flushes_since_sync = 0;

    ESP_LOGI(TAG, "raw open %s/%s", dir, file);
    return true;
}

static bool datalog_rotate_raw_if_needed(const char *dir, const char *file)
{
    if (s_raw_file && strcmp(dir, s_raw_dir) == 0 && strcmp(file, s_raw_name) == 0) {
        return true;
    }

    if (s_raw_file) {
        fclose(s_raw_file);
        s_raw_file = NULL;
    }

    return datalog_open_raw_file(dir, file);
}

static bool datalog_emit_raw_line(const DatalogRaw_t *raw)
{
    if (!s_raw_file || !raw) {
        return false;
    }

    char datetime[DATETIME_BUF_SIZE];
    datalog_format_datetime(raw->datetime_local_ms, datetime, sizeof(datetime));

    char acc_x[FLOAT_BUF_SIZE];
    char acc_y[FLOAT_BUF_SIZE];
    char acc_z[FLOAT_BUF_SIZE];
    char gyr_x[FLOAT_BUF_SIZE];
    char gyr_y[FLOAT_BUF_SIZE];
    char gyr_z[FLOAT_BUF_SIZE];
    char speed[FLOAT_BUF_SIZE];
    char turn[FLOAT_BUF_SIZE];

    datalog_format_float(acc_x, sizeof(acc_x), raw->acc_x);
    datalog_format_float(acc_y, sizeof(acc_y), raw->acc_y);
    datalog_format_float(acc_z, sizeof(acc_z), raw->acc_z);
    datalog_format_float(gyr_x, sizeof(gyr_x), raw->gyro_x);
    datalog_format_float(gyr_y, sizeof(gyr_y), raw->gyro_y);
    datalog_format_float(gyr_z, sizeof(gyr_z), raw->gyro_z);
    datalog_format_float(speed, sizeof(speed), raw->speed_mps);
    datalog_format_float(turn, sizeof(turn), raw->turn_rate_deg_s);

    int rc = fprintf(s_raw_file,
                     "%s,%llu,%s,%s,%s,%s,%s,%s,%s,%s,%u\r\n",
                     datetime,
                     (unsigned long long)raw->uptime_ms,
                     acc_x, acc_y, acc_z,
                     gyr_x, gyr_y, gyr_z,
                     speed, turn,
                     (unsigned int)raw->gps_valid);

    if (rc < 0 || ferror(s_raw_file)) {
        return false;
    }
    return true;
}

static void datalog_raw_flush(bool force_sync)
{
    if (!s_raw_file || s_raw_lines_since_flush == 0) {
        return;
    }
    if (fflush(s_raw_file) == 0) {
        if (force_sync || ++s_raw_flushes_since_sync >= RAW_FSYNC_FLUSHES) {
            s_raw_flushes_since_sync = 0;
            (void)fsync(fileno(s_raw_file));
        }
    } else {
        ESP_LOGW(TAG, "flush failed errno=%d (%s)", errno, strerror(errno));
    }
    s_raw_lines_since_flush = 0;
}

static void datalog_close_raw_file(void)
{
    if (!s_raw_file) {
        return;
    }
    fflush(s_raw_file);
    (void)fsync(fileno(s_raw_file));
    fclose(s_raw_file);
    s_raw_file = NULL;
}

static void datalog_write_raw(const DatalogRaw_t *raw)
{
    if (!datalog_session_is_active() && !s_default_raw_enabled) {
        return;
    }

    char dir[DIR_BUF_SIZE];
    char file[FILE_NAME_SIZE];
    (void)datalog_build_raw_paths(raw, dir, sizeof(dir), file, sizeof(file));

    if (!datalog_rotate_raw_if_needed(dir, file)) {
        return;
    }

    if (!datalog_emit_raw_line(raw)) {
        int e = errno;
        ESP_LOGW(TAG, "write failed errno=%d (%s) retry", e, strerror(e));
        clearerr(s_raw_file);
        fclose(s_raw_file);
        s_raw_file = NULL;
        if (!datalog_open_raw_file(dir, file)) {
            return;
        }
        if (!datalog_emit_raw_line(raw)) {
            return;
        }
    }

    if (++s_raw_lines_since_flush >= RAW_FLUSH_LINES) {
        datalog_raw_flush(false);
    }
}

static bool datalog_build_raw_sample(DatalogRaw_t *out)
{
    if (!out) {
        return false;
    }

    app_state_imu_sample_t imu;
    if (!app_state_get_latest_imu(&imu)) {
        return false;
    }
    if (imu.timestamp_us == s_last_imu_ts) {
        return false;
    }
    s_last_imu_ts = imu.timestamp_us;

    memset(out, 0, sizeof(*out));
    out->acc_x = (float)imu.acc_x;
    out->acc_y = (float)imu.acc_y;
    out->acc_z = (float)imu.acc_z;
    out->gyro_x = (float)imu.gyr_x;
    out->gyro_y = (float)imu.gyr_y;
    out->gyro_z = (float)imu.gyr_z;
    out->uptime_ms = (uint64_t)(esp_timer_get_time() / 1000);

    int64_t epoch_ms = 0;
    if (datalog_get_epoch_ms(&epoch_ms)) {
        out->datetime_local_ms = epoch_ms;
    } else {
        out->datetime_local_ms = 0;
    }

    GNSS_Data gps;
    if (app_state_get_latest_gps(&gps)) {
        s_last_speed = gps.speed;
        s_last_gps_valid = (gps.is_valid == 1);
        s_have_gps_cache = true;
    }

    if (s_have_gps_cache) {
        out->speed_mps = s_last_speed;
        out->gps_valid = s_last_gps_valid ? 1 : 0;
    } else {
        out->speed_mps = NAN;
        out->gps_valid = 0;
    }
    out->turn_rate_deg_s = 0.0f;

    return true;
}

static void datalog_logger_task(void *arg)
{
    (void)arg;
    DatalogRaw_t raw;
    int64_t last_mount_log_us = 0;

    while (1) {
        if (xQueueReceive(s_raw_queue, &raw, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (!s_datalog_enabled) {
            if (app_sdcard_lock_fs(pdMS_TO_TICKS(1000))) {
                datalog_close_raw_file();
                app_sdcard_unlock_fs();
            }
            if (s_datalog_stop_requested) {
                ESP_LOGW(TAG, "datalog stopped");
                s_datalog_stop_requested = false;
            }
            while (xQueueReceive(s_raw_queue, &raw, 0) == pdTRUE) {
                s_raw_dropped++;
            }
            continue;
        }

        do {
            if (!datalog_ensure_mounted()) {
                s_raw_dropped++;
                int64_t now_us = esp_timer_get_time();
                if (now_us - last_mount_log_us > 5000000LL) {
                    ESP_LOGW(TAG, "mount failed, dropping raw data");
                    last_mount_log_us = now_us;
                }
            } else {
                if (app_sdcard_lock_fs(portMAX_DELAY)) {
                    datalog_write_raw(&raw);
                    app_sdcard_unlock_fs();
                } else {
                    s_raw_dropped++;
                }
            }
        } while (xQueueReceive(s_raw_queue, &raw, pdMS_TO_TICKS(LOGGER_IDLE_WAIT_MS)) == pdTRUE);

        if (app_sdcard_lock_fs(portMAX_DELAY)) {
            datalog_raw_flush(true);
            app_sdcard_unlock_fs();
        }
    }
}

static void datalog_sampler_task(void *arg)
{
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();
    while (1) {
        if (!s_datalog_enabled) {
            vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SAMPLER_INTERVAL_MS));
            continue;
        }
        DatalogRaw_t raw;
        if (datalog_build_raw_sample(&raw)) {
            app_datalog_enqueue_raw(&raw);
        }
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SAMPLER_INTERVAL_MS));
    }
}


esp_err_t app_datalog_start(void)
{
    static bool started = false;
    if (started) {
        return ESP_OK;
    }

    s_datalog_enabled = true;
    s_raw_queue = xQueueCreate(LOGGER_QUEUE_LENGTH, sizeof(DatalogRaw_t));
    if (!s_raw_queue) {
        ESP_LOGE(TAG, "raw queue create failed");
        return ESP_ERR_NO_MEM;
    }

    if (xTaskCreate(datalog_logger_task, "datalog_writer", LOGGER_TASK_STACK, NULL,
                    LOGGER_TASK_PRIO, &s_logger_task) != pdPASS) {
        ESP_LOGE(TAG, "logger task create failed");
        return ESP_ERR_NO_MEM;
    }

    if (xTaskCreate(datalog_sampler_task, "datalog_sampler", SAMPLER_TASK_STACK, NULL,
                    SAMPLER_TASK_PRIO, &s_sampler_task) != pdPASS) {
        ESP_LOGE(TAG, "sampler task create failed");
        return ESP_ERR_NO_MEM;
    }

    (void)datalog_ensure_mounted();
    started = true;
    ESP_LOGI(TAG, "datalog start");
    return ESP_OK;
}

esp_err_t app_datalog_start_session(const char *label)
{
    if (!label || label[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    char fs_label[APP_DATALOG_MODE_MAX_LEN];
    if (!datalog_format_session_label(label, fs_label, sizeof(fs_label))) {
        return ESP_ERR_INVALID_ARG;
    }

    datalog_session_lock();
    bool already_active = s_session_active &&
        (strncmp(s_session_label, fs_label, sizeof(s_session_label)) == 0);
    datalog_session_unlock();
    if (already_active) {
        return ESP_OK;
    }

    if (!app_sdcard_lock_fs(pdMS_TO_TICKS(2000))) {
        ESP_LOGW(TAG, "session start skipped: sd fs busy");
        return ESP_ERR_TIMEOUT;
    }
    if (!datalog_ensure_mounted()) {
        ESP_LOGW(TAG, "session start skipped: SD not mounted");
        app_sdcard_unlock_fs();
        return ESP_FAIL;
    }

    if (strcmp(label, fs_label) != 0) {
        ESP_LOGW(TAG, "session label sanitized: %s -> %s", label, fs_label);
    }

    char dir[DIR_BUF_SIZE];
    int dir_len = snprintf(dir, sizeof(dir), MOUNT_POINT "/%s", fs_label);
    if (dir_len < 0 || dir_len >= (int)sizeof(dir)) {
        app_sdcard_unlock_fs();
        return ESP_ERR_INVALID_SIZE;
    }
    if (!datalog_ensure_dir_with_remount(dir)) {
        app_sdcard_unlock_fs();
        return ESP_FAIL;
    }

    uint32_t next_index = datalog_find_next_session_index(dir, fs_label);
    if (next_index == UINT32_MAX) {
        app_sdcard_unlock_fs();
        return ESP_FAIL;
    }

    char file[FILE_NAME_SIZE];
    if (!datalog_format_session_filename(fs_label, next_index, file, sizeof(file))) {
        app_sdcard_unlock_fs();
        return ESP_ERR_INVALID_SIZE;
    }

    datalog_session_lock();
    s_session_active = true;
    datalog_copy_token(s_session_dir, sizeof(s_session_dir), dir);
    datalog_copy_token(s_session_file, sizeof(s_session_file), file);
    datalog_copy_token(s_session_label, sizeof(s_session_label), fs_label);
    datalog_session_unlock();

    app_sdcard_unlock_fs();
    ESP_LOGI(TAG, "session start %s/%s", dir, file);
    return ESP_OK;
}

void app_datalog_stop_session(void)
{
    datalog_session_lock();
    s_session_active = false;
    s_session_dir[0] = '\0';
    s_session_file[0] = '\0';
    s_session_label[0] = '\0';
    datalog_session_unlock();

    if (app_sdcard_lock_fs(pdMS_TO_TICKS(2000))) {
        datalog_close_raw_file();
        app_sdcard_unlock_fs();
    } else {
        ESP_LOGW(TAG, "session stop: sd fs busy");
    }
}

void app_datalog_set_default_raw_enabled(bool enable)
{
    s_default_raw_enabled = enable;
}

bool app_datalog_is_default_raw_enabled(void)
{
    return s_default_raw_enabled;
}

void app_datalog_stop(void)
{
    s_datalog_enabled = false;
    s_datalog_stop_requested = true;
    if (s_raw_queue) {
        DatalogRaw_t dummy = {0};
        (void)xQueueSend(s_raw_queue, &dummy, 0);
    }
}

void app_datalog_resume(void)
{
    s_datalog_enabled = true;
    ESP_LOGW(TAG, "datalog resume");
}

bool app_datalog_is_running(void)
{
    return s_datalog_enabled;
}

void app_datalog_enqueue_raw(const DatalogRaw_t *raw_data)
{
    if (!raw_data || !s_raw_queue || !s_datalog_enabled) {
        return;
    }
    if (xQueueSend(s_raw_queue, raw_data, 0) != pdTRUE) {
        s_raw_dropped++;
        if ((s_raw_dropped % 100) == 0) {
            ESP_LOGW(TAG, "raw queue full drop=%u", (unsigned int)s_raw_dropped);
        }
    }
}

void app_datalog_log_event(const DatalogEvent_t *event)
{
    if (!event) {
        return;
    }
    if (!app_sdcard_lock_fs(pdMS_TO_TICKS(2000))) {
        ESP_LOGW(TAG, "event log skipped: sd fs busy");
        return;
    }
    if (!datalog_ensure_mounted()) {
        ESP_LOGW(TAG, "event log skipped: SD not mounted");
        app_sdcard_unlock_fs();
        return;
    }

    char dir[DIR_BUF_SIZE];
    (void)datalog_build_daily_dir(dir, sizeof(dir));
    if (!datalog_ensure_dir(dir)) {
        app_sdcard_unlock_fs();
        return;
    }

    char path[PATH_BUF_SIZE];
    snprintf(path, sizeof(path), "%s/events.csv", dir);

    bool need_header = true;
    struct stat st;
    if (stat(path, &st) == 0 && st.st_size > 0) {
        need_header = false;
    }

    FILE *f = fopen(path, "a");
    if (!f) {
        ESP_LOGE(TAG, "event fopen failed: %s errno=%d (%s)", path, errno, strerror(errno));
        app_sdcard_unlock_fs();
        return;
    }

    char buf[FILE_BUF_SIZE];
    if (setvbuf(f, buf, _IOFBF, sizeof(buf)) != 0) {
        ESP_LOGW(TAG, "event setvbuf failed");
    }

    if (need_header) {
        const char *header =
            "start_time,end_time,start_uptime_ms,end_uptime_ms,duration_sec,mode,avg_speed_mps\r\n";
        if (fprintf(f, "%s", header) < 0) {
            ESP_LOGE(TAG, "event header write failed");
            fclose(f);
            app_sdcard_unlock_fs();
            return;
        }
    }

    char start[sizeof(event->start_time)];
    char end[sizeof(event->end_time)];
    char mode[sizeof(event->mode)];
    datalog_copy_token(start, sizeof(start), event->start_time);
    datalog_copy_token(end, sizeof(end), event->end_time);
    datalog_copy_token(mode, sizeof(mode), event->mode);

    char line[128];
    int len = snprintf(line, sizeof(line),
                       "%s,%s,%" PRIi64 ",%" PRIi64 ",%.2f,%s,%.3f\r\n",
                       start,
                       end,
                       event->start_uptime_ms,
                       event->end_uptime_ms,
                       event->duration_sec,
                       mode,
                       event->avg_speed_mps);
    if (len < 0 || len >= (int)sizeof(line)) {
        ESP_LOGW(TAG, "event line format overflow");
        fclose(f);
        app_sdcard_unlock_fs();
        return;
    }
    if (fwrite(line, 1, (size_t)len, f) != (size_t)len) {
        ESP_LOGW(TAG, "event write failed");
        fclose(f);
        app_sdcard_unlock_fs();
        return;
    }
    fflush(f);
    (void)fsync(fileno(f));
    fclose(f);
    app_sdcard_unlock_fs();

    ESP_LOGI(TAG, "event logged %s", path);
}

void app_datalog_save_summary(const DatalogSummary_t *summary)
{
    if (!summary) {
        return;
    }
    (void)app_datalog_save_summary_batch(summary, 1);
}

esp_err_t app_datalog_save_summary_batch(const DatalogSummary_t *rows, size_t count)
{
    if (!rows || count == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!app_sdcard_lock_fs(pdMS_TO_TICKS(5000))) {
        ESP_LOGW(TAG, "summary skipped: sd fs busy");
        return ESP_FAIL;
    }
    if (!datalog_ensure_mounted()) {
        ESP_LOGW(TAG, "summary skipped: SD not mounted");
        app_sdcard_unlock_fs();
        return ESP_FAIL;
    }

    char dir[DIR_BUF_SIZE];
    (void)datalog_build_daily_dir(dir, sizeof(dir));
    if (!datalog_ensure_dir(dir)) {
        app_sdcard_unlock_fs();
        return ESP_FAIL;
    }

    char path[PATH_BUF_SIZE];
    char tmp_path[PATH_BUF_SIZE];
    snprintf(path, sizeof(path), "%s/summary.csv", dir);
    snprintf(tmp_path, sizeof(tmp_path), "%s/summary.tmp", dir);

    FILE *f = fopen(tmp_path, "w");
    if (!f) {
        ESP_LOGE(TAG, "summary fopen failed: %s errno=%d (%s)", tmp_path, errno, strerror(errno));
        app_sdcard_unlock_fs();
        return ESP_FAIL;
    }

    char buf[FILE_BUF_SIZE];
    if (setvbuf(f, buf, _IOFBF, sizeof(buf)) != 0) {
        ESP_LOGW(TAG, "summary setvbuf failed");
    }

    const char *header = "mode,total_duration_min,carbon_factor_g_per_min,co2_g\r\n";
    if (fprintf(f, "%s", header) < 0) {
        ESP_LOGE(TAG, "summary header write failed");
        fclose(f);
        app_sdcard_unlock_fs();
        return ESP_FAIL;
    }

    float total_duration_min = 0.0f;
    float total_co2 = 0.0f;

    for (size_t i = 0; i < count; ++i) {
        char mode[sizeof(rows[i].mode)];
        datalog_copy_token(mode, sizeof(mode), rows[i].mode);
        if (mode[0] == '\0') {
            continue;
        }
        fprintf(f, "%s,%.2f,%.2f,%.2f\r\n",
                mode,
                rows[i].total_duration_min,
                rows[i].carbon_factor_g_per_min,
                rows[i].co2_g);
        total_duration_min += rows[i].total_duration_min;
        total_co2 += rows[i].co2_g;
    }

    fprintf(f, "TOTAL,%.2f,0.00,%.2f\r\n", total_duration_min, total_co2);

    fflush(f);
    (void)fsync(fileno(f));
    fclose(f);

    if (rename(tmp_path, path) != 0) {
        ESP_LOGE(TAG, "summary rename failed: %s errno=%d (%s)", path, errno, strerror(errno));
        app_sdcard_unlock_fs();
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "summary saved %s", path);
    app_sdcard_unlock_fs();
    return ESP_OK;
}
