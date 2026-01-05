#pragma once
#include <stdint.h>
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "freertos/semphr.h"


typedef struct {
    esp_lcd_panel_handle_t     panel;      // 由 esp_lcd_new_panel_sh8601() 产生 LCD 面板句柄（esp_lcd 面板驱动对象）。
    esp_lcd_panel_io_handle_t  io;         // IO 句柄（new_panel_io_spi 后得到） ， QSPI IO 句柄（用来跟屏幕通信）。
    SemaphoreHandle_t          trans_done; // DMA 传输完成信号量 ， DMA 传输完成信号（用于同步刷新完成）。
    SemaphoreHandle_t          te_sema;    // TE semaphore TE 同步信号（屏的撕裂同步中断）。

    uint16_t                   hor_res;//屏幕分辨率
    uint16_t                   ver_res;//屏幕分辨率
} display_hal_t;

// 初始化 SH8601（QSPI），点亮屏并返回 panel 句柄与分辨率
esp_err_t display_hal_init(display_hal_t *out);

// Optional: draw a simple test pattern (full-screen fill + 5 white lines)
esp_err_t display_hal_test_once(display_hal_t *hal);

