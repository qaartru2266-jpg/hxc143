#include "actions.h"

#include <stdbool.h>
#include <string.h>

#include "app_datalog.h"
#include "esp_log.h"
#include "screens.h"

static const char *TAG = "ui_actions";

static bool label_is_active(const char *label,
                            bool walk_on,
                            bool stationary_on,
                            bool bike_on,
                            bool car_on,
                            bool bus_on,
                            bool subway_on)
{
    if (!label) {
        return false;
    }
    if (strcmp(label, "walk") == 0) {
        return walk_on;
    }
    if (strcmp(label, "stationary") == 0) {
        return stationary_on;
    }
    if (strcmp(label, "bike") == 0) {
        return bike_on;
    }
    if (strcmp(label, "car") == 0) {
        return car_on;
    }
    if (strcmp(label, "bus") == 0) {
        return bus_on;
    }
    if (strcmp(label, "subway") == 0) {
        return subway_on;
    }
    return false;
}

static void update_datalog_state(const char *preferred_label)
{
    bool walk_on = objects.walk_data_get &&
        lv_obj_has_state(objects.walk_data_get, LV_STATE_CHECKED);
    bool stationary_on = objects.stationary_data_get &&
        lv_obj_has_state(objects.stationary_data_get, LV_STATE_CHECKED);
    // developer_page: obj0=bike, obj1=car, obj2=bus, obj3=subway
    bool bike_on = objects.obj0 && lv_obj_has_state(objects.obj0, LV_STATE_CHECKED);
    bool car_on = objects.obj1 && lv_obj_has_state(objects.obj1, LV_STATE_CHECKED);
    bool bus_on = objects.obj2 && lv_obj_has_state(objects.obj2, LV_STATE_CHECKED);
    bool subway_on = objects.obj3 && lv_obj_has_state(objects.obj3, LV_STATE_CHECKED);

    if (walk_on || stationary_on || bike_on || car_on || bus_on || subway_on) {
        const char *label = NULL;

        if (label_is_active(preferred_label,
                            walk_on,
                            stationary_on,
                            bike_on,
                            car_on,
                            bus_on,
                            subway_on)) {
            label = preferred_label;
        } else if (walk_on) {
            label = "walk";
        } else if (stationary_on) {
            label = "stationary";
        } else if (bike_on) {
            label = "bike";
        } else if (car_on) {
            label = "car";
        } else if (bus_on) {
            label = "bus";
        } else if (subway_on) {
            label = "subway";
        }

        esp_err_t err = app_datalog_start_session(label);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "datalog on: %s", label);
        } else {
            app_datalog_stop_session();
            ESP_LOGW(TAG, "datalog start failed: %s err=%d", label, err);
        }
        return;
    }

    app_datalog_stop_session();
    ESP_LOGI(TAG, "datalog off");
}

void action_on_walk_data_get(lv_event_t *e)
{
    (void)e;
    update_datalog_state("walk");
}

void action_on_stationary_data_get(lv_event_t *e)
{
    (void)e;
    update_datalog_state("stationary");
}

void action_on_bike_data_get(lv_event_t *e)
{
    (void)e;
    update_datalog_state("bike");
}

void action_on_bus_data_get(lv_event_t *e)
{
    (void)e;
    update_datalog_state("bus");
}

void action_on_car_data_get(lv_event_t *e)
{
    (void)e;
    update_datalog_state("car");
}

void action_on_subway_data_get(lv_event_t *e)
{
    (void)e;
    update_datalog_state("subway");
}
