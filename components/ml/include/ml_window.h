#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int pred;
    float p_walk;
    float p_ebike;
} ml_result_t;

void ml_window_init(void);
void ml_window_push_sample_raw(
    int16_t acc_x, int16_t acc_y, int16_t acc_z,
    int16_t gyr_x, int16_t gyr_y, int16_t gyr_z,
    bool gps_valid, float speed_mps, float course_deg
);

bool ml_get_latest_result(ml_result_t *out);

#ifdef __cplusplus
}
#endif