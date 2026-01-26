#include "app_logic.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_datalog.h"

#define TAG "app_logic"

#define TSM_WINDOW_SIZE 5
#define TSM_INSTANT_THRESHOLD 4
#define TSM_MIN_HOLD_MS 10000
#define TSM_SUMMARY_INTERVAL_MS 60000

#define MOCK_TASK_STACK 4096
#define MOCK_TASK_PRIO 4
#define MOCK_PERIOD_MS 100
#define MOCK_TOTAL_MS 240000
#define MOCK_STAGE1_MS 60000
#define MOCK_STAGE2_MS 180000

static const float k_carbon_factor_g_per_min[TRAFFIC_MODE_COUNT] = {
    [TRAFFIC_MODE_UNKNOWN] = 0.0f,
    [TRAFFIC_MODE_STATIONARY] = 0.0f,
    [TRAFFIC_MODE_WALK] = 0.0f,
    [TRAFFIC_MODE_BIKE_EBIKE] = 1.2f,
    [TRAFFIC_MODE_CAR] = 12.0f,
    [TRAFFIC_MODE_BUS] = 6.5f,
    [TRAFFIC_MODE_METRO] = 4.2f,
};

static const TrafficMode_t k_summary_modes[] = {
    TRAFFIC_MODE_STATIONARY,
    TRAFFIC_MODE_WALK,
    TRAFFIC_MODE_BIKE_EBIKE,
    TRAFFIC_MODE_CAR,
    TRAFFIC_MODE_BUS,
    TRAFFIC_MODE_METRO,
    TRAFFIC_MODE_UNKNOWN,
};

static TrafficStateMachine s_tsm;
static TaskHandle_t s_mock_task = NULL;

