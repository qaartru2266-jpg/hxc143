#include "ml_window.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#define ML_WINDOW_HZ 25
#define ML_WINDOW_SECONDS 6
#define ML_WINDOW_SAMPLES (ML_WINDOW_HZ * ML_WINDOW_SECONDS)
#define ML_FEATURES 8

static float s_window[ML_WINDOW_SAMPLES * ML_FEATURES];
static size_t s_write_index = 0;
static size_t s_sample_count = 0;
static portMUX_TYPE s_window_lock = portMUX_INITIALIZER_UNLOCKED;

void ml_window_init(void)
{
    portENTER_CRITICAL(&s_window_lock);
    s_write_index = 0;
    s_sample_count = 0;
    memset(s_window, 0, sizeof(s_window));
    portEXIT_CRITICAL(&s_window_lock);
}

void ml_window_push_sample_raw(
    int16_t acc_x, int16_t acc_y, int16_t acc_z,
    int16_t gyr_x, int16_t gyr_y, int16_t gyr_z,
    bool gps_valid, float speed_mps, float turn_rate_deg_s)
{
    float speed = gps_valid ? speed_mps : 0.0f;
    float turn_rate = gps_valid ? turn_rate_deg_s : 0.0f;

    portENTER_CRITICAL(&s_window_lock);
    size_t base = s_write_index * ML_FEATURES;
    s_window[base + 0] = (float)acc_x;
    s_window[base + 1] = (float)acc_y;
    s_window[base + 2] = (float)acc_z;
    s_window[base + 3] = (float)gyr_x;
    s_window[base + 4] = (float)gyr_y;
    s_window[base + 5] = (float)gyr_z;
    s_window[base + 6] = speed;
    s_window[base + 7] = turn_rate;

    s_write_index = (s_write_index + 1) % ML_WINDOW_SAMPLES;
    if (s_sample_count < ML_WINDOW_SAMPLES) {
        s_sample_count++;
    }
    portEXIT_CRITICAL(&s_window_lock);
}

size_t ml_window_required_input_len(void)
{
    return ML_WINDOW_SAMPLES * ML_FEATURES;
}

bool ml_window_is_ready(void)
{
    bool ready = false;
    portENTER_CRITICAL(&s_window_lock);
    ready = (s_sample_count >= ML_WINDOW_SAMPLES);
    portEXIT_CRITICAL(&s_window_lock);
    return ready;
}

bool ml_window_get_input(float *out, size_t out_len)
{
    if (!out || out_len < ml_window_required_input_len()) {
        return false;
    }

    portENTER_CRITICAL(&s_window_lock);
    if (s_sample_count < ML_WINDOW_SAMPLES) {
        portEXIT_CRITICAL(&s_window_lock);
        return false;
    }

    size_t out_idx = 0;
    for (size_t i = 0; i < ML_WINDOW_SAMPLES; ++i) {
        size_t idx = ((s_write_index + i) % ML_WINDOW_SAMPLES) * ML_FEATURES;
        memcpy(&out[out_idx], &s_window[idx], ML_FEATURES * sizeof(float));
        out_idx += ML_FEATURES;
    }
    portEXIT_CRITICAL(&s_window_lock);
    return true;
}
