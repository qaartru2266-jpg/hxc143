#include "app_ml.h"
#include "ml_runner.h"
#include "ml_window.h"

#include <math.h>
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
#define ML_SWITCH_CONFIRM 3
#define ML_INPUT_LEN (25 * 6 * 8)

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

static int smooth_update(int pred_raw)
{
    static int recent[ML_SMOOTH_WIN] = {0};
    static size_t idx = 0;
    static size_t count = 0;
    static int smooth = -1;
    static int candidate = -1;
    static int candidate_count = 0;

    recent[idx] = pred_raw;
    idx = (idx + 1) % ML_SMOOTH_WIN;
    if (count < ML_SMOOTH_WIN) {
        count++;
    }

    int count0 = 0;
    int count1 = 0;
    for (size_t i = 0; i < count; ++i) {
        if (recent[i] == 0) {
            count0++;
        } else {
            count1++;
        }
    }
    int majority = (count1 > count0) ? 1 : 0;

    if (smooth < 0) {
        smooth = majority;
        candidate = majority;
        candidate_count = 0;
        return smooth;
    }

    if (majority == smooth) {
        candidate = majority;
        candidate_count = 0;
        return smooth;
    }

    if (majority != candidate) {
        candidate = majority;
        candidate_count = 1;
    } else {
        candidate_count++;
    }

    if (candidate_count >= ML_SWITCH_CONFIRM) {
        smooth = candidate;
        candidate_count = 0;
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

    while (1) {
        app_state_imu_sample_t imu;
        if (app_state_get_latest_imu(&imu)) {
            if (imu.timestamp_us != last_imu_ts) {
                last_imu_ts = imu.timestamp_us;

                GNSS_Data gps;
                bool have_gps = app_state_get_latest_gps(&gps);
                bool gps_valid = have_gps && (gps.is_valid == 1);
                float speed = gps_valid ? gps.speed : 0.0f;
                float turn_rate = 0.0f;
                uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);

                if (gps_valid) {
                    if (have_course) {
                        float delta = gps.course - last_course;
                        if (delta > 180.0f) delta -= 360.0f;
                        if (delta < -180.0f) delta += 360.0f;
                        float dt = (now_ms - last_course_ms) / 1000.0f;
                        if (dt > 0.05f) {
                            turn_rate = delta / dt;
                        }
                    }
                    last_course = gps.course;
                    last_course_ms = now_ms;
                    have_course = true;
                }

                ml_window_push_sample_raw(
                    imu.acc_x, imu.acc_y, imu.acc_z,
                    imu.gyr_x, imu.gyr_y, imu.gyr_z,
                    gps_valid, speed, turn_rate
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
                    int pred_smooth = smooth_update(pred_raw);
                    float conf = 0.0f;
                    if (result.pred >= 0 && result.pred < ML_MAX_CLASSES) {
                        conf = result.probs[result.pred];
                    }

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
                        ESP_LOGI(TAG, "pred_smooth=%d conf=%.3f gps=%u",
                                 st.pred_smooth, st.confidence, st.gps_valid);
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
