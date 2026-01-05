// components/display/display_hal.c
#include "display_hal.h"

#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_sh8601.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "display_hal";

// ====== 分辨率（466x466）======
#define LCD_H_RES         466
#define LCD_V_RES         466

// ====== 像素格式（SH8601 选 RGB888）======
#define LCD_BIT_PER_PIXEL 24

// ====== QSPI 引脚（确保与 SD 卡不冲突）======
#define LCD_HOST          SPI3_HOST
#define PIN_LCD_CS        (GPIO_NUM_40)
#define PIN_LCD_SCLK      (GPIO_NUM_39)
#define PIN_LCD_SIO0      (GPIO_NUM_42)
#define PIN_LCD_SIO1      (GPIO_NUM_38)
#define PIN_LCD_SIO2      (GPIO_NUM_48)
#define PIN_LCD_SIO3      (GPIO_NUM_47)
#define PIN_LCD_RST       (GPIO_NUM_21)
#define PIN_LCD_TE        (GPIO_NUM_41)

// 可选：背光脚（若无就保持 -1）
#define PIN_LCD_BK        (-1)

// QSPI clock for panel IO (Hz). Try 60MHz first, back off if unstable.
#define LCD_QSPI_PCLK_HZ  (60 * 1000 * 1000)


// ====== SH8601 初始化命令表 ======
// 0x77: 像素格式；0x55 => RGB888（与 bits_per_pixel=24 对应），给 SH8601 面板的初始化命令表，包括像素格式、显示区域、开屏指令等。
static const sh8601_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xFE, (uint8_t[]){0x20}, 1, 0},
    {0xF4, (uint8_t[]){0x5A}, 1, 0},
    {0xF5, (uint8_t[]){0x59}, 1, 0},
    {0xFE, (uint8_t[]){0x20}, 1, 0},
    {0x19, (uint8_t[]){0x10}, 1, 0},
    {0x1C, (uint8_t[]){0xA0}, 1, 0},
    {0xFE, (uint8_t[]){0x00}, 1, 0},
    {0xC4, (uint8_t[]){0x80}, 1, 0},
    {0x3A, (uint8_t[]){0x77}, 1, 0},
    {0x36, (uint8_t[]){0x08}, 1, 0},
    {0x35, (uint8_t[]){0x00}, 1, 0},
    {0x53, (uint8_t[]){0x20}, 1, 0},
    {0x51, (uint8_t[]){0xFF}, 1, 0},
    {0x63, (uint8_t[]){0xFF}, 1, 0},
    {0x2A, (uint8_t[]){0x00, 0x06, 0x01, 0xD7}, 4, 0},
    {0x2B, (uint8_t[]){0x00, 0x00, 0x01, 0xD1}, 4, 0},
    {0x11, NULL, 0, 60},
    {0x29, NULL, 0, 0},
};

// ---- 传输完成回调（中断上下文）---- DMA 传输完成中断回调，释放 trans_done 信号量，告诉上层“本次传输结束”。
static bool on_color_trans_done(esp_lcd_panel_io_handle_t io,
                                esp_lcd_panel_io_event_data_t *edata,
                                void *user_ctx)
{
    display_hal_t *hal = (display_hal_t *)user_ctx;
    BaseType_t hp_woken = pdFALSE;
    if (hal && hal->trans_done) {
        xSemaphoreGiveFromISR(hal->trans_done, &hp_woken);
    }
    return hp_woken == pdTRUE;
}
// TE 引脚中断服务函数，释放 te_sema，用于“等到屏幕刷新时机再画”。
static void IRAM_ATTR on_te_isr(void *arg)
{
    display_hal_t *hal = (display_hal_t *)arg;
    BaseType_t hp_woken = pdFALSE;
    if (hal && hal->te_sema) {
        xSemaphoreGiveFromISR(hal->te_sema, &hp_woken);
    }
    if (hp_woken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

esp_err_t display_hal_init(display_hal_t *out)//完整硬件初始化流程
{
    ESP_RETURN_ON_FALSE(out, ESP_ERR_INVALID_ARG, TAG, "out is NULL");
    memset(out, 0, sizeof(*out));

    // 1) 初始化 QSPI 总线（容忍“已初始化”）
    ESP_LOGI(TAG, "Initialize QSPI bus");
    spi_bus_config_t buscfg = {0};
    buscfg.sclk_io_num     = PIN_LCD_SCLK;
    buscfg.data0_io_num    = PIN_LCD_SIO0;
    buscfg.data1_io_num    = PIN_LCD_SIO1;
    buscfg.data2_io_num    = PIN_LCD_SIO2;
    buscfg.data3_io_num    = PIN_LCD_SIO3;
    // 行缓冲最大传输长度：设大一点即可
    buscfg.max_transfer_sz = LCD_H_RES * 80 * (LCD_BIT_PER_PIXEL / 8);
    buscfg.flags           = SPICOMMON_BUSFLAG_QUAD;

    esp_err_t ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "SPI bus already initialized, reuse it.");
        ret = ESP_OK;
    }
    ESP_RETURN_ON_ERROR(ret, TAG, "spi_bus_initialize failed");

    // 2) 面板 IO（QSPI）
    ESP_LOGI(TAG, "Install panel IO (QSPI)");
    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_spi_config_t io_cfg =
        SH8601_PANEL_IO_QSPI_CONFIG(PIN_LCD_CS,
                                    NULL,   // 暂不在 cfg 里放回调
                                    NULL);  // user_ctx
    io_cfg.pclk_hz = LCD_QSPI_PCLK_HZ;
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_cfg, &io),
        TAG, "esp_lcd_new_panel_io_spi failed");
    out->io = io;

    // 3) 供应商配置 + 设备配置
    sh8601_vendor_config_t vendor_cfg = {
        .init_cmds      = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
        .flags = { .use_qspi_interface = 1 },
    };

    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_endian     = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
        .vendor_config  = &vendor_cfg,
    };

    ESP_LOGI(TAG, "Install SH8601 panel driver");
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_sh8601(io, &panel_cfg, &panel),
                        TAG, "esp_lcd_new_panel_sh8601 failed");

    // 4) 复位 + 初始化 + 开屏
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel), TAG, "panel_reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel),  TAG, "panel_init failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel, true),
                        TAG, "panel_disp_on_off failed");

 
    out->panel   = panel;
    out->hor_res = LCD_H_RES;
    out->ver_res = LCD_V_RES;



    // 5) 传输完成信号量 + 回调注册（DMA 同步）
    out->trans_done = xSemaphoreCreateBinary();

    ESP_RETURN_ON_FALSE(out->trans_done != NULL, ESP_ERR_NO_MEM, TAG, "no mem for semaphore");

    esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = on_color_trans_done,
    };
    ESP_RETURN_ON_ERROR(
        esp_lcd_panel_io_register_event_callbacks(out->io, &cbs, out),
        TAG, "register_event_callbacks failed");

