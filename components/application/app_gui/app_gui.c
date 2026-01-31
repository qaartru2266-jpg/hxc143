// main/app_gui.c
#include "app_gui.h"
#include "display_hal.h"
#include "ui.h"
#include "vars.h"


#include "lvgl.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_memory_utils.h"
#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#include "app_touch.h"
#include "app_power.h"
#include "app_antenna.h"
#include "actions.h"
#include "sdkconfig.h"
#if CONFIG_JOFTMODE_ENABLE_ML
#include "app_ml.h"
#endif

// Declared in components/ui/actions.c; keep local to avoid generator overwriting actions.h.
void ui_set_wifi_toggle(bool enabled);

void app_gui_set_flow_var_int(int32_t var_id, int32_t value);
void app_gui_set_flow_var_string(int32_t var_id, const char *value);
int32_t app_gui_get_flow_var_int(int32_t var_id, int32_t default_value);

static const char *TAG = "app_gui";
static volatile bool s_ui_ready = false;
static display_hal_t s_hal;              //  s_hal：保�?display_hal
static esp_lcd_panel_handle_t s_panel_handle = NULL;   //屏幕句柄，用于开关屏
static bool s_screen_on = true;  //屏幕状�?
static lv_indev_t *s_touch_indev = NULL; //LVGL 输入设备
static int32_t s_last_brightness = -1;
static lv_draw_buf_t s_draw_buf1;
static lv_draw_buf_t s_draw_buf2;
static void *s_draw_buf1_mem = NULL;
static void *s_draw_buf2_mem = NULL;
static uint8_t *s_dma_bounce = NULL;
static size_t s_dma_bounce_size = 0;
// Set to 1 to run display_hal_test_once() during startup (useful for panel bring-up).
#define APP_GUI_RUN_DISPLAY_TEST_ONCE 0
#define APP_GUI_DEFAULT_BRIGHTNESS 100  // 1-100
#define APP_GUI_USE_FULL_FB 1
#define APP_GUI_REQUIRE_PSRAM 1
#define APP_GUI_DMA_BOUNCE_LINES 8

#if 0

static lv_obj_t *s_main_screen = NULL;
static lv_obj_t *s_clock_screen = NULL;
static lv_obj_t *s_clock_label = NULL;
static lv_timer_t *s_clock_timer = NULL;  //两个界面和时钟相关对�?
static time_t s_clock_epoch = 0;  //时钟时间戳（秒）

extern const lv_image_dsc_t wallpaper1;  //背景图片资源
#endif

static void update_time_vars(void)
{
    static time_t last_update = 0;
    time_t now = time(NULL);
    if (now <= 0 || now == last_update) {
        return;
    }
    last_update = now;

    struct tm local_tm;
    if (!localtime_r(&now, &local_tm)) {
        return;
    }

    char time_buf[6] = {0};
    if (strftime(time_buf, sizeof(time_buf), "%H:%M", &local_tm) > 0) {
        app_gui_set_flow_var_string(FLOW_GLOBAL_VARIABLE_CURRENT_TIME, time_buf);
    }

    char date_buf[11] = {0};
    if (strftime(date_buf, sizeof(date_buf), "%Y/%m/%d", &local_tm) > 0) {
        app_gui_set_flow_var_string(FLOW_GLOBAL_VARIABLE_CURRENT_DATE, date_buf);
    }
}

#if CONFIG_JOFTMODE_ENABLE_ML
static void update_mode_var(void)
{
    static uint64_t last_uptime_ms = 0;
    app_ml_status_t st;
    if (!app_ml_get_latest_status(&st)) {
        return;
    }
    if (st.uptime_ms == 0 || st.uptime_ms == last_uptime_ms) {
        return;
    }
    last_uptime_ms = st.uptime_ms;

    int32_t mode_value = (st.pred_smooth >= 0) ? st.pred_smooth : -1;
    app_gui_set_flow_var_int(FLOW_GLOBAL_VARIABLE_MODE_LVGL, mode_value);
}
#endif

/* ---------- LVGL tick（v9 要求“返回毫秒”） ---------- */
static uint32_t lv_tick_cb(void)  //告诉 LVGL “现在的毫秒数”，LVGL 用它处理动画/定时�?
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

