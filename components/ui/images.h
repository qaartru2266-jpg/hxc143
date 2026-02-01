#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_dianchi;
extern const lv_img_dsc_t img_wifi;
extern const lv_img_dsc_t img_shijianjiaozhun;
extern const lv_img_dsc_t img_liangdu;
extern const lv_img_dsc_t img_stationary;
extern const lv_img_dsc_t img_walk;
extern const lv_img_dsc_t img_bike;
extern const lv_img_dsc_t img_car;
extern const lv_img_dsc_t img_bus;
extern const lv_img_dsc_t img_subway;
extern const lv_img_dsc_t img_bingshan;
extern const lv_img_dsc_t img_bingshangao;
extern const lv_img_dsc_t img_fish01_base;
extern const lv_img_dsc_t img_fish01_lock;
extern const lv_img_dsc_t img_fish02_base;
extern const lv_img_dsc_t img_fish02_lock;
extern const lv_img_dsc_t img_fish03_base;
extern const lv_img_dsc_t img_fish03_lock;
extern const lv_img_dsc_t img_fish04_lock;
extern const lv_img_dsc_t img_fish04_base;
extern const lv_img_dsc_t img_fish05_base;
extern const lv_img_dsc_t img_fish05_lock;
extern const lv_img_dsc_t img_fish06_lock;
extern const lv_img_dsc_t img_fish06_base;
extern const lv_img_dsc_t img_fish07_lock;
extern const lv_img_dsc_t img_fish07_base;
extern const lv_img_dsc_t img_fish08_base;
extern const lv_img_dsc_t img_fish08_lock;
extern const lv_img_dsc_t img_fish09_base;
extern const lv_img_dsc_t img_fish09_lock;
extern const lv_img_dsc_t img_fish10_base;
extern const lv_img_dsc_t img_fish10_lock;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[32];


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/