// TE sync (optional)
    out->te_sema = xSemaphoreCreateBinary();
    ESP_RETURN_ON_FALSE(out->te_sema != NULL, ESP_ERR_NO_MEM, TAG, "no mem for te semaphore");

    gpio_config_t te_cfg = {
        .pin_bit_mask = (1ULL << PIN_LCD_TE),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_POSEDGE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&te_cfg), TAG, "gpio_config(te) failed");

    esp_err_t isr_ret = gpio_install_isr_service(0);
    if (isr_ret != ESP_OK && isr_ret != ESP_ERR_INVALID_STATE) {
        ESP_RETURN_ON_ERROR(isr_ret, TAG, "gpio_install_isr_service failed");
    }
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(PIN_LCD_TE, on_te_isr, out),
                        TAG, "gpio_isr_handler_add failed");

    // 6) 背光（若有，用了amoled，其实没这部分，但是万一要用lcd先留着）
#if (PIN_LCD_BK >= 0)
    {
        gpio_config_t bk = {
            .pin_bit_mask = (1ULL << PIN_LCD_BK),
            .mode         = GPIO_MODE_OUTPUT,
            .pull_up_en   = 0,
            .pull_down_en = 0,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&bk), TAG, "gpio_config(bk) failed");
        gpio_set_level(PIN_LCD_BK, 1);
    }
#endif

    ESP_LOGI(TAG, "Display OK, %dx%d", out->hor_res, out->ver_res);
    return ESP_OK;
}

// ---- 阻塞等待一次 DMA 完成（配合 on_color_trans_done）----
static esp_err_t wait_flush(display_hal_t *hal, uint32_t timeout_ms)
{
    if (!hal || !hal->trans_done) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(hal->trans_done, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

// ---- 只画一次的测试：整屏填色 + 5 条白线 ----
esp_err_t display_hal_test_once(display_hal_t *hal)
{
    if (!hal || !hal->panel) return ESP_ERR_INVALID_STATE;

    // 1) 整屏纯色（深绿）
    static uint8_t line[LCD_H_RES * 3]; // RGB888
    for (int x = 0; x < LCD_H_RES; ++x) {
        line[3*x+0] = 0x00; // R
        line[3*x+1] = 0x60; // G
        line[3*x+2] = 0x00; // B
    }
    for (int y = 0; y < LCD_V_RES; ++y) {
        esp_err_t e = esp_lcd_panel_draw_bitmap(hal->panel, 0, y, LCD_H_RES, y+1, line);
        if (e != ESP_OK) return e;
        (void)wait_flush(hal, 200);
    }

    // 2) 画 5 条水平白线
    memset(line, 0xFF, sizeof(line));
    for (int k = 1; k <= 5; ++k) {
        int y = (LCD_V_RES * k) / 6;
        esp_err_t e = esp_lcd_panel_draw_bitmap(hal->panel, 0, y, LCD_H_RES, y+1, line);
        if (e != ESP_OK) return e;
        (void)wait_flush(hal, 200);
    }

    ESP_LOGI(TAG, "test_once done");
    return ESP_OK;
}