/* ---------- 刷新回调：把 px_map 区域刷到面板 ---------- */
//LVGL 画完一块区域后回调，把像素数据交给 esp_lcd_panel_draw_bitmap() 真正刷到屏上�?
//使用 trans_done �?te_sema 做同步，避免撕裂或传输冲突�?
static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    const int w = area->x2 - area->x1 + 1;
    const int h = area->y2 - area->y1 + 1;
    if (w <= 0 || h <= 0) {
        lv_display_flush_ready(disp);
        return;
    }

    if (!px_map || !area) {
        lv_display_flush_ready(disp);
        return;
    }

    const size_t src_stride = LV_DRAW_BUF_STRIDE(s_hal.hor_res, LV_COLOR_FORMAT_RGB888);
    const uint8_t *base = px_map;
    if (APP_GUI_USE_FULL_FB) {
        base = px_map + (size_t)area->y1 * src_stride + (size_t)area->x1 * 3U;
    }

    if (!esp_ptr_dma_capable(base)) {
        const size_t dst_stride = (size_t)w * 3U;
        const size_t chunk_bytes = dst_stride * APP_GUI_DMA_BOUNCE_LINES;
        if (!s_dma_bounce || s_dma_bounce_size < chunk_bytes) {
            if (s_dma_bounce) {
                heap_caps_free(s_dma_bounce);
                s_dma_bounce = NULL;
                s_dma_bounce_size = 0;
            }
            s_dma_bounce = (uint8_t*)heap_caps_malloc(chunk_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
            if (s_dma_bounce) {
                s_dma_bounce_size = chunk_bytes;
            }
        }
        if (!s_dma_bounce) {
            ESP_LOGE(TAG, "DMA bounce alloc failed, size=%u", (unsigned)chunk_bytes);
            lv_display_flush_ready(disp);
            return;
        }

        int y = 0;
        while (y < h) {
            const int lines = (h - y) > APP_GUI_DMA_BOUNCE_LINES ? APP_GUI_DMA_BOUNCE_LINES : (h - y);
            uint8_t *dst = s_dma_bounce;
            const uint8_t *src = base + (size_t)y * src_stride;
            for (int i = 0; i < lines; ++i) {
                memcpy(dst + (size_t)i * dst_stride, src + (size_t)i * src_stride, dst_stride);
            }

            if (s_hal.trans_done) (void)xSemaphoreTake(s_hal.trans_done, 0);
            if (s_hal.te_sema) (void)xSemaphoreTake(s_hal.te_sema, pdMS_TO_TICKS(5));

            esp_err_t e = esp_lcd_panel_draw_bitmap(
                s_hal.panel,
                area->x1, area->y1 + y,
                area->x2 + 1, area->y1 + y + lines,
                s_dma_bounce
            );

            if (e == ESP_OK && s_hal.trans_done) {
                (void)xSemaphoreTake(s_hal.trans_done, portMAX_DELAY);
            } else if (e != ESP_OK) {
                ESP_LOGE(TAG, "draw_bitmap failed: %d", (int)e);
                break;
            }

            y += lines;
        }

        lv_display_flush_ready(disp);
        return;
    }

    if (s_hal.trans_done) (void)xSemaphoreTake(s_hal.trans_done, 0);
    if (s_hal.te_sema) (void)xSemaphoreTake(s_hal.te_sema, pdMS_TO_TICKS(5));

    esp_err_t e = esp_lcd_panel_draw_bitmap(
        s_hal.panel,
        area->x1, area->y1,
        area->x2 + 1, area->y2 + 1,
        base
    );

    if (e == ESP_OK && s_hal.trans_done) {
        (void)xSemaphoreTake(s_hal.trans_done, portMAX_DELAY);
    } else if (e != ESP_OK) {
        ESP_LOGE(TAG, "draw_bitmap failed: %d", (int)e);
    }

    lv_display_flush_ready(disp);
}


/* ---------- 触控回调函数 ---------- */
//LVGL 读触控数据的回调，内部调用app_touch_read() 把触控坐标塞�?LVGL�?
static void touch_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
    LV_UNUSED(indev);

    int32_t x = 0, y = 0;
    bool pressed = app_touch_read(&x, &y); // 调用 C++ 那边的方法
    
    bool consume = app_power_on_touch(pressed);

    if (consume) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    if (pressed) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/* update_clock_label()/clock_timer_cb()：更新文字时间，每秒 +1�?*/
#if 0
static void update_clock_label(void)
{
    if (!s_clock_label) {
        return;
    }

    struct tm current_time;
    if (localtime_r(&s_clock_epoch, &current_time) == NULL) {
        return;
    }

    char buf[32];
    strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", &current_time);
    lv_label_set_text(s_clock_label, buf);
}

static void clock_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    s_clock_epoch += 1;
    update_clock_label();
}

