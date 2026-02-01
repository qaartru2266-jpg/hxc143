#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

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
extern void action_on_force_stationary(lv_event_t * e);
extern void action_chongdian_tishi(lv_event_t * e);
extern void action_on_force_walk(lv_event_t * e);
extern void action_on_force_bike(lv_event_t * e);
extern void action_on_force_car(lv_event_t * e);
extern void action_on_force_bus(lv_event_t * e);
extern void action_on_force_subway(lv_event_t * e);
extern void action_on_force_cancel(lv_event_t * e);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/