#pragma once
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Start power/key management tasks.
esp_err_t app_power_start(void);

// Notify power manager about touch state; returns true if this touch should be consumed.
bool app_power_on_touch(bool pressed);

#ifdef __cplusplus
}
#endif
