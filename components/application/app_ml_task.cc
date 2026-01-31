#include "app_ml.h"
#include "ml_runner.h"
#include "ml_window.h"

#include <math.h>
#include <string.h>
#include <sys/time.h>

#include "app_state.h"
#include "app_time.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "app_ml_task";

#define ML_TASK_STACK 6144
#define ML_TASK_PRIO 6
#define ML_SAMPLE_INTERVAL_MS 40
#define ML_INFER_EVERY_SAMPLES 25
#define ML_SMOOTH_WIN 5
#define ML_VOTE_THRESHOLD 3
#define ML_UNKNOWN_CLASS 255
#define ML_CONF_MIN 0.6f
#define ML_MARGIN_MIN 0.15f
#define ML_INPUT_LEN (25 * 6 * 8)
#define GPS_HOLD_MS 1000

static TaskHandle_t s_ml_task = NULL;
static float s_input_buf[ML_INPUT_LEN];

static bool ml_get_epoch_ms(int64_t *out_ms)
{
    if (!out_ms) {
        return false;
    }
    if (!app_time_is_valid()) {
        *out_ms = 0;
        return false;
    }
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        *out_ms = 0;
        return false;
    }
    *out_ms = (int64_t)tv.tv_sec * 1000 + (tv.tv_usec / 1000);
    return true;
}

static int smooth_update(int pred_raw, float confidence, float margin, bool *is_unknown_out)
{
    static int recent[ML_SMOOTH_WIN] = {0};
    static size_t idx = 0;
    static size_t count = 0;
    static int smooth = ML_UNKNOWN_CLASS;

    bool is_unknown = (pred_raw < 0 || pred_raw >= ML_MAX_CLASSES ||
                       confidence < ML_CONF_MIN || margin < ML_MARGIN_MIN);
    if (is_unknown_out) {
        *is_unknown_out = is_unknown;
    }
    if (is_unknown) {
        return smooth;
    }

    recent[idx] = pred_raw;
    idx = (idx + 1) % ML_SMOOTH_WIN;
    if (count < ML_SMOOTH_WIN) {
        count++;
    }

    int counts[ML_MAX_CLASSES] = {0};
    for (size_t i = 0; i < count; ++i) {
        int v = recent[i];
        if (v >= 0 && v < ML_MAX_CLASSES) {
            counts[v]++;
        }
    }

    for (int i = 0; i < ML_MAX_CLASSES; ++i) {
        if (counts[i] >= ML_VOTE_THRESHOLD) {
            smooth = i;
            break;
        }
    }

    return smooth;
}

