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

static void event_handler_cb_main_page_test(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 5, 0, e);
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

static void event_handler_cb_about_page_back_main(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 2, 0, e);
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
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // current_time
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.current_time = obj;
            lv_obj_set_pos(obj, 194, 254);
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
            // bar_battery
            lv_obj_t *obj = lv_bar_create(parent_obj);
            objects.bar_battery = obj;
            lv_obj_set_pos(obj, 197, 79);
            lv_obj_set_size(obj, 50, 10);
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
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // current_date
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.current_date = obj;
            lv_obj_set_pos(obj, 142, 189);
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
            lv_obj_set_pos(obj, 26, 26);
            lv_obj_set_size(obj, 415, 415);
            lv_arc_set_value(obj, 50);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN_VER|LV_OBJ_FLAG_SCROLL_ON_FOCUS|LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        }
        {
            // test
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.test = obj;
            lv_obj_set_pos(obj, 163, 341);
            lv_obj_set_size(obj, 140, 70);
            lv_obj_add_event_cb(obj, event_handler_cb_main_page_test, LV_EVENT_ALL, flowState);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "about");
                }
            }
        }
        {
            // label_battery
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_battery = obj;
            lv_obj_set_pos(obj, 248, 76);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 371, 218);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "state\n");
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
    objects.test = 0;
    objects.label_battery = 0;
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
        const char *new_val = evalTextProperty(flowState, 7, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.label_battery);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.label_battery;
            lv_label_set_text(objects.label_battery, new_val);
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
            lv_obj_set_pos(obj, 131, 272);
            lv_obj_set_size(obj, 247, 22);
            lv_slider_set_value(obj, 25, LV_ANIM_OFF);
        }
        {
            lv_obj_t *obj = lv_switch_create(parent_obj);
            lv_obj_set_pos(obj, 242, 105);
            lv_obj_set_size(obj, 85, 47);
        }
    }
    
    tick_screen_control_page();
}

void delete_screen_control_page() {
    lv_obj_delete(objects.control_page);
    objects.control_page = 0;
    deletePageFlowState(1);
}

void tick_screen_control_page() {
    void *flowState = getFlowState(0, 1);
    (void)flowState;
}

void create_screen_test_page() {
    void *flowState = getFlowState(0, 2);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.test_page = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 466, 466);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_list_create(parent_obj);
            lv_obj_set_pos(obj, 143, 122);
            lv_obj_set_size(obj, 180, 100);
            lv_obj_set_style_layout(obj, LV_LAYOUT_GRID, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_flow(obj, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                static lv_coord_t dsc[] = {LV_GRID_TEMPLATE_LAST};
                lv_obj_set_style_grid_row_dsc_array(obj, dsc, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            {
                static lv_coord_t dsc[] = {LV_GRID_TEMPLATE_LAST};
                lv_obj_set_style_grid_column_dsc_array(obj, dsc, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            lv_obj_set_style_flex_main_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_cross_place(obj, LV_FLEX_ALIGN_START, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_flex_track_place(obj, LV_FLEX_ALIGN_END, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_test_page();
}

void delete_screen_test_page() {
    lv_obj_delete(objects.test_page);
    objects.test_page = 0;
    deletePageFlowState(2);
}

void tick_screen_test_page() {
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
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_qrcode_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 153, 153);
            lv_obj_set_size(obj, 160, 160);
            lv_qrcode_set_size(obj, 160);
            lv_qrcode_set_dark_color(obj, lv_color_hex(0xff20429f));
            lv_qrcode_set_light_color(obj, lv_color_hex(0xffe2f5fe));
            lv_qrcode_update(obj, "tel:15924812316", 15);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        }
        {
            // back_main
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.back_main = obj;
            lv_obj_set_pos(obj, 183, 376);
            lv_obj_set_size(obj, 100, 50);
            lv_obj_add_event_cb(obj, event_handler_cb_about_page_back_main, LV_EVENT_ALL, flowState);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "◀");
                }
            }
        }
    }
    
    tick_screen_about_page();
}

void delete_screen_about_page() {
    lv_obj_delete(objects.about_page);
    objects.about_page = 0;
    objects.obj0 = 0;
    objects.back_main = 0;
    deletePageFlowState(3);
}

