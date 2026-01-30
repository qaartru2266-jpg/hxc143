#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations



// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_CURRENT_TIME = 0,
    FLOW_GLOBAL_VARIABLE_BATTERY_LEVEL = 1,
    FLOW_GLOBAL_VARIABLE_GESTURE_DIR = 2,
    FLOW_GLOBAL_VARIABLE_CURRENT_DATE = 3,
    FLOW_GLOBAL_VARIABLE_BRIGHTNESS_LEVEL = 4,
    FLOW_GLOBAL_VARIABLE_MODE_LVGL = 5
};

// Native global variables



#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/