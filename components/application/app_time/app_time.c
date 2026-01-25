#include "app_time.h"

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#define APP_TIME_NVS_NAMESPACE "app_time"
#define APP_TIME_NVS_KEY "last_ts"
#define APP_TIME_SAVE_PERIOD_SEC 3600
#define APP_TIME_TZ_DEFAULT "CST-8"
#define APP_TIME_TZ_OFFSET_SEC (8 * 3600)
#define APP_TIME_DEFAULT_YEAR 2000
#define APP_TIME_DEFAULT_MONTH 1
#define APP_TIME_DEFAULT_DAY 1
#define APP_TIME_DEFAULT_HOUR 0
#define APP_TIME_DEFAULT_MIN 0
#define APP_TIME_DEFAULT_SEC 0

static const char *TAG = "app_time";

static bool s_started = false;
static bool s_time_valid = false;
static app_time_source_t s_time_source = APP_TIME_SOURCE_NONE;
static time_t s_compile_epoch = 0;
static TaskHandle_t s_save_task = NULL;
static SemaphoreHandle_t s_nvs_lock = NULL;

static void app_time_lock_init(void)
{
    if (!s_nvs_lock) {
        s_nvs_lock = xSemaphoreCreateMutex();
    }
}

static void app_time_lock(void)
{
    if (s_nvs_lock) {
        xSemaphoreTake(s_nvs_lock, portMAX_DELAY);
    }
}

static void app_time_unlock(void)
{
    if (s_nvs_lock) {
        xSemaphoreGive(s_nvs_lock);
    }
}

static bool app_time_is_leap(int year)
{
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

static int app_time_days_in_month(int year, int month)
{
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && app_time_is_leap(year)) {
        return 29;
    }
    if (month < 1 || month > 12) {
        return 0;
    }
    return days[month - 1];
}

static bool app_time_epoch_from_utc(int year, int month, int day,
                                    int hour, int minute, int second,
                                    time_t *out_epoch)
{
    if (!out_epoch) {
        return false;
    }
    if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31) {
        return false;
    }
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return false;
    }

    int max_day = app_time_days_in_month(year, month);
    if (day > max_day) {
        return false;
    }

    int64_t days = 0;
    for (int y = 1970; y < year; ++y) {
        days += app_time_is_leap(y) ? 366 : 365;
    }
    for (int m = 1; m < month; ++m) {
        days += app_time_days_in_month(year, m);
    }
    days += (day - 1);

    int64_t total = days * 86400LL + hour * 3600LL + minute * 60LL + second;
    *out_epoch = (time_t)total;
    return true;
}

static int app_time_parse_month(const char *mon)
{
    static const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    for (int i = 0; i < 12; ++i) {
        if (strncmp(mon, months[i], 3) == 0) {
            return i + 1;
        }
    }
    return 0;
}

static time_t app_time_get_compile_epoch(void)
{
    const char *date = __DATE__;
    const char *time_str = __TIME__;
    if (!date || !time_str) {
        return 0;
    }

    int month = app_time_parse_month(date);
    int day = atoi(date + 4);
    int year = atoi(date + 7);
    int hour = atoi(time_str);
    int minute = atoi(time_str + 3);
    int second = atoi(time_str + 6);
    if (month == 0 || day <= 0 || year <= 0) {
        return 0;
    }

    time_t epoch = 0;
    if (!app_time_epoch_from_utc(year, month, day, hour, minute, second, &epoch)) {
        return 0;
    }

    if (epoch > APP_TIME_TZ_OFFSET_SEC) {
        epoch -= APP_TIME_TZ_OFFSET_SEC;
    }
    return epoch;
}

static time_t app_time_get_default_epoch(void)
{
    time_t epoch = 0;
    if (!app_time_epoch_from_utc(APP_TIME_DEFAULT_YEAR, APP_TIME_DEFAULT_MONTH,
                                 APP_TIME_DEFAULT_DAY, APP_TIME_DEFAULT_HOUR,
                                 APP_TIME_DEFAULT_MIN, APP_TIME_DEFAULT_SEC,
                                 &epoch)) {
        return 0;
    }
    if (epoch > APP_TIME_TZ_OFFSET_SEC) {
        epoch -= APP_TIME_TZ_OFFSET_SEC;
    }
    return epoch;
}

