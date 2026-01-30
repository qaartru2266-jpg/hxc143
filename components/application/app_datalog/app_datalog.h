#ifndef APP_DATALOG_H
#define APP_DATALOG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_DATALOG_MODE_MAX_LEN 16

typedef struct {
    int64_t datetime_local_ms;
    uint64_t uptime_ms;
    float acc_x;
    float acc_y;
    float acc_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float speed_mps;
    float turn_rate_deg_s;
    uint8_t gps_valid;
} DatalogRaw_t;

typedef struct {
    char start_time[9];
    char end_time[9];
    int64_t start_uptime_ms;
    int64_t end_uptime_ms;
    float duration_sec;
    char mode[APP_DATALOG_MODE_MAX_LEN];
    float avg_speed_mps;
} DatalogEvent_t;

typedef struct {
    char mode[APP_DATALOG_MODE_MAX_LEN];
    float total_duration_min;
    float carbon_factor_g_per_min;
    float co2_g;
} DatalogSummary_t;

typedef struct {
    int64_t datetime_local_ms;
    uint64_t uptime_ms;
    int pred_raw;
    int pred_smooth;
    float confidence;
    uint8_t gps_valid;
} DatalogPred_t;

esp_err_t app_datalog_start(void);
void app_datalog_enqueue_raw(const DatalogRaw_t *raw_data);
void app_datalog_log_event(const DatalogEvent_t *event);
void app_datalog_save_summary(const DatalogSummary_t *summary);
esp_err_t app_datalog_save_summary_batch(const DatalogSummary_t *rows, size_t count);
esp_err_t app_datalog_log_pred(const DatalogPred_t *pred);
esp_err_t app_datalog_start_session(const char *label);
void app_datalog_stop_session(void);
void app_datalog_set_default_raw_enabled(bool enable);
bool app_datalog_is_default_raw_enabled(void);
void app_datalog_stop(void);
void app_datalog_resume(void);
bool app_datalog_is_running(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_DATALOG_H */