static void ml_task(void *arg)
{
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();
    int64_t last_imu_ts = -1;
    uint32_t samples_since_infer = 0;
    int64_t last_log_us = 0;

    float last_course = 0.0f;
    uint64_t last_course_ms = 0;
    bool have_course = false;
    float last_speed = 0.0f;
    float last_turn_rate = 0.0f;
    uint64_t last_gps_valid_ms = 0;
    uint64_t last_gps_update_ms = 0;
    char last_gps_time[sizeof(((GNSS_Data *)0)->timestamp)] = {0};

    while (1) {
        app_state_imu_sample_t imu;
        if (app_state_get_latest_imu(&imu)) {
            if (imu.timestamp_us != last_imu_ts) {
                last_imu_ts = imu.timestamp_us;

                GNSS_Data gps;
                bool have_gps = app_state_get_latest_gps(&gps);
                uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);

                bool gps_valid_raw = have_gps && (gps.is_valid == 1);
                bool gps_time_changed = false;
                if (gps_valid_raw && gps.timestamp[0]) {
                    if (strncmp(gps.timestamp, last_gps_time, sizeof(last_gps_time)) != 0) {
                        strncpy(last_gps_time, gps.timestamp, sizeof(last_gps_time) - 1);
                        last_gps_time[sizeof(last_gps_time) - 1] = '\0';
                        gps_time_changed = true;
                    }
                }

                bool gps_update = false;
                if (gps_valid_raw) {
                    if (gps_time_changed) {
                        gps_update = true;
                    } else if (last_gps_update_ms == 0 ||
                               (now_ms - last_gps_update_ms) >= GPS_HOLD_MS) {
                        gps_update = true;
                    }
                }

                if (gps_update) {
                    if (have_course) {
                        float delta = gps.course - last_course;
                        if (delta > 180.0f) {
                            delta -= 360.0f;
                        } else if (delta < -180.0f) {
                            delta += 360.0f;
                        }
                        float dt = (now_ms - last_course_ms) / 1000.0f;
                        if (dt > 0.05f) {
                            last_turn_rate = delta / dt;
                        } else {
                            last_turn_rate = 0.0f;
                        }
                    } else {
                        last_turn_rate = 0.0f;
                    }
                    last_course = gps.course;
                    last_course_ms = now_ms;
                    have_course = true;
                    last_speed = gps.speed;
                    last_gps_valid_ms = now_ms;
                    last_gps_update_ms = now_ms;
                }

                bool gps_valid_hold = (last_gps_valid_ms != 0 &&
                                       (now_ms - last_gps_valid_ms) <= GPS_HOLD_MS);
                bool gps_valid_for_ml = gps_valid_hold;
                float speed = gps_valid_for_ml ? last_speed : 0.0f;
                float turn_rate = gps_valid_for_ml ? last_turn_rate : 0.0f;

                ml_window_push_sample_raw(
                    imu.acc_x, imu.acc_y, imu.acc_z,
                    imu.gyr_x, imu.gyr_y, imu.gyr_z,
                    gps_valid_for_ml, speed, turn_rate
                );

                samples_since_infer++;
            }
        }

        if (ml_window_is_ready() && samples_since_infer >= ML_INFER_EVERY_SAMPLES) {
            samples_since_infer = 0;
            size_t need = ml_window_required_input_len();
            if (need <= ML_INPUT_LEN && ml_window_get_input(s_input_buf, need)) {
                ml_result_t result;
                if (ml_run_inference(s_input_buf, need, &result)) {
                    app_ml_update_result(&result);

                    int pred_raw = result.pred;
                    float conf = 0.0f;
                    float margin = 0.0f;
                    if (pred_raw >= 0 && pred_raw < ML_MAX_CLASSES) {
                        conf = result.probs[pred_raw];
                    }
                    float top1 = -1.0f;
                    float top2 = -1.0f;
                    for (int i = 0; i < ML_MAX_CLASSES; ++i) {
                        float v = result.probs[i];
                        if (v > top1) {
                            top2 = top1;
                            top1 = v;
                        } else if (v > top2) {
                            top2 = v;
                        }
                    }
                    if (top1 >= 0.0f && top2 >= 0.0f) {
                        margin = top1 - top2;
                    }

                    bool is_unknown = false;
                    int pred_smooth = smooth_update(pred_raw, conf, margin, &is_unknown);

                    app_ml_status_t st = {
                        .pred_raw = pred_raw,
                        .pred_smooth = pred_smooth,
                        .confidence = conf,
                        .gps_valid = 0,
                        .datetime_local_ms = 0,
                        .uptime_ms = (uint64_t)(esp_timer_get_time() / 1000ULL)
                    };

                    int64_t epoch_ms = 0;
                    if (ml_get_epoch_ms(&epoch_ms)) {
                        st.datetime_local_ms = epoch_ms;
                    }

                    GNSS_Data gps;
                    if (app_state_get_latest_gps(&gps)) {
                        st.gps_valid = (gps.is_valid == 1) ? 1 : 0;
                    }

                    app_ml_update_status(&st);

                    int64_t now_us = esp_timer_get_time();
                    if (now_us - last_log_us > 5000000LL) {
                        ESP_LOGI(TAG, "pred_raw=%d pred_smooth=%d conf=%.3f margin=%.3f unknown=%u",
                                 st.pred_raw, st.pred_smooth, st.confidence, margin, is_unknown ? 1U : 0U);
                        last_log_us = now_us;
                    }
                }
            }
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(ML_SAMPLE_INTERVAL_MS));
    }
}

extern "C" void app_ml_task_start(void)
{
    if (s_ml_task) {
        return;
    }

    ml_window_init();

    if (xTaskCreate(ml_task, "ml_task", ML_TASK_STACK, NULL, ML_TASK_PRIO, &s_ml_task) != pdPASS) {
        ESP_LOGE(TAG, "ml task create failed");
        s_ml_task = NULL;
    } else {
        ESP_LOGI(TAG, "ml task started");
    }
}