void tick_screen_about_page() {
    void *flowState = getFlowState(0, 3);
    (void)flowState;
}

void create_screen_calendar_page() {
    void *flowState = getFlowState(0, 4);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.calendar_page = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 466, 466);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_calendar_create(parent_obj);
            objects.obj1 = obj;
            lv_obj_set_pos(obj, 58, 66);
            lv_obj_set_size(obj, 350, 350);
            lv_calendar_header_arrow_create(obj);
            lv_calendar_set_today_date(obj, 2026, 2, 1);
            lv_calendar_set_showed_date(obj, 2026, 2);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_calendar_page();
}

void delete_screen_calendar_page() {
    lv_obj_delete(objects.calendar_page);
    objects.calendar_page = 0;
    objects.obj1 = 0;
    deletePageFlowState(4);
}

void tick_screen_calendar_page() {
    void *flowState = getFlowState(0, 4);
    (void)flowState;
}

void create_screen_chart_page() {
    void *flowState = getFlowState(0, 5);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.chart_page = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 466, 466);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_chart_create(parent_obj);
            lv_obj_set_pos(obj, 106, 141);
            lv_obj_set_size(obj, 254, 185);
        }
    }
    
    tick_screen_chart_page();
}

void delete_screen_chart_page() {
    lv_obj_delete(objects.chart_page);
    objects.chart_page = 0;
    deletePageFlowState(5);
}

void tick_screen_chart_page() {
    void *flowState = getFlowState(0, 5);
    (void)flowState;
}

void create_screen_menu_page() {
    void *flowState = getFlowState(0, 6);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.menu_page = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 466, 466);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    tick_screen_menu_page();
}

void delete_screen_menu_page() {
    lv_obj_delete(objects.menu_page);
    objects.menu_page = 0;
    deletePageFlowState(6);
}

void tick_screen_menu_page() {
    void *flowState = getFlowState(0, 6);
    (void)flowState;
}

void create_screen_developer_page() {
    void *flowState = getFlowState(0, 7);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.developer_page = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 466, 466);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 148, 26);
            lv_obj_set_size(obj, 170, 84);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_alibaba_pu_hui_ti_3_55_regular, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "IMU 校准");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 259, 136);
            lv_obj_set_size(obj, 170, 84);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Stationary");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 37, 136);
            lv_obj_set_size(obj, 170, 84);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "walk");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 148, 343);
            lv_obj_set_size(obj, 170, 84);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // Bus
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.bus = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Bus");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 37, 246);
            lv_obj_set_size(obj, 170, 84);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Bike");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 259, 246);
            lv_obj_set_size(obj, 170, 84);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Car");
                }
            }
        }
    }
    
    tick_screen_developer_page();
}

void delete_screen_developer_page() {
    lv_obj_delete(objects.developer_page);
    objects.developer_page = 0;
    objects.bus = 0;
    deletePageFlowState(7);
}

void tick_screen_developer_page() {
    void *flowState = getFlowState(0, 7);
    (void)flowState;
}


static const char *screen_names[] = { "main_page", "control_page", "test_page", "about_page", "calendar_page", "chart_page", "menu_page", "developer_page" };
static const char *object_names[] = { "main_page", "control_page", "test_page", "about_page", "calendar_page", "chart_page", "menu_page", "developer_page", "test", "back_main", "current_time", "bar_battery", "current_date", "label_battery", "obj0", "obj1", "bus" };


typedef void (*create_screen_func_t)();
create_screen_func_t create_screen_funcs[] = {
    create_screen_main_page,
    create_screen_control_page,
    create_screen_test_page,
    create_screen_about_page,
    create_screen_calendar_page,
    create_screen_chart_page,
    create_screen_menu_page,
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
    delete_screen_test_page,
    delete_screen_about_page,
    delete_screen_calendar_page,
    delete_screen_chart_page,
    delete_screen_menu_page,
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
    tick_screen_test_page,
    tick_screen_about_page,
    tick_screen_calendar_page,
    tick_screen_chart_page,
    tick_screen_menu_page,
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
    create_screen_menu_page();
    create_screen_developer_page();
}
