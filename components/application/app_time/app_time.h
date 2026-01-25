#ifndef APP_TIME_H
#define APP_TIME_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_TIME_SOURCE_NONE = 0,
    APP_TIME_SOURCE_NVS,
    APP_TIME_SOURCE_SNTP,
    APP_TIME_SOURCE_GPS,
} app_time_source_t;

void app_time_start(void);
void app_time_set_timezone(const char *tz);
bool app_time_set_unix(time_t unix_time, app_time_source_t source);
bool app_time_set_utc(const struct tm *utc_time, app_time_source_t source);
bool app_time_get_unix_from_utc(const struct tm *utc_time, time_t *out_unix);
void app_time_save_now(void);
bool app_time_is_valid(void);
app_time_source_t app_time_get_source(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_TIME_H */