const char *traffic_mode_to_str(TrafficMode_t mode)
{
    switch (mode) {
    case TRAFFIC_MODE_STATIONARY:
        return "STATIONARY";
    case TRAFFIC_MODE_WALK:
        return "WALK";
    case TRAFFIC_MODE_BIKE_EBIKE:
        return "BIKE_EBIKE";
    case TRAFFIC_MODE_CAR:
        return "CAR";
    case TRAFFIC_MODE_BUS:
        return "BUS";
    case TRAFFIC_MODE_METRO:
        return "METRO";
    case TRAFFIC_MODE_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}

static void tsm_reset_window(TrafficStateMachine *tsm)
{
    memset(tsm->window, 0, sizeof(tsm->window));
    memset(tsm->mode_counts, 0, sizeof(tsm->mode_counts));
    tsm->window_count = 0;
    tsm->window_index = 0;
}

void tsm_init(TrafficStateMachine *tsm)
{
    if (!tsm) {
        return;
    }
    memset(tsm, 0, sizeof(*tsm));
    tsm->instant_mode = TRAFFIC_MODE_UNKNOWN;
    tsm->stable_mode = TRAFFIC_MODE_UNKNOWN;
    tsm->pending_mode = TRAFFIC_MODE_UNKNOWN;
    tsm_reset_window(tsm);
}

static void tsm_update_window(TrafficStateMachine *tsm, TrafficMode_t mode)
{
    if (tsm->window_count == TSM_WINDOW_SIZE) {
        TrafficMode_t old = tsm->window[tsm->window_index];
        if (old < TRAFFIC_MODE_COUNT && tsm->mode_counts[old] > 0) {
            tsm->mode_counts[old]--;
        }
    } else {
        tsm->window_count++;
    }

    tsm->window[tsm->window_index] = mode;
    if (mode < TRAFFIC_MODE_COUNT) {
        tsm->mode_counts[mode]++;
    }
    tsm->window_index = (uint8_t)((tsm->window_index + 1) % TSM_WINDOW_SIZE);
}

static bool tsm_get_majority(const TrafficStateMachine *tsm, TrafficMode_t *out_mode)
{
    if (!tsm || !out_mode) {
        return false;
    }

    for (int i = 0; i < TRAFFIC_MODE_COUNT; ++i) {
        if (tsm->mode_counts[i] >= TSM_INSTANT_THRESHOLD) {
            *out_mode = (TrafficMode_t)i;
            return true;
        }
    }
    return false;
}

static void tsm_write_summary(TrafficStateMachine *tsm, int64_t uptime_ms)
{
    if (!tsm) {
        return;
    }

    float durations[TRAFFIC_MODE_COUNT];
    for (int i = 0; i < TRAFFIC_MODE_COUNT; ++i) {
        durations[i] = tsm->duration_sec[i];
    }

    if (tsm->stable_mode != TRAFFIC_MODE_UNKNOWN && tsm->segment_start_ms > 0 && uptime_ms >= tsm->segment_start_ms) {
        float ongoing = (float)(uptime_ms - tsm->segment_start_ms) / 1000.0f;
        durations[tsm->stable_mode] += ongoing;
    }

    DatalogSummary_t rows[TRAFFIC_MODE_COUNT];
    size_t count = 0;
    for (size_t i = 0; i < sizeof(k_summary_modes) / sizeof(k_summary_modes[0]); ++i) {
        TrafficMode_t mode = k_summary_modes[i];
        if (mode >= TRAFFIC_MODE_COUNT) {
            continue;
        }
        float minutes = durations[mode] / 60.0f;
        snprintf(rows[count].mode, sizeof(rows[count].mode), "%s", traffic_mode_to_str(mode));
        rows[count].total_duration_min = minutes;
        rows[count].carbon_factor_g_per_min = k_carbon_factor_g_per_min[mode];
        rows[count].co2_g = minutes * rows[count].carbon_factor_g_per_min;
        count++;
    }

    (void)app_datalog_save_summary_batch(rows, count);
    tsm->last_summary_ms = uptime_ms;
}

void tsm_input(TrafficStateMachine *tsm, TrafficMode_t mode, float confidence, int64_t uptime_ms)
{
    (void)confidence;
    if (!tsm) {
        return;
    }
    if (mode >= TRAFFIC_MODE_COUNT) {
        mode = TRAFFIC_MODE_UNKNOWN;
    }

    if (tsm->last_input_ms > 0 && uptime_ms < tsm->last_input_ms) {
        ESP_LOGW(TAG, "TSM: uptime rewind, reset state");
        tsm_init(tsm);
    }
    tsm->last_input_ms = uptime_ms;

    tsm_update_window(tsm, mode);

    TrafficMode_t candidate = tsm->instant_mode;
    bool have_majority = tsm_get_majority(tsm, &candidate);
    if (have_majority) {
        if (candidate != tsm->instant_mode) {
            tsm->instant_mode = candidate;
        }
    } else if (mode != tsm->instant_mode && mode != TRAFFIC_MODE_UNKNOWN) {
        ESP_LOGI(TAG, "TSM: Noise Filtered (Ignored %s input)", traffic_mode_to_str(mode));
    }

    if (tsm->stable_mode == TRAFFIC_MODE_UNKNOWN) {
        if (tsm->instant_mode != TRAFFIC_MODE_UNKNOWN && have_majority) {
            tsm->stable_mode = tsm->instant_mode;
            tsm->segment_start_ms = uptime_ms;
            tsm->pending_active = false;
            ESP_LOGI(TAG, "TSM: Initial stable -> %s", traffic_mode_to_str(tsm->stable_mode));
        }
    } else if (tsm->instant_mode != TRAFFIC_MODE_UNKNOWN) {
        if (tsm->instant_mode == tsm->stable_mode) {
            if (tsm->pending_active) {
                tsm->pending_active = false;
                ESP_LOGI(TAG, "TSM: Pending cancelled");
            }
        } else {
            if (!tsm->pending_active || tsm->pending_mode != tsm->instant_mode) {
                tsm->pending_active = true;
                tsm->pending_mode = tsm->instant_mode;
                tsm->pending_start_ms = uptime_ms;
                ESP_LOGI(TAG, "TSM: Entering Pending State (Target: %s)...",
                         traffic_mode_to_str(tsm->pending_mode));
            } else if ((uptime_ms - tsm->pending_start_ms) >= TSM_MIN_HOLD_MS) {
                float duration_sec = 0.0f;
                if (uptime_ms >= tsm->segment_start_ms) {
                    duration_sec = (float)(uptime_ms - tsm->segment_start_ms) / 1000.0f;
                }
                tsm->duration_sec[tsm->stable_mode] += duration_sec;

                DatalogEvent_t event = {0};
                snprintf(event.start_time, sizeof(event.start_time), "NA");
                snprintf(event.end_time, sizeof(event.end_time), "NA");
                event.start_uptime_ms = tsm->segment_start_ms;
                event.end_uptime_ms = uptime_ms;
                event.duration_sec = duration_sec;
                snprintf(event.mode, sizeof(event.mode), "%s", traffic_mode_to_str(tsm->stable_mode));
                event.avg_speed_mps = 0.0f;
                app_datalog_log_event(&event);

                ESP_LOGI(TAG, "TSM: SWITCH CONFIRMED! [%s] -> [%s] (Duration: %.1fs)",
                         traffic_mode_to_str(tsm->stable_mode),
                         traffic_mode_to_str(tsm->instant_mode),
                         duration_sec);

                tsm->stable_mode = tsm->instant_mode;
                tsm->segment_start_ms = uptime_ms;
                tsm->pending_active = false;
            }
        }
    }

    if (tsm->last_summary_ms == 0) {
        tsm->last_summary_ms = uptime_ms;
    } else if ((uptime_ms - tsm->last_summary_ms) >= TSM_SUMMARY_INTERVAL_MS) {
        tsm_write_summary(tsm, uptime_ms);
    }
}

void tsm_flush(TrafficStateMachine *tsm, int64_t uptime_ms)
{
    tsm_write_summary(tsm, uptime_ms);
}

void tsm_input_mode(TrafficMode_t mode, float confidence, int64_t uptime_ms)
{
    tsm_input(&s_tsm, mode, confidence, uptime_ms);
}

void app_logic_flush(int64_t uptime_ms)
{
    tsm_flush(&s_tsm, uptime_ms);
}

static void app_logic_mock_task(void *arg)
{
    (void)arg;
    int64_t start_ms = esp_timer_get_time() / 1000;
    TickType_t last_wake = xTaskGetTickCount();
    int64_t last_bucket = -1;
    int noise_remaining = 0;

    while (1) {
        int64_t now_ms = esp_timer_get_time() / 1000;
        int64_t elapsed_ms = now_ms - start_ms;
        if (elapsed_ms >= MOCK_TOTAL_MS) {
            break;
        }

        TrafficMode_t mode = TRAFFIC_MODE_STATIONARY;
        if (elapsed_ms < MOCK_STAGE1_MS) {
            int64_t bucket = elapsed_ms / 10000;
            if (bucket != last_bucket) {
                last_bucket = bucket;
                noise_remaining = 1 + (esp_random() % 2);
            }
            if (noise_remaining > 0) {
                mode = TRAFFIC_MODE_CAR;
                noise_remaining--;
            }
        } else if (elapsed_ms < MOCK_STAGE2_MS) {
            mode = TRAFFIC_MODE_WALK;
        } else {
            mode = TRAFFIC_MODE_BUS;
        }

        tsm_input_mode(mode, 0.9f, now_ms);
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(MOCK_PERIOD_MS));
    }

    app_logic_flush(esp_timer_get_time() / 1000);
    ESP_LOGI(TAG, "MOCK: done");
    vTaskDelete(NULL);
}

void app_logic_start(void)
{
    static bool started = false;
    if (started) {
        return;
    }
    started = true;

    tsm_init(&s_tsm);
    if (!s_mock_task) {
        xTaskCreate(app_logic_mock_task, "mock_scenario", MOCK_TASK_STACK, NULL, MOCK_TASK_PRIO, &s_mock_task);
    }
}
