#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_gps.h"
#include "app_gps_parser.h"
#include "app_state.h"
#include "app_time.h"
#include "app_control.h"
#include "gps_interface.h"

static const char *TAG = "gps";

static unsigned char s_read_buf[GPS_BUF_SIZE];
static char s_line_buf[GPS_BUF_SIZE];
static size_t s_line_len = 0;
static gps_parser_t s_parser;
static TaskHandle_t s_gps_task = NULL;
static volatile bool s_gps_enabled = true;

#define GPS_TASK_PERIOD_MS 100
#define GPS_TIME_SYNC_THRESHOLD_SEC 60
#define GPS_NMEA_LOG_PERIOD_MS 8000
static TickType_t s_last_nmea_log = 0;

static bool parse_two_digits(const char *text, int *out)
{
    if (!text || !out) {
        return false;
    }
    if (!isdigit((unsigned char)text[0]) || !isdigit((unsigned char)text[1])) {
        return false;
    }
    *out = (text[0] - '0') * 10 + (text[1] - '0');
    return true;
}

static bool gps_parse_utc_time(const GNSS_Data *data, struct tm *out_tm)
{
    int hour = 0;
    int minute = 0;
    int second = 0;
    int day = 0;
    int month = 0;
    int year = 0;

    if (!data || !out_tm) {
        return false;
    }

    if (!parse_two_digits(data->timestamp, &hour) ||
        !parse_two_digits(data->timestamp + 2, &minute) ||
        !parse_two_digits(data->timestamp + 4, &second)) {
        return false;
    }

    if (!parse_two_digits(data->date, &day) ||
        !parse_two_digits(data->date + 2, &month) ||
        !parse_two_digits(data->date + 4, &year)) {
        return false;
    }

    int full_year = (year < 70) ? (2000 + year) : (1900 + year);
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        return false;
    }

    memset(out_tm, 0, sizeof(*out_tm));
    out_tm->tm_year = full_year - 1900;
    out_tm->tm_mon = month - 1;
    out_tm->tm_mday = day;
    out_tm->tm_hour = hour;
    out_tm->tm_min = minute;
    out_tm->tm_sec = second;
    out_tm->tm_isdst = 0;
    return true;
}

static bool gps_sentence_has_time(const char *line)
{
    return strstr(line, "$GNRMC") || strstr(line, "$GPRMC") ||
           strstr(line, "$GNZDA") || strstr(line, "$GPZDA");
}

static void gps_try_sync_time(const char *line, const GNSS_Data *data)
{
    if (!line || !data || !data->is_valid) {
        return;
    }
    if (!gps_sentence_has_time(line)) {
        return;
    }

    struct tm utc_tm;
    if (!gps_parse_utc_time(data, &utc_tm)) {
        return;
    }

    time_t gps_epoch = 0;
    if (!app_time_get_unix_from_utc(&utc_tm, &gps_epoch)) {
        return;
    }

    time_t now = time(NULL);
    if (now <= 0 ||
        llabs((long long)now - (long long)gps_epoch) > GPS_TIME_SYNC_THRESHOLD_SEC) {
        if (app_time_set_unix(gps_epoch, APP_TIME_SOURCE_GPS)) {
            ESP_LOGI(TAG, "GPS time synced: %lld", (long long)gps_epoch);
        }
    }
}

static void dispatch_sentence(const char *line)
{
    TickType_t now = xTaskGetTickCount();
    if (s_last_nmea_log == 0 ||
        (now - s_last_nmea_log) >= pdMS_TO_TICKS(GPS_NMEA_LOG_PERIOD_MS)) {
        if (!app_control_is_quiet()) {
            ESP_LOGI(TAG, "GPS NMEA: %s", line);
            s_last_nmea_log = now;
        }
    }

    GNSS_Data parsed = {0};
    if (gps_parser_handle_sentence(&s_parser, line, &parsed)) {
        app_state_set_gps_data(&parsed);
        gps_try_sync_time(line, &parsed);
        ESP_LOGD(TAG, "GPS updated: lat=%.5f lon=%.5f speed=%.2f",
                 parsed.latitude, parsed.longitude, parsed.speed);
    }
}

static void process_buffer(const unsigned char *data, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        char c = (char)data[i];

        if (c == '$') {
            s_line_len = 0;
        }

        if ((c == '\r' || c == '\n')) {
            if (s_line_len > 0) {
                s_line_buf[s_line_len] = '\0';
                dispatch_sentence(s_line_buf);
                s_line_len = 0;
            }
            continue;
        }

        if (s_line_len == 0 && c != '$') {
            continue;
        }

        if (s_line_len < GPS_BUF_SIZE - 1) {
            s_line_buf[s_line_len++] = c;
        } else {
            s_line_len = 0;
        }
    }
}

static void app_gps_task(void *arg)
{
    gps_parser_init(&s_parser);
    gps_init();
    // Wait for GNSS power to stabilize before UART traffic.
    vTaskDelay(pdMS_TO_TICKS(300));

    while (1) {
        if (!s_gps_enabled) {
            vTaskDelay(pdMS_TO_TICKS(GPS_TASK_PERIOD_MS));
            continue;
        }
        memset(s_read_buf, 0, sizeof(s_read_buf));
        unsigned int len = GpsReadData(s_read_buf);
        if (len > 0) {
            process_buffer(s_read_buf, len);
        }

        vTaskDelay(pdMS_TO_TICKS(GPS_TASK_PERIOD_MS));
    }
}

void app_gps_start(void)
{
    if (s_gps_task) {
        return;
    }
    s_gps_enabled = true;
    xTaskCreate(app_gps_task, "app_gps", 10240, NULL, 10, &s_gps_task);
}

void app_gps_stop(void)
{
    s_gps_enabled = false;
    gps_power_set(false);
    ESP_LOGW(TAG, "gps stop requested");
}

void app_gps_resume(void)
{
    s_gps_enabled = true;
    gps_power_set(true);
    ESP_LOGW(TAG, "gps resume");
}

bool app_gps_is_running(void)
{
    return s_gps_enabled;
}
