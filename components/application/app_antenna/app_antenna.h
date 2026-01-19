#ifndef APP_ANTENNA_H
#define APP_ANTENNA_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_ANTENNA_TIME_SOURCE_NONE = 0,
    APP_ANTENNA_TIME_SOURCE_SNTP,
    APP_ANTENNA_TIME_SOURCE_GPS,
} app_antenna_time_source_t;

void app_antenna_start(void);

void app_antenna_set_wifi_enabled(bool enabled);
void app_antenna_set_ble_enabled(bool enabled);
bool app_antenna_is_wifi_enabled(void);
bool app_antenna_is_ble_enabled(void);

void app_antenna_sntp_set_enabled(bool enabled);
bool app_antenna_sntp_is_enabled(void);

bool app_antenna_time_is_valid(void);
bool app_antenna_time_get_local(struct tm *out_tm);
void app_antenna_time_set_timezone(const char *tz);
void app_antenna_time_set_unix(time_t unix_time, app_antenna_time_source_t source);
app_antenna_time_source_t app_antenna_time_get_source(void);

uint16_t app_antenna_get_last_scan_count(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_ANTENNA_H */