static bool app_time_nvs_read(time_t *out_epoch)
{
    if (!out_epoch) {
        return false;
    }
    bool ok = false;
    nvs_handle_t handle;
    app_time_lock();
    esp_err_t err = nvs_open(APP_TIME_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_OK) {
        int64_t value = 0;
        err = nvs_get_i64(handle, APP_TIME_NVS_KEY, &value);
        if (err == ESP_OK) {
            *out_epoch = (time_t)value;
            ok = true;
        }
        nvs_close(handle);
    }
    app_time_unlock();
    return ok;
}

static void app_time_nvs_write(time_t epoch)
{
    nvs_handle_t handle;
    app_time_lock();
    esp_err_t err = nvs_open(APP_TIME_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        app_time_unlock();
        ESP_LOGW(TAG, "NVS open failed: %s", esp_err_to_name(err));
        return;
    }

    err = nvs_set_i64(handle, APP_TIME_NVS_KEY, (int64_t)epoch);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS set failed: %s", esp_err_to_name(err));
        nvs_close(handle);
        app_time_unlock();
        return;
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS commit failed: %s", esp_err_to_name(err));
    }
    nvs_close(handle);
    app_time_unlock();
}

static void app_time_save_task(void *arg)
{
    (void)arg;
    const TickType_t delay = pdMS_TO_TICKS(APP_TIME_SAVE_PERIOD_SEC * 1000U);
    while (1) {
        vTaskDelay(delay);
        app_time_save_now();
    }
}

void app_time_start(void)
{
    if (s_started) {
        return;
    }
    s_started = true;

    app_time_lock_init();
    app_time_set_timezone(APP_TIME_TZ_DEFAULT);
    s_compile_epoch = app_time_get_compile_epoch();

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS init failed: %s", esp_err_to_name(err));
    }

    bool time_loaded = false;
    time_t stored_epoch = 0;
    if (app_time_nvs_read(&stored_epoch)) {
        if (stored_epoch > 0 && stored_epoch > s_compile_epoch) {
            app_time_set_unix(stored_epoch, APP_TIME_SOURCE_NVS);
            time_loaded = true;
        }
    }

    if (!time_loaded) {
        time_t default_epoch = app_time_get_default_epoch();
        if (default_epoch > 0) {
            struct timeval tv = {
                .tv_sec = default_epoch,
                .tv_usec = 0,
            };
            settimeofday(&tv, NULL);
            s_time_valid = false;
            s_time_source = APP_TIME_SOURCE_NONE;
        }
    }

    if (!s_save_task) {
        xTaskCreate(app_time_save_task, "app_time_save", 4096, NULL, 5, &s_save_task);
    }
}

void app_time_set_timezone(const char *tz)
{
    if (!tz || !tz[0]) {
        return;
    }
    setenv("TZ", tz, 1);
    tzset();
}

bool app_time_get_unix_from_utc(const struct tm *utc_time, time_t *out_unix)
{
    if (!utc_time || !out_unix) {
        return false;
    }
    int year = utc_time->tm_year + 1900;
    int month = utc_time->tm_mon + 1;
    int day = utc_time->tm_mday;
    int hour = utc_time->tm_hour;
    int minute = utc_time->tm_min;
    int second = utc_time->tm_sec;
    return app_time_epoch_from_utc(year, month, day, hour, minute, second, out_unix);
}

bool app_time_set_unix(time_t unix_time, app_time_source_t source)
{
    if (unix_time <= 0) {
        return false;
    }
    struct timeval tv = {
        .tv_sec = unix_time,
        .tv_usec = 0,
    };
    settimeofday(&tv, NULL);
    s_time_valid = true;
    s_time_source = source;
    return true;
}

bool app_time_set_utc(const struct tm *utc_time, app_time_source_t source)
{
    time_t unix_time = 0;
    if (!app_time_get_unix_from_utc(utc_time, &unix_time)) {
        return false;
    }
    return app_time_set_unix(unix_time, source);
}

void app_time_save_now(void)
{
    time_t now = time(NULL);
    if (now <= 0) {
        return;
    }
    if (s_compile_epoch > 0 && now < s_compile_epoch) {
        return;
    }
    app_time_nvs_write(now);
}

bool app_time_is_valid(void)
{
    if (s_time_valid) {
        return true;
    }
    time_t now = time(NULL);
    if (now <= 0) {
        return false;
    }
    if (s_compile_epoch > 0 && now <= s_compile_epoch) {
        return false;
    }
    return true;
}

app_time_source_t app_time_get_source(void)
{
    return s_time_source;
}
