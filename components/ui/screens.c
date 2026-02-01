#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;

static void event_handler_cb_main_page_main_page(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_GESTURE) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 1, 0, e);
    }
}

static void event_handler_cb_main_page_obj7(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_arc_get_value(ta);
            assignIntegerProperty(flowState, 4, 3, value, "Failed to assign Value in Arc widget");
        }
    }
}

static void event_handler_cb_main_page_bingshan(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 7, 0, e);
    }
}

static void event_handler_cb_main_page_mode(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_LONG_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 11, 0, e);
    }
}

static void event_handler_cb_control_page_control_page(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_GESTURE) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 1, 0, e);
    }
}

static void event_handler_cb_control_page_obj10(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_slider_get_value(ta);
            assignIntegerProperty(flowState, 0, 3, value, "Failed to assign Value in Slider widget");
        }
    }
}

static void event_handler_cb_control_page_wifi(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        e->user_data = (void *)0;
        action_on_wifi(e);
    }
}

static void event_handler_cb_select_page_select_page(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 1, 0, e);
    }
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        action_on_force_stationary(e);
    }
}

static void event_handler_cb_select_page_obj0(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        action_on_force_bike(e);
    }
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 0, 0, e);
    }
}

static void event_handler_cb_select_page_obj1(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        action_on_force_bus(e);
    }
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 2, 0, e);
    }
}

static void event_handler_cb_select_page_obj2(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        action_on_force_walk(e);
    }
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 3, 0, e);
    }
}

static void event_handler_cb_select_page_obj3(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        action_on_car_data_get(e);
    }
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 4, 0, e);
    }
}

static void event_handler_cb_select_page_obj4(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        action_on_force_subway(e);
    }
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 5, 0, e);
    }
}

static void event_handler_cb_select_page_obj5(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        action_on_force_stationary(e);
    }
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 6, 0, e);
    }
}

static void event_handler_cb_select_page_obj6(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 7, 0, e);
    }
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        action_on_force_cancel(e);
    }
}

static void event_handler_cb_about_page_about_page(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_GESTURE) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 1, 0, e);
    }
}

static void event_handler_cb_fish_page_fish_page(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_GESTURE) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 1, 0, e);
    }
}

static void event_handler_cb_fish_page_bingshan_1(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 22, 0, e);
    }
}

static void event_handler_cb_developer_page_walk_data_get(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        e->user_data = (void *)0;
        action_on_walk_data_get(e);
    }
}

static void event_handler_cb_developer_page_stationary_data_get(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        e->user_data = (void *)0;
        action_on_stationary_data_get(e);
    }
}

static void event_handler_cb_developer_page_bike_data_get(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        e->user_data = (void *)0;
        action_on_bike_data_get(e);
    }
}

static void event_handler_cb_developer_page_car_data_get(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        e->user_data = (void *)0;
        action_on_car_data_get(e);
    }
}

static void event_handler_cb_developer_page_bus_data_get(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        e->user_data = (void *)0;
        action_on_bus_data_get(e);
    }
}

static void event_handler_cb_developer_page_subway_data_get(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        e->user_data = (void *)0;
        action_on_subway_data_get(e);
    }
}