//gesture_event_cb()：左右滑动切换屏幕（主屏 <-> 时钟屏）
static void gesture_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_GESTURE) {
        return;
    }

    lv_obj_t *target = lv_event_get_target(e);
    lv_dir_t dir = LV_DIR_NONE;
    lv_indev_t *active = lv_indev_active();
    if (active) {
        dir = lv_indev_get_gesture_dir(active);
    }

    if (target == s_main_screen && dir == LV_DIR_LEFT) {
        lv_screen_load_anim(s_clock_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
    } else if (target == s_clock_screen && dir == LV_DIR_RIGHT) {
        lv_screen_load_anim(s_main_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
    }
}
#endif


/* ---------- GUI 任务（唯一地方调用 lv_label_set_text---------- */
static void gui_task(void *arg)
{
    ESP_LOGI(TAG, "GUI task start");

    // 1) 初始化底层显�?
    esp_err_t err = display_hal_init(&s_hal);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "display_hal_init failed: %d", (int)err);
        vTaskDelete(NULL);
        return;
    }
    {
        esp_err_t br_err = display_hal_set_brightness(&s_hal, APP_GUI_DEFAULT_BRIGHTNESS);
        if (br_err != ESP_OK) {
            ESP_LOGW(TAG, "set brightness failed: %d", (int)br_err);
        }
    }
    s_panel_handle = s_hal.panel;
    s_screen_on = true;

#if APP_GUI_RUN_DISPLAY_TEST_ONCE
    {
        esp_err_t test_err = display_hal_test_once(&s_hal);
        if (test_err != ESP_OK) {
            ESP_LOGW(TAG, "display_hal_test_once failed: %d", (int)test_err);
        }
        vTaskDelay(pdMS_TO_TICKS(1200)); // keep pattern visible briefly
    }
