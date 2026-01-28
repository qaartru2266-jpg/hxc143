#include "app_quiet.h"

#include <string.h>

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED
#include "driver/usb_serial_jtag.h"
#endif

#include "app_control.h"
#include "app_imu_calib.h"
#include "app_sd_export.h"

#define CONSOLE_BUF_SIZE 128

#if CONFIG_ESP_CONSOLE_UART_NUM >= 0
#define CONSOLE_UART ((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM)
#else
#define CONSOLE_UART UART_NUM_0
#endif

static TaskHandle_t s_console_task = NULL;

static const char *console_skip_ws(const char *s)
{
    if (!s) {
        return "";
    }
    while (*s == ' ' || *s == '\t') {
        ++s;
    }
    return s;
}

static void console_trim_line(char *line)
{
    if (!line) {
        return;
    }
    char *start = line;
    while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') {
        ++start;
    }
    if (start != line) {
        memmove(line, start, strlen(start) + 1);
    }
    size_t len = strlen(line);
    while (len > 0) {
        char c = line[len - 1];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            line[len - 1] = '\0';
            --len;
        } else {
            break;
        }
    }
}

static void console_write_line(const char *line)
{
    if (!line) {
        return;
    }
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED
    usb_serial_jtag_write_bytes(line, strlen(line), pdMS_TO_TICKS(1000));
    usb_serial_jtag_write_bytes("\r\n", 2, pdMS_TO_TICKS(1000));
#else
    uart_write_bytes(CONSOLE_UART, line, strlen(line));
    uart_write_bytes(CONSOLE_UART, "\r\n", 2);
    uart_wait_tx_done(CONSOLE_UART, pdMS_TO_TICKS(1000));
#endif
}

static bool console_handle_quiet(const char *line)
{
    const char *cmd = console_skip_ws(line);
    if (strncmp(cmd, "quiet", 5) != 0) {
        return false;
    }

    const char *arg = console_skip_ws(cmd + 5);
    if (strncmp(arg, "on", 2) == 0) {
        app_control_set_quiet(true);
        console_write_line("OK quiet=1");
        return true;
    }
    if (strncmp(arg, "off", 3) == 0) {
        app_control_set_quiet(false);
        console_write_line("OK quiet=0");
        return true;
    }

    console_write_line("ERR usage: quiet on|off");
    return true;
}

bool app_quiet_handle_line(const char *line)
{
    if (!line) {
        return false;
    }

    char buf[CONSOLE_BUF_SIZE];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    console_trim_line(buf);
    if (!buf[0]) {
        return false;
    }

    const char *cmd = console_skip_ws(buf);
    if (console_handle_quiet(cmd)) {
        return true;
    }
    if (app_sd_export_handle_line(cmd)) {
        return true;
    }
    if (app_imu_calib_handle_line(cmd)) {
        return true;
    }

    if (!app_control_is_quiet()) {
        console_write_line("ERR unknown cmd");
    }
    return false;
}

static void console_task(void *arg)
{
    (void)arg;
    char buf[CONSOLE_BUF_SIZE];
    size_t len = 0;

    while (1) {
        uint8_t ch = 0;
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED
        int rx = usb_serial_jtag_read_bytes(&ch, 1, pdMS_TO_TICKS(100));
#else
        int rx = uart_read_bytes(CONSOLE_UART, &ch, 1, pdMS_TO_TICKS(100));
#endif
        if (rx <= 0) {
            continue;
        }

        if (ch == '\r' || ch == '\n') {
            if (len > 0) {
                buf[len] = '\0';
                (void)app_quiet_handle_line(buf);
                len = 0;
            }
            continue;
        }

        if (len < sizeof(buf) - 1) {
            buf[len++] = (char)ch;
        } else {
            len = 0;
        }
    }
}

void app_quiet_start(void)
{
    if (s_console_task) {
        return;
    }

#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED
    usb_serial_jtag_driver_config_t cfg = {
        .rx_buffer_size = 256,
        .tx_buffer_size = 256,
    };
    usb_serial_jtag_driver_install(&cfg);
#else
    if (uart_driver_install(CONSOLE_UART, 256, 0, 0, NULL, 0) == ESP_OK) {
        uart_param_config(CONSOLE_UART, &(uart_config_t){
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
        });
    }
#endif

    xTaskCreate(console_task, "console_cmd", 4096, NULL, 5, &s_console_task);
}