void create_screen_main_page() {
    void *flowState = getFlowState(0, 0);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.main_page = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 466, 466);
    lv_obj_add_event_cb(obj, event_handler_cb_main_page_main_page, LV_EVENT_ALL, flowState);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ONE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE|LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // current_time
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.current_time = obj;
            lv_obj_set_pos(obj, 89, 171);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLL_ONE);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_ON);
            lv_obj_set_scroll_dir(obj, LV_DIR_ALL);
            lv_obj_set_scroll_snap_x(obj, LV_SCROLL_SNAP_START);
            lv_obj_set_scroll_snap_y(obj, LV_SCROLL_SNAP_START);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_letter_space(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_SCROLLED);
            lv_label_set_text(obj, "");
        }
        {
            // bar_battery
            lv_obj_t *obj = lv_bar_create(parent_obj);
            objects.bar_battery = obj;
            lv_obj_set_pos(obj, 214, 66);
            lv_obj_set_size(obj, 33, 20);
            lv_bar_set_range(obj, 1, 100);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_ONE);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_ON);
            lv_obj_set_scroll_dir(obj, LV_DIR_ALL);
            lv_obj_set_scroll_snap_x(obj, LV_SCROLL_SNAP_START);
            lv_obj_set_scroll_snap_y(obj, LV_SCROLL_SNAP_START);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_outline_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff2196f3), LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // current_date
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.current_date = obj;
            lv_obj_set_pos(obj, 89, 241);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLL_ONE);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_ON);
            lv_obj_set_scroll_dir(obj, LV_DIR_ALL);
            lv_obj_set_scroll_snap_x(obj, LV_SCROLL_SNAP_START);
            lv_obj_set_scroll_snap_y(obj, LV_SCROLL_SNAP_START);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_letter_space(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.obj7 = obj;
            lv_obj_set_pos(obj, 16, 16);
            lv_obj_set_size(obj, 435, 435);
            lv_obj_add_event_cb(obj, event_handler_cb_main_page_obj7, LV_EVENT_ALL, flowState);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        }
        {
            // label_battery
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_battery = obj;
            lv_obj_set_pos(obj, 124, 99);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj8 = obj;
            lv_obj_set_pos(obj, 130, 76);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
        }
        {
            // bingshan
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.bingshan = obj;
            lv_obj_set_pos(obj, 108, 361);
            lv_obj_set_size(obj, 245, 119);
            lv_obj_add_event_cb(obj, event_handler_cb_main_page_bingshan, LV_EVENT_ALL, flowState);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff335dca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 206, 60);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_dianchi);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 291, 68);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
        }
        {
            // mode
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.mode = obj;
            lv_obj_set_pos(obj, 247, 147);
            lv_obj_set_size(obj, 174, 150);
            lv_obj_add_event_cb(obj, event_handler_cb_main_page_mode, LV_EVENT_ALL, flowState);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff335dca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
        {
            // qingchongdian
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.qingchongdian = obj;
            lv_obj_set_pos(obj, 201, 104);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_alibaba_pu_hui_ti_3_55_regular, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "请充电");
        }
        {
            // ic_stationary
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.ic_stationary = obj;
            lv_obj_set_pos(obj, 270, 155);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_stationary);
        }
        {
            // Walk
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.walk = obj;
            lv_obj_set_pos(obj, 270, 155);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_walk);
        }
        {
            // Bike
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.bike = obj;
            lv_obj_set_pos(obj, 270, 155);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_bike);
        }
        {
            // Car
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.car = obj;
            lv_obj_set_pos(obj, 270, 155);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_car);
        }
        {
            // Bus
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.bus = obj;
            lv_obj_set_pos(obj, 270, 155);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_bus);
        }
        {
            // Subway
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.subway = obj;
            lv_obj_set_pos(obj, 270, 155);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_subway);
        }
        {
            // Unknown
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.unknown = obj;
            lv_obj_set_pos(obj, 270, 155);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 111, 349);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_bingshan);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.obj9 = obj;
            lv_obj_set_pos(obj, 97, 300);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_bingshangao);
        }
    }
    
    tick_screen_main_page();
}

void delete_screen_main_page() {
    lv_obj_delete(objects.main_page);
    objects.main_page = 0;
    objects.current_time = 0;
    objects.bar_battery = 0;
    objects.current_date = 0;
    objects.obj7 = 0;
    objects.label_battery = 0;
    objects.obj8 = 0;
    objects.bingshan = 0;
    objects.mode = 0;
    objects.qingchongdian = 0;
    objects.ic_stationary = 0;
    objects.walk = 0;
    objects.bike = 0;
    objects.car = 0;
    objects.bus = 0;
    objects.subway = 0;
    objects.unknown = 0;
    objects.obj9 = 0;
    deletePageFlowState(0);
}

