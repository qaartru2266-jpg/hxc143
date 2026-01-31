#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <stdbool.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_on_walk_data_get(lv_event_t * e);
extern void action_on_stationary_data_get(lv_event_t * e);
extern void action_on_bike_data_get(lv_event_t * e);
extern void action_on_bus_data_get(lv_event_t * e);
extern void action_on_car_data_get(lv_event_t * e);
extern void action_on_subway_data_get(lv_event_t * e);
extern void action_on_wifi(lv_event_t * e);
extern void ui_set_wifi_toggle(bool enabled);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/
