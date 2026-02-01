#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main_page;
    lv_obj_t *control_page;
    lv_obj_t *select_page;
    lv_obj_t *about_page;
    lv_obj_t *fish_page;
    lv_obj_t *developer_page;
    lv_obj_t *bingshan;
    lv_obj_t *mode;
    lv_obj_t *wifi;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *bingshan_1;
    lv_obj_t *walk_data_get;
    lv_obj_t *stationary_data_get;
    lv_obj_t *bike_data_get;
    lv_obj_t *car_data_get;
    lv_obj_t *bus_data_get;
    lv_obj_t *subway_data_get;
    lv_obj_t *current_time;
    lv_obj_t *bar_battery;
    lv_obj_t *current_date;
    lv_obj_t *obj7;
    lv_obj_t *label_battery;
    lv_obj_t *qingchongdian;
    lv_obj_t *ic_stationary;
    lv_obj_t *walk;
    lv_obj_t *bike;
    lv_obj_t *car;
    lv_obj_t *bus;
    lv_obj_t *subway;
    lv_obj_t *unknown;
    lv_obj_t *obj8;
    lv_obj_t *obj9;
    lv_obj_t *obj10;
    lv_obj_t *obj11;
    lv_obj_t *fish01_base;
    lv_obj_t *fish01_lock;
    lv_obj_t *fish03_base;
    lv_obj_t *fish03_lock;
    lv_obj_t *fish02_base;
    lv_obj_t *fish02_lock;
    lv_obj_t *fish04_base;
    lv_obj_t *fish04_lock;
    lv_obj_t *fish05_base;
    lv_obj_t *fish05_lock;
    lv_obj_t *fish06_base;
    lv_obj_t *fish06_lock;
    lv_obj_t *fish07_base;
    lv_obj_t *fish07_lock;
    lv_obj_t *fish08_base;
    lv_obj_t *fish08_lock;
    lv_obj_t *fish09_base;
    lv_obj_t *fish09_lock;
    lv_obj_t *fish10_base;
    lv_obj_t *fish10_lock;
    lv_obj_t *current_time_1;
    lv_obj_t *current_date_1;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN_PAGE = 1,
    SCREEN_ID_CONTROL_PAGE = 2,
    SCREEN_ID_SELECT_PAGE = 3,
    SCREEN_ID_ABOUT_PAGE = 4,
    SCREEN_ID_FISH_PAGE = 5,
    SCREEN_ID_DEVELOPER_PAGE = 6,
};

void create_screen_main_page();
void delete_screen_main_page();
void tick_screen_main_page();

void create_screen_control_page();
void delete_screen_control_page();
void tick_screen_control_page();

void create_screen_select_page();
void delete_screen_select_page();
void tick_screen_select_page();

void create_screen_about_page();
void delete_screen_about_page();
void tick_screen_about_page();

void create_screen_fish_page();
void delete_screen_fish_page();
void tick_screen_fish_page();

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