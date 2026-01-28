#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main_page;
    lv_obj_t *control_page;
    lv_obj_t *test_page;
    lv_obj_t *about_page;
    lv_obj_t *calendar_page;
    lv_obj_t *chart_page;
    lv_obj_t *menu_page;
    lv_obj_t *developer_page;
    lv_obj_t *test;
    lv_obj_t *back_main;
    lv_obj_t *walk_data_get;
    lv_obj_t *stationary_data_get;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *current_time;
    lv_obj_t *bar_battery;
    lv_obj_t *current_date;
    lv_obj_t *label_battery;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN_PAGE = 1,
    SCREEN_ID_CONTROL_PAGE = 2,
    SCREEN_ID_TEST_PAGE = 3,
    SCREEN_ID_ABOUT_PAGE = 4,
    SCREEN_ID_CALENDAR_PAGE = 5,
    SCREEN_ID_CHART_PAGE = 6,
    SCREEN_ID_MENU_PAGE = 7,
    SCREEN_ID_DEVELOPER_PAGE = 8,
};

void create_screen_main_page();
void delete_screen_main_page();
void tick_screen_main_page();

void create_screen_control_page();
void delete_screen_control_page();
void tick_screen_control_page();

void create_screen_test_page();
void delete_screen_test_page();
void tick_screen_test_page();

void create_screen_about_page();
void delete_screen_about_page();
void tick_screen_about_page();

void create_screen_calendar_page();
void delete_screen_calendar_page();
void tick_screen_calendar_page();

void create_screen_chart_page();
void delete_screen_chart_page();
void tick_screen_chart_page();

void create_screen_menu_page();
void delete_screen_menu_page();
void tick_screen_menu_page();

void create_screen_developer_page();
void delete_screen_developer_page();
void tick_screen_developer_page();

void create_screen_by_id(enum ScreensEnum screenId);
void delete_screen_by_id(enum ScreensEnum screenId);
void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/