#endif

    // 2) 初始�?LVGL & tick
    lv_init();
    lv_tick_set_cb(lv_tick_cb);

    // 3) 创建 display —�?保持 RGB888�? 字节/像素�?
    lv_display_t *disp = lv_display_create(s_hal.hor_res, s_hal.ver_res);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB888);

    // 4) 配置行缓冲（格式也用 RGB888）（24 行的 line buffer�?
    const uint32_t line_cnt = 12;   // reduce buffer to save RAM
    const uint32_t buf_height = APP_GUI_USE_FULL_FB ? s_hal.ver_res : line_cnt;
    const uint32_t buf_size = LV_DRAW_BUF_SIZE(s_hal.hor_res, buf_height, LV_COLOR_FORMAT_RGB888);
    if (!s_draw_buf1_mem) {
        s_draw_buf1_mem = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    if (!s_draw_buf2_mem) {
        s_draw_buf2_mem = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
#if !APP_GUI_REQUIRE_PSRAM
    if (!s_draw_buf1_mem || !s_draw_buf2_mem) {
        if (!s_draw_buf1_mem) {
            s_draw_buf1_mem = heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        }
        if (!s_draw_buf2_mem) {
            s_draw_buf2_mem = heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        }
    }
#endif
    if (!s_draw_buf1_mem || !s_draw_buf2_mem) {
        if (s_draw_buf1_mem) {
            heap_caps_free(s_draw_buf1_mem);
            s_draw_buf1_mem = NULL;
        }
        if (s_draw_buf2_mem) {
            heap_caps_free(s_draw_buf2_mem);
            s_draw_buf2_mem = NULL;
        }
        ESP_LOGE(TAG, "LVGL draw buffer alloc failed, size=%u", (unsigned)buf_size);
        vTaskDelete(NULL);
        return;
    }
    if (lv_draw_buf_init(&s_draw_buf1, s_hal.hor_res, buf_height, LV_COLOR_FORMAT_RGB888,
                         LV_STRIDE_AUTO, s_draw_buf1_mem, buf_size) != LV_RESULT_OK) {
        ESP_LOGE(TAG, "LVGL draw buf1 init failed");
        vTaskDelete(NULL);
        return;
    }
    if (lv_draw_buf_init(&s_draw_buf2, s_hal.hor_res, buf_height, LV_COLOR_FORMAT_RGB888,
                         LV_STRIDE_AUTO, s_draw_buf2_mem, buf_size) != LV_RESULT_OK) {
        ESP_LOGE(TAG, "LVGL draw buf2 init failed");
        vTaskDelete(NULL);
        return;
    }
    lv_draw_buf_set_flag(&s_draw_buf1, LV_IMAGE_FLAGS_MODIFIABLE);
    lv_draw_buf_set_flag(&s_draw_buf2, LV_IMAGE_FLAGS_MODIFIABLE);
    lv_display_set_draw_buffers(disp, &s_draw_buf1, &s_draw_buf2);
    if (APP_GUI_USE_FULL_FB) {
        lv_display_set_render_mode(disp, LV_DISPLAY_RENDER_MODE_DIRECT);
    }

    // 5) 刷新回调，设flush 回调 flush_cb()
    lv_display_set_flush_cb(disp, flush_cb);

// ... 原有�?lv_display_set_flush_cb(disp, flush_cb); 之后 ...

    // --- 新增：初始化硬件触控 ---
    app_touch_init();

    // --- 新增：注�?LVGL 输入设备 (LVGL v9 写法) ---
    s_touch_indev = lv_indev_create();                // 创建输入设备
    lv_indev_set_type(s_touch_indev, LV_INDEV_TYPE_POINTER);  // 设置类型为指触摸)
    lv_indev_set_read_cb(s_touch_indev, touch_read_cb);       // 设置回调函数
    lv_indev_set_display(s_touch_indev, disp);                // 绑定到当前屏幕
    ui_init();
    s_ui_ready = true;
    app_gui_set_flow_var_int(FLOW_GLOBAL_VARIABLE_BRIGHTNESS_LEVEL, APP_GUI_DEFAULT_BRIGHTNESS);
    s_last_brightness = APP_GUI_DEFAULT_BRIGHTNESS;

    // ... 原有�?lv_obj_set_style_bg_color ...


#if 0
    // 6) 黑底 + 白字；彻底“非斜体”，并保证真正居�?
    s_main_screen = lv_screen_active();
    lv_obj_set_style_bg_color(s_main_screen, lv_color_black(), 0);


    // 用整屏图片替换文字显�?
    lv_obj_t *img = lv_image_create(s_main_screen);
    lv_image_set_src(img, &wallpaper1);
    lv_obj_set_size(img, s_hal.hor_res, s_hal.ver_res);
    lv_obj_center(img);
    lv_obj_add_flag(img, LV_OBJ_FLAG_GESTURE_BUBBLE);

    // Screen 2: clock screen with black background
    s_clock_screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(s_clock_screen);
    lv_obj_set_style_bg_color(s_clock_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_clock_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_clock_screen, LV_OBJ_FLAG_SCROLLABLE);

    s_clock_label = lv_label_create(s_clock_screen);
    lv_obj_set_style_text_color(s_clock_label, lv_color_white(), 0);
    lv_obj_set_style_text_align(s_clock_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(s_clock_label);
    lv_obj_add_flag(s_clock_label, LV_OBJ_FLAG_GESTURE_BUBBLE);

    struct tm initial_tm = {
        .tm_year = 2025 - 1900,
        .tm_mon = 12 - 1,
        .tm_mday = 30,
        .tm_hour = 14,
        .tm_min = 11,
        .tm_sec = 0,
    };
    s_clock_epoch = mktime(&initial_tm);
    update_clock_label();

    s_clock_timer = lv_timer_create(clock_timer_cb, 1000, NULL);
    if (s_clock_timer) {
        lv_timer_set_repeat_count(s_clock_timer, -1);
    }

    lv_obj_add_event_cb(s_main_screen, gesture_event_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(s_clock_screen, gesture_event_cb, LV_EVENT_GESTURE, NULL);

    // 7) 启动时主动全�?
    lv_obj_invalidate(s_main_screen);
    lv_refr_now(disp);
#endif


    ESP_LOGW(TAG, "LVGL fmt=RGB888; ensure panel is 3B/px (18/24bpp) to avoid slant.");

    while (1) {
        lv_timer_handler();
        update_time_vars();
#if CONFIG_JOFTMODE_ENABLE_ML
        update_mode_var();
#endif
        ui_set_wifi_toggle(app_antenna_is_wifi_enabled());
        ui_tick();
        {
            int32_t brightness = app_gui_get_flow_var_int(
                FLOW_GLOBAL_VARIABLE_BRIGHTNESS_LEVEL,
                APP_GUI_DEFAULT_BRIGHTNESS
            );
            if (brightness != s_last_brightness) {
                s_last_brightness = brightness;
                (void)display_hal_set_brightness(&s_hal, (uint8_t)brightness);
            }
        }


        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

//创建 gui_task 任务
esp_err_t app_gui_start(void)
{
    static bool started = false;
    if (started) return ESP_OK;
    started = true;

    BaseType_t ok = xTaskCreate(gui_task, "gui", 8192, NULL, 5, NULL);
    return ok == pdPASS ? ESP_OK : ESP_FAIL;
}

//控制/查询屏幕开关状
void app_gui_screen_on(void)
{
    s_screen_on = true;
    if (s_panel_handle) {
        esp_lcd_panel_disp_on_off(s_panel_handle, true);
    }
}

void app_gui_screen_off(void)
{
    s_screen_on = false;
    if (s_panel_handle) {
        esp_lcd_panel_disp_on_off(s_panel_handle, false);
    }
}

bool app_gui_screen_is_on(void)
{
    return s_screen_on;
}

bool app_gui_is_ready(void)
{
    return s_ui_ready;
}
