#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TRAFFIC_MODE_UNKNOWN = 0,
    TRAFFIC_MODE_STATIONARY,
    TRAFFIC_MODE_WALK,
    TRAFFIC_MODE_BIKE_EBIKE,
    TRAFFIC_MODE_CAR,
    TRAFFIC_MODE_BUS,
    TRAFFIC_MODE_METRO,
    TRAFFIC_MODE_COUNT
} TrafficMode_t;

typedef struct {
    TrafficMode_t window[5];
    uint8_t window_count;
    uint8_t window_index;
    uint8_t mode_counts[TRAFFIC_MODE_COUNT];

    TrafficMode_t instant_mode;
    TrafficMode_t stable_mode;
    bool pending_active;
    TrafficMode_t pending_mode;
    int64_t pending_start_ms;
    int64_t segment_start_ms;
    int64_t last_input_ms;
    int64_t last_summary_ms;

    float duration_sec[TRAFFIC_MODE_COUNT];
} TrafficStateMachine;

const char *traffic_mode_to_str(TrafficMode_t mode);

void tsm_init(TrafficStateMachine *tsm);
void tsm_input(TrafficStateMachine *tsm, TrafficMode_t mode, float confidence, int64_t uptime_ms);
void tsm_flush(TrafficStateMachine *tsm, int64_t uptime_ms);

void tsm_input_mode(TrafficMode_t mode, float confidence, int64_t uptime_ms);
void app_logic_start(void);
void app_logic_flush(int64_t uptime_ms);

#ifdef __cplusplus
}
#endif

#endif /* APP_LOGIC_H */