void tick_screen_main_page() {
    void *flowState = getFlowState(0, 0);
    (void)flowState;
    {
        const char *new_val = evalTextProperty(flowState, 0, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.current_time);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.current_time;
            lv_label_set_text(objects.current_time, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        int32_t new_val = evalIntegerProperty(flowState, 2, 3, "Failed to evaluate Value in Bar widget");
        int32_t cur_val = lv_bar_get_value(objects.bar_battery);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.bar_battery;
            lv_bar_set_value(objects.bar_battery, new_val, LV_ANIM_OFF);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 3, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.current_date);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.current_date;
            lv_label_set_text(objects.current_date, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        int32_t new_val = evalIntegerProperty(flowState, 4, 3, "Failed to evaluate Value in Arc widget");
        int32_t cur_val = lv_arc_get_value(objects.obj7);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj7;
            lv_arc_set_value(objects.obj7, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 5, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.label_battery);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.label_battery;
            lv_label_set_text(objects.label_battery, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 6, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.obj8);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj8;
            lv_label_set_text(objects.obj8, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 13, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.qingchongdian, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.qingchongdian;
            if (new_val) lv_obj_add_flag(objects.qingchongdian, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.qingchongdian, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 14, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.ic_stationary, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.ic_stationary;
            if (new_val) lv_obj_add_flag(objects.ic_stationary, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.ic_stationary, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 15, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.walk, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.walk;
            if (new_val) lv_obj_add_flag(objects.walk, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.walk, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 16, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.bike, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.bike;
            if (new_val) lv_obj_add_flag(objects.bike, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.bike, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 17, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.car, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.car;
            if (new_val) lv_obj_add_flag(objects.car, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.car, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 18, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.bus, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.bus;
            if (new_val) lv_obj_add_flag(objects.bus, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.bus, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 19, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.subway, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.subway;
            if (new_val) lv_obj_add_flag(objects.subway, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.subway, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 20, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.unknown, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.unknown;
            if (new_val) lv_obj_add_flag(objects.unknown, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.unknown, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 22, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.obj9, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj9;
            if (new_val) lv_obj_add_flag(objects.obj9, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.obj9, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_control_page() {
    void *flowState = getFlowState(0, 1);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.control_page = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 466, 466);
    lv_obj_add_event_cb(obj, event_handler_cb_control_page_control_page, LV_EVENT_ALL, flowState);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_slider_create(parent_obj);
            objects.obj10 = obj;
            lv_obj_set_pos(obj, 127, 253);
            lv_obj_set_size(obj, 311, 38);
            lv_slider_set_range(obj, 1, 100);
            lv_obj_add_event_cb(obj, event_handler_cb_control_page_obj10, LV_EVENT_ALL, flowState);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff66bb6a), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 51, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff286632), LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_outline_width(obj, 3, LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_outline_color(obj, lv_color_hex(0xffffffff), LV_PART_KNOB | LV_STATE_DEFAULT);
        }
        {
            // wifi
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.wifi = obj;
            lv_obj_set_pos(obj, 256, 109);
            lv_obj_set_size(obj, 120, 65);
            lv_obj_add_event_cb(obj, event_handler_cb_control_page_wifi, LV_EVENT_ALL, flowState);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 51, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff286632), LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_outline_width(obj, 2, LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_outline_color(obj, lv_color_hex(0xffffffff), LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff66bb6a), LV_PART_INDICATOR | LV_STATE_CHECKED);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 95, 110);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_wifi);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 168, 110);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_shijianjiaozhun);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 40, 240);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_liangdu);
        }
    }
    
    tick_screen_control_page();
}

void delete_screen_control_page() {
    lv_obj_delete(objects.control_page);
    objects.control_page = 0;
    objects.obj10 = 0;
    objects.wifi = 0;
    deletePageFlowState(1);
}

void tick_screen_control_page() {
    void *flowState = getFlowState(0, 1);
    (void)flowState;
    {
        int32_t new_val = evalIntegerProperty(flowState, 0, 3, "Failed to evaluate Value in Slider widget");
        int32_t cur_val = lv_slider_get_value(objects.obj10);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj10;
            lv_slider_set_value(objects.obj10, new_val, LV_ANIM_OFF);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_select_page() {
    void *flowState = getFlowState(0, 2);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.select_page = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 466, 466);
    lv_obj_add_event_cb(obj, event_handler_cb_select_page_select_page, LV_EVENT_ALL, flowState);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 73, 25);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_bike);
            lv_obj_add_event_cb(obj, event_handler_cb_select_page_obj0, LV_EVENT_ALL, flowState);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.obj1 = obj;
            lv_obj_set_pos(obj, 18, 169);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_bus);
            lv_obj_add_event_cb(obj, event_handler_cb_select_page_obj1, LV_EVENT_ALL, flowState);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.obj2 = obj;
            lv_obj_set_pos(obj, 159, 169);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_walk);
            lv_obj_add_event_cb(obj, event_handler_cb_select_page_obj2, LV_EVENT_ALL, flowState);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.obj3 = obj;
            lv_obj_set_pos(obj, 311, 176);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_car);
            lv_obj_add_event_cb(obj, event_handler_cb_select_page_obj3, LV_EVENT_ALL, flowState);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.obj4 = obj;
            lv_obj_set_pos(obj, 247, 41);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_subway);
            lv_obj_add_event_cb(obj, event_handler_cb_select_page_obj4, LV_EVENT_ALL, flowState);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.obj5 = obj;
            lv_obj_set_pos(obj, 95, 304);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_stationary);
            lv_obj_add_event_cb(obj, event_handler_cb_select_page_obj5, LV_EVENT_ALL, flowState);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.obj6 = obj;
            lv_obj_set_pos(obj, 257, 304);
            lv_obj_set_size(obj, 109, 109);
            lv_obj_add_event_cb(obj, event_handler_cb_select_page_obj6, LV_EVENT_ALL, flowState);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff286632), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, -22, -10);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_alibaba_pu_hui_ti_3_55_regular, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_letter_space(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_transform_scale_y(obj, 512, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_transform_scale_x(obj, 512, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "取消");
                }
            }
        }
    }
    
    tick_screen_select_page();
}

