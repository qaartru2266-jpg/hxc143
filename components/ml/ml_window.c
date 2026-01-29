#include "ml_window.h"

void ml_window_init(void)
{
}

void ml_window_push_sample_raw(
    int16_t acc_x, int16_t acc_y, int16_t acc_z,
    int16_t gyr_x, int16_t gyr_y, int16_t gyr_z,
    bool gps_valid, float speed_mps, float course_deg)
{
    (void)acc_x;
    (void)acc_y;
    (void)acc_z;
    (void)gyr_x;
    (void)gyr_y;
    (void)gyr_z;
    (void)gps_valid;
    (void)speed_mps;
    (void)course_deg;
}

bool ml_get_latest_result(ml_result_t *out)
{
    (void)out;
    return false;
}