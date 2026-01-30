#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ML_MAX_CLASSES 6

typedef struct {
    int pred;
    float probs[ML_MAX_CLASSES];
} ml_result_t;

void ml_window_init(void);
void ml_window_push_sample_raw(
    int16_t acc_x, int16_t acc_y, int16_t acc_z,
    int16_t gyr_x, int16_t gyr_y, int16_t gyr_z,
    bool gps_valid, float speed_mps, float turn_rate_deg_s
);

size_t ml_window_required_input_len(void);
bool ml_window_is_ready(void);
bool ml_window_get_input(float *out, size_t out_len);

bool ml_get_latest_result(ml_result_t *out);

#ifdef __cplusplus
}
#endif