void delete_screen_select_page() {
    lv_obj_delete(objects.select_page);
    objects.select_page = 0;
    objects.obj0 = 0;
    objects.obj1 = 0;
    objects.obj2 = 0;
    objects.obj3 = 0;
    objects.obj4 = 0;
    objects.obj5 = 0;
    objects.obj6 = 0;
    deletePageFlowState(2);
}

void tick_screen_select_page() {
    void *flowState = getFlowState(0, 2);
    (void)flowState;
}

void create_screen_about_page() {
    void *flowState = getFlowState(0, 3);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.about_page = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 466, 466);
    lv_obj_add_event_cb(obj, event_handler_cb_about_page_about_page, LV_EVENT_ALL, flowState);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_qrcode_create(parent_obj);
            objects.obj11 = obj;
            lv_obj_set_pos(obj, 154, 138);
            lv_obj_set_size(obj, 160, 160);
            lv_qrcode_set_size(obj, 160);
            lv_qrcode_set_dark_color(obj, lv_color_hex(0xff20429f));
            lv_qrcode_set_light_color(obj, lv_color_hex(0xffe2f5fe));
            lv_qrcode_update(obj, "tel:15924812316", 15);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 163, 358);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "EcoStep V1.0");
        }
    }
    
    tick_screen_about_page();
}

void delete_screen_about_page() {
    lv_obj_delete(objects.about_page);
    objects.about_page = 0;
    objects.obj11 = 0;
    deletePageFlowState(3);
}

void tick_screen_about_page() {
    void *flowState = getFlowState(0, 3);
    (void)flowState;
}

