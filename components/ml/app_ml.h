#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "ml_window.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int pred_raw;
    int pred_smooth;
    float confidence;
    uint8_t gps_valid;
    int64_t datetime_local_ms;
    uint64_t uptime_ms;
} app_ml_status_t;

bool app_ml_init(void);
bool app_ml_get_latest_status(app_ml_status_t *out);
void app_ml_update_status(const app_ml_status_t *st);
void app_ml_update_result(const ml_result_t *res);
void app_ml_set_infer_enabled(bool enable);
bool app_ml_is_infer_enabled(void);

// Implemented in application component; weak default in ml component.
void app_ml_task_start(void);

#ifdef __cplusplus
}
#endif