void create_screen_fish_page() {
    void *flowState = getFlowState(0, 4);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.fish_page = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 466, 466);
    lv_obj_add_event_cb(obj, event_handler_cb_fish_page_fish_page, LV_EVENT_ALL, flowState);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // fish01_base
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish01_base = obj;
            lv_obj_set_pos(obj, 38, 122);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish01_base);
        }
        {
            // fish01_lock
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish01_lock = obj;
            lv_obj_set_pos(obj, 38, 122);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish01_lock);
        }
        {
            // fish03_base
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish03_base = obj;
            lv_obj_set_pos(obj, 188, 122);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish03_base);
        }
        {
            // fish03_lock
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish03_lock = obj;
            lv_obj_set_pos(obj, 188, 122);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish03_lock);
        }
        {
            // fish02_base
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish02_base = obj;
            lv_obj_set_pos(obj, 75, 283);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish02_base);
        }
        {
            // fish02_lock
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish02_lock = obj;
            lv_obj_set_pos(obj, 75, 283);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish02_lock);
        }
        {
            // fish04_base
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish04_base = obj;
            lv_obj_set_pos(obj, 233, 254);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish04_base);
        }
        {
            // fish04_lock
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish04_lock = obj;
            lv_obj_set_pos(obj, 233, 254);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish04_lock);
        }
        {
            // fish05_base
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish05_base = obj;
            lv_obj_set_pos(obj, 325, 122);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish05_base);
        }
        {
            // fish05_lock
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish05_lock = obj;
            lv_obj_set_pos(obj, 325, 122);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish05_lock);
        }
        {
            // fish06_base
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish06_base = obj;
            lv_obj_set_pos(obj, 406, 285);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish06_base);
        }
        {
            // fish06_lock
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish06_lock = obj;
            lv_obj_set_pos(obj, 406, 285);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish06_lock);
        }
        {
            // fish07_base
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish07_base = obj;
            lv_obj_set_pos(obj, 478, 122);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish07_base);
        }
        {
            // fish07_lock
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish07_lock = obj;
            lv_obj_set_pos(obj, 478, 122);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish07_lock);
        }
        {
            // fish08_base
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish08_base = obj;
            lv_obj_set_pos(obj, 566, 291);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish08_base);
        }
        {
            // fish08_lock
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish08_lock = obj;
            lv_obj_set_pos(obj, 566, 291);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish08_lock);
        }
        {
            // fish09_base
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish09_base = obj;
            lv_obj_set_pos(obj, 628, 122);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish09_base);
        }
        {
            // fish09_lock
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish09_lock = obj;
            lv_obj_set_pos(obj, 628, 122);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish09_lock);
        }
        {
            // fish10_base
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish10_base = obj;
            lv_obj_set_pos(obj, 713, 285);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish10_base);
        }
        {
            // fish10_lock
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fish10_lock = obj;
            lv_obj_set_pos(obj, 713, 285);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fish10_lock);
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 75, -105);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_bingshangao);
        }
        {
            // bingshan_1
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.bingshan_1 = obj;
            lv_obj_set_pos(obj, 90, -44);
            lv_obj_set_size(obj, 269, 119);
            lv_obj_add_event_cb(obj, event_handler_cb_fish_page_bingshan_1, LV_EVENT_ALL, flowState);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff335dca), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
            }
        }
    }
    
    tick_screen_fish_page();
}

void delete_screen_fish_page() {
    lv_obj_delete(objects.fish_page);
    objects.fish_page = 0;
    objects.fish01_base = 0;
    objects.fish01_lock = 0;
    objects.fish03_base = 0;
    objects.fish03_lock = 0;
    objects.fish02_base = 0;
    objects.fish02_lock = 0;
    objects.fish04_base = 0;
    objects.fish04_lock = 0;
    objects.fish05_base = 0;
    objects.fish05_lock = 0;
    objects.fish06_base = 0;
    objects.fish06_lock = 0;
    objects.fish07_base = 0;
    objects.fish07_lock = 0;
    objects.fish08_base = 0;
    objects.fish08_lock = 0;
    objects.fish09_base = 0;
    objects.fish09_lock = 0;
    objects.fish10_base = 0;
    objects.fish10_lock = 0;
    objects.bingshan_1 = 0;
    deletePageFlowState(4);
}

void tick_screen_fish_page() {
    void *flowState = getFlowState(0, 4);
    (void)flowState;
    {
        bool new_val = evalBooleanProperty(flowState, 2, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.fish01_lock, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.fish01_lock;
            if (new_val) lv_obj_add_flag(objects.fish01_lock, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.fish01_lock, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 4, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.fish03_lock, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.fish03_lock;
            if (new_val) lv_obj_add_flag(objects.fish03_lock, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.fish03_lock, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 6, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.fish02_lock, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.fish02_lock;
            if (new_val) lv_obj_add_flag(objects.fish02_lock, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.fish02_lock, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 8, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.fish04_lock, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.fish04_lock;
            if (new_val) lv_obj_add_flag(objects.fish04_lock, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.fish04_lock, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 10, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.fish05_lock, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.fish05_lock;
            if (new_val) lv_obj_add_flag(objects.fish05_lock, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.fish05_lock, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 12, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.fish06_lock, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.fish06_lock;
            if (new_val) lv_obj_add_flag(objects.fish06_lock, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.fish06_lock, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 14, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.fish07_lock, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.fish07_lock;
            if (new_val) lv_obj_add_flag(objects.fish07_lock, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.fish07_lock, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 16, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.fish08_lock, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.fish08_lock;
            if (new_val) lv_obj_add_flag(objects.fish08_lock, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.fish08_lock, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 18, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.fish09_lock, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.fish09_lock;
            if (new_val) lv_obj_add_flag(objects.fish09_lock, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.fish09_lock, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 20, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.fish10_lock, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.fish10_lock;
            if (new_val) lv_obj_add_flag(objects.fish10_lock, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.fish10_lock, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_developer_page() {
    void *flowState = getFlowState(0, 5);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.developer_page = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 466, 466);
    {
        lv_obj_t *parent_obj = obj;
        {
            // walk_data_get
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.walk_data_get = obj;
            lv_obj_set_pos(obj, 186, 30);
            lv_obj_set_size(obj, 130, 65);
            lv_obj_add_event_cb(obj, event_handler_cb_developer_page_walk_data_get, LV_EVENT_ALL, flowState);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 148, 46);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "walk");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 171, 138);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "stationary");
        }
        {
            // stationary_data_get
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.stationary_data_get = obj;
            lv_obj_set_pos(obj, 61, 154);
            lv_obj_set_size(obj, 130, 65);
            lv_obj_add_event_cb(obj, event_handler_cb_developer_page_stationary_data_get, LV_EVENT_ALL, flowState);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 260, 203);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "bike");
        }
        {
            // bike_data_get
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.bike_data_get = obj;
            lv_obj_set_pos(obj, 290, 154);
            lv_obj_set_size(obj, 130, 65);
            lv_obj_add_event_cb(obj, event_handler_cb_developer_page_bike_data_get, LV_EVENT_ALL, flowState);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 184, 257);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "car");
        }
        {
            // car_data_get
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.car_data_get = obj;
            lv_obj_set_pos(obj, 61, 263);
            lv_obj_set_size(obj, 130, 65);
            lv_obj_add_event_cb(obj, event_handler_cb_developer_page_car_data_get, LV_EVENT_ALL, flowState);
        }
        {
            // bus_data_get
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.bus_data_get = obj;
            lv_obj_set_pos(obj, 148, 367);
            lv_obj_set_size(obj, 130, 65);
            lv_obj_add_event_cb(obj, event_handler_cb_developer_page_bus_data_get, LV_EVENT_ALL, flowState);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 276, 389);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "bus");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 230, 296);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "subway");
        }
        {
            // subway_data_get
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.subway_data_get = obj;
            lv_obj_set_pos(obj, 284, 265);
            lv_obj_set_size(obj, 130, 65);
            lv_obj_add_event_cb(obj, event_handler_cb_developer_page_subway_data_get, LV_EVENT_ALL, flowState);
        }
        {
            // current_time_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.current_time_1 = obj;
            lv_obj_set_pos(obj, 324, 103);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLL_ONE);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_ON);
            lv_obj_set_scroll_dir(obj, LV_DIR_ALL);
            lv_obj_set_scroll_snap_x(obj, LV_SCROLL_SNAP_START);
            lv_obj_set_scroll_snap_y(obj, LV_SCROLL_SNAP_START);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_letter_space(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_SCROLLED);
            lv_label_set_text(obj, "");
        }
        {
            // current_date_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.current_date_1 = obj;
            lv_obj_set_pos(obj, 46, 103);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLL_ONE);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_ON);
            lv_obj_set_scroll_dir(obj, LV_DIR_ALL);
            lv_obj_set_scroll_snap_x(obj, LV_SCROLL_SNAP_START);
            lv_obj_set_scroll_snap_y(obj, LV_SCROLL_SNAP_START);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_letter_space(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
    }
    
    tick_screen_developer_page();
}

void delete_screen_developer_page() {
    lv_obj_delete(objects.developer_page);
    objects.developer_page = 0;
    objects.walk_data_get = 0;
    objects.stationary_data_get = 0;
    objects.bike_data_get = 0;
    objects.car_data_get = 0;
    objects.bus_data_get = 0;
    objects.subway_data_get = 0;
    objects.current_time_1 = 0;
    objects.current_date_1 = 0;
    deletePageFlowState(5);
}

void tick_screen_developer_page() {
    void *flowState = getFlowState(0, 5);
    (void)flowState;
    {
        const char *new_val = evalTextProperty(flowState, 13, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.current_time_1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.current_time_1;
            lv_label_set_text(objects.current_time_1, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 14, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.current_date_1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.current_date_1;
            lv_label_set_text(objects.current_date_1, new_val);
            tick_value_change_obj = NULL;
        }
    }
}


static const char *screen_names[] = { "main_page", "control_page", "select_page", "about_page", "fish_page", "developer_page" };
static const char *object_names[] = { "main_page", "control_page", "select_page", "about_page", "fish_page", "developer_page", "bingshan", "mode", "wifi", "obj0", "obj1", "obj2", "obj3", "obj4", "obj5", "obj6", "bingshan_1", "walk_data_get", "stationary_data_get", "bike_data_get", "car_data_get", "bus_data_get", "subway_data_get", "current_time", "bar_battery", "current_date", "obj7", "label_battery", "qingchongdian", "ic_stationary", "walk", "bike", "car", "bus", "subway", "unknown", "obj8", "obj9", "obj10", "obj11", "fish01_base", "fish01_lock", "fish03_base", "fish03_lock", "fish02_base", "fish02_lock", "fish04_base", "fish04_lock", "fish05_base", "fish05_lock", "fish06_base", "fish06_lock", "fish07_base", "fish07_lock", "fish08_base", "fish08_lock", "fish09_base", "fish09_lock", "fish10_base", "fish10_lock", "current_time_1", "current_date_1" };


typedef void (*create_screen_func_t)();
create_screen_func_t create_screen_funcs[] = {
    create_screen_main_page,
    create_screen_control_page,
    create_screen_select_page,
    create_screen_about_page,
    create_screen_fish_page,
    create_screen_developer_page,
};
void create_screen(int screen_index) {
    create_screen_funcs[screen_index]();
}
void create_screen_by_id(enum ScreensEnum screenId) {
    create_screen_funcs[screenId - 1]();
}

typedef void (*delete_screen_func_t)();
delete_screen_func_t delete_screen_funcs[] = {
    delete_screen_main_page,
    delete_screen_control_page,
    delete_screen_select_page,
    delete_screen_about_page,
    delete_screen_fish_page,
    delete_screen_developer_page,
};
void delete_screen(int screen_index) {
    delete_screen_funcs[screen_index]();
}
void delete_screen_by_id(enum ScreensEnum screenId) {
    delete_screen_funcs[screenId - 1]();
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main_page,
    tick_screen_control_page,
    tick_screen_select_page,
    tick_screen_about_page,
    tick_screen_fish_page,
    tick_screen_developer_page,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    eez_flow_init_screen_names(screen_names, sizeof(screen_names) / sizeof(const char *));
    eez_flow_init_object_names(object_names, sizeof(object_names) / sizeof(const char *));
    
    eez_flow_set_create_screen_func(create_screen);
    eez_flow_set_delete_screen_func(delete_screen);
    
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_main_page();
    create_screen_select_page();
    create_screen_fish_page();
    create_screen_developer_page();
}
