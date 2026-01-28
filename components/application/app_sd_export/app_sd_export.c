#include "app_sd_export.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sdkconfig.h"
#include "dirent.h"
#include "driver/uart.h"
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED
#include "driver/usb_serial_jtag.h"
#endif
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "app_control.h"
#include "app_sdcard.h"

#define TAG "app_sd_export"

#define EXPORT_ROOT "/sdcard"
#define EXPORT_BUF_SIZE 256
#define EXPORT_PATH_SIZE 512
#define EXPORT_IO_CHUNK 1024
#define EXPORT_WAIT_MS 2000
#define EXPORT_LOCK_TIMEOUT_MS 500
#define EXPORT_TX_TIMEOUT_US 200000
#define EXPORT_LIST_MAX_ITEMS 2000
#define EXPORT_LIST_MAX_US 2000000
#define EXPORT_YIELD_BLOCKS 8
#define EXPORT_DELETE_MAX_ITEMS 20000
#define EXPORT_DELETE_MAX_US 5000000
#define EXPORT_TX_CHUNK 256
#define EXPORT_TX_PROGRESS_TIMEOUT_US 3000000

#if CONFIG_ESP_CONSOLE_UART_NUM >= 0
#define EXPORT_UART ((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM)
#else
#define EXPORT_UART UART_NUM_0
#endif

static bool s_export_busy = false;
static bool s_transfer_active = false;
static bool s_debug_tx = false;
static QueueHandle_t s_export_queue = NULL;
static TaskHandle_t s_export_task = NULL;
static void export_wait_tx_done(void);
static bool export_tx_write(const uint8_t *data, size_t len);
static bool export_lock_fs(uint32_t timeout_ms);
static void export_unlock_fs(void);
static void export_task(void *arg);
static bool export_enqueue(int type, const char *arg);
static void export_run_with_pause(void (*fn)(const char *), const char *arg);
static void export_list_dir(const char *path);
static void export_bundle_dir(const char *path);
static void export_handle_get(const char *arg);
static void export_handle_pull_day(const char *arg);
static void export_handle_rm(const char *arg);
static void export_handle_rmdir(const char *arg);
static void export_handle_rm_rf(const char *arg);
static void export_handle_clear_all(const char *arg);

static void export_wait_tx_done(void)
{
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED
    (void)EXPORT_WAIT_MS;
#else
    uart_wait_tx_done(EXPORT_UART, pdMS_TO_TICKS(EXPORT_WAIT_MS));
#endif
}

static bool export_tx_write(const uint8_t *data, size_t len)
{
    if (!data || len == 0) {
        return true;
    }
    size_t sent = 0;
    int64_t last_progress = esp_timer_get_time();
    uint32_t attempts = 0;
    while (sent < len) {
        size_t remaining = len - sent;
        size_t chunk = remaining > EXPORT_TX_CHUNK ? EXPORT_TX_CHUNK : remaining;
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED
        size_t written = usb_serial_jtag_write_bytes(
            (const char *)data + sent, chunk, 0);
#else
        int written_bytes = uart_write_bytes(
            EXPORT_UART, (const char *)data + sent, chunk);
        size_t written = (written_bytes > 0) ? (size_t)written_bytes : 0;
#endif
        if (written > 0) {
            sent += written;
            last_progress = esp_timer_get_time();
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        attempts++;
        if ((attempts % EXPORT_YIELD_BLOCKS) == 0U) {
            vTaskDelay(pdMS_TO_TICKS(1));
#if CONFIG_ESP_TASK_WDT
            esp_task_wdt_reset();
#endif
        }
        if ((esp_timer_get_time() - last_progress) > EXPORT_TX_PROGRESS_TIMEOUT_US) {
            return false;
        }
    }
    return true;
}

static bool export_lock_fs(uint32_t timeout_ms)
{
    return app_sdcard_lock_fs(pdMS_TO_TICKS(timeout_ms));
}

static void export_unlock_fs(void)
{
    app_sdcard_unlock_fs();
}

static bool export_join_path(char *out, size_t out_sz, const char *base, const char *name)
{
    if (!out || !base || !name) {
        return false;
    }
    size_t base_len = strlen(base);
    size_t name_len = strlen(name);
    if (base_len + 1 + name_len + 1 > out_sz) {
        return false;
    }
    memcpy(out, base, base_len);
    out[base_len] = '/';
    memcpy(out + base_len + 1, name, name_len);
    out[base_len + 1 + name_len] = '\0';
    return true;
}

static size_t export_copy_trimmed(const char *input, char *out, size_t out_sz)
{
    if (!out || out_sz == 0) {
        return 0;
    }
    if (!input) {
        out[0] = '\0';
        return 0;
    }
    const char *start = input;
    while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') {
        ++start;
    }
    size_t len = strlen(start);
    while (len > 0) {
        char c = start[len - 1];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            --len;
            continue;
        }
        break;
    }
    if (len >= out_sz) {
        len = out_sz - 1;
    }
    memcpy(out, start, len);
    out[len] = '\0';
    return len;
}

static bool export_normalize_path(const char *input, char *out, size_t out_sz)
{
    char tmp[EXPORT_PATH_SIZE];
    size_t len = export_copy_trimmed(input, tmp, sizeof(tmp));
    if (len == 0) {
        snprintf(tmp, sizeof(tmp), "%s", EXPORT_ROOT);
        len = strlen(tmp);
    }

    char abs_path[EXPORT_PATH_SIZE];
    if (tmp[0] == '/') {
        snprintf(abs_path, sizeof(abs_path), "%s", tmp);
    } else {
        if (!export_join_path(abs_path, sizeof(abs_path), EXPORT_ROOT, tmp)) {
            return false;
        }
    }

    size_t out_len = 0;
    for (size_t i = 0; abs_path[i] != '\0' && out_len + 1 < out_sz; ++i) {
        if (abs_path[i] == '/' && out_len > 0 && out[out_len - 1] == '/') {
            continue;
        }
        out[out_len++] = abs_path[i];
    }
    out[out_len] = '\0';

    if (out_len > 1 && out[out_len - 1] == '/') {
        out[out_len - 1] = '\0';
        out_len--;
    }

    size_t root_len = strlen(EXPORT_ROOT);
    if (strncmp(out, EXPORT_ROOT, root_len) != 0) {
        return false;
    }
    if (out[root_len] != '\0' && out[root_len] != '/') {
        return false;
    }
    return true;
}

static const char *export_skip_ws(const char *s)
{
    if (!s) {
        return "";
    }
    while (*s == ' ' || *s == '\t') {
        ++s;
    }
    return s;
}

static void export_send_line(const char *fmt, ...)
{
    if (s_transfer_active) {
        return;
    }
    char buf[EXPORT_BUF_SIZE];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (len < 0) {
        return;
    }
    if (len >= (int)sizeof(buf)) {
        len = (int)sizeof(buf) - 1;
        buf[len] = '\0';
    }
    (void)export_tx_write((const uint8_t *)buf, (size_t)len);
    (void)export_tx_write((const uint8_t *)"\r\n", 2);
}

static bool export_send_line_checked(const char *line)
{
    if (!line) {
        return false;
    }
    size_t len = strlen(line);
    if (!export_tx_write((const uint8_t *)line, len)) {
        return false;
    }
    if (!export_tx_write((const uint8_t *)"\r\n", 2)) {
        return false;
    }
    return true;
}

typedef struct {
    uint32_t items;
    int64_t start_us;
    bool log_root;
} export_walk_t;

typedef enum {
    EXPORT_REQ_LS = 0,
    EXPORT_REQ_GET,
    EXPORT_REQ_PULL_DAY,
    EXPORT_REQ_PULL_DIR,
    EXPORT_REQ_PULL_ALL,
    EXPORT_REQ_RM,
    EXPORT_REQ_RMDIR,
    EXPORT_REQ_RM_RF,
    EXPORT_REQ_CLEAR_ALL
} export_req_type_t;

typedef struct {
    export_req_type_t type;
    char arg[EXPORT_PATH_SIZE];
} export_req_t;

typedef struct {
    uint32_t items;
    int64_t start_us;
} export_delete_t;

static bool export_delete_budget_ok(export_delete_t *ctx)
{
    if (!ctx) {
        return true;
    }
    ctx->items++;
    if (ctx->items >= EXPORT_DELETE_MAX_ITEMS ||
        (esp_timer_get_time() - ctx->start_us) > EXPORT_DELETE_MAX_US) {
        export_send_line("ERR RM_TIMEOUT");
        return false;
    }
    if ((ctx->items % 50U) == 0U) {
        vTaskDelay(pdMS_TO_TICKS(1));
#if CONFIG_ESP_TASK_WDT
        esp_task_wdt_reset();
#endif
    }
    return true;
}

static bool export_remove_file(const char *path, export_delete_t *ctx)
{
    if (!export_delete_budget_ok(ctx)) {
        return false;
    }
    if (!export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
        export_send_line("ERR FS_BUSY");
        return false;
    }
    int rc = unlink(path);
    int e = errno;
    export_unlock_fs();
    if (rc != 0) {
        export_send_line("ERR RM %d", e);
        return false;
    }
    return true;
}

static bool export_remove_dir(const char *path, export_delete_t *ctx)
{
    if (!export_delete_budget_ok(ctx)) {
        return false;
    }
    if (!export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
        export_send_line("ERR FS_BUSY");
        return false;
    }
    int rc = rmdir(path);
    int e = errno;
    export_unlock_fs();
    if (rc != 0) {
        export_send_line("ERR RMDIR %d", e);
        return false;
    }
    return true;
}

static bool export_delete_entry(const char *path, export_delete_t *ctx)
{
    if (!export_delete_budget_ok(ctx)) {
        return false;
    }

    struct stat st;
    if (!export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
        export_send_line("ERR FS_BUSY");
        return false;
    }
    int rc = stat(path, &st);
    export_unlock_fs();
    if (rc != 0) {
        export_send_line("ERR NOFILE %s", path);
        return false;
    }

    if (S_ISREG(st.st_mode)) {
        return export_remove_file(path, ctx);
    }
    if (!S_ISDIR(st.st_mode)) {
        export_send_line("ERR NOTDIR %s", path);
        return false;
    }

    if (!export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
        export_send_line("ERR FS_BUSY");
        return false;
    }
    DIR *dir = opendir(path);
    int e = errno;
    export_unlock_fs();
    if (!dir) {
        export_send_line("ERR OPENDIR %s %d", path, e);
        return false;
    }

    while (1) {
        if (!export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
            export_send_line("ERR FS_BUSY");
            closedir(dir);
            return false;
        }
        struct dirent *entry = readdir(dir);
        export_unlock_fs();
        if (!entry) {
            break;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (strncmp(entry->d_name, "SYSTEM~", 7) == 0) {
            continue;
        }

        char full_path[EXPORT_PATH_SIZE];
        if (!export_join_path(full_path, sizeof(full_path), path, entry->d_name)) {
            export_send_line("ERR PATH_TOO_LONG %s %s", path, entry->d_name);
            continue;
        }

        if (!export_delete_entry(full_path, ctx)) {
            closedir(dir);
            return false;
        }
    }

    if (export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
        closedir(dir);
        export_unlock_fs();
    } else {
        closedir(dir);
    }

    if (strcmp(path, EXPORT_ROOT) == 0) {
        return true;
    }
    return export_remove_dir(path, ctx);
}

static bool export_enqueue(int type, const char *arg)
{
    if (!s_export_queue) {
        s_export_queue = xQueueCreate(4, sizeof(export_req_t));
        if (!s_export_queue) {
            return false;
        }
    }
    if (!s_export_task) {
        if (xTaskCreate(export_task, "sd_export_task", 12288, NULL, 6, &s_export_task) != pdPASS) {
            return false;
        }
    }

    export_req_t req = {0};
    req.type = (export_req_type_t)type;
    export_copy_trimmed(arg, req.arg, sizeof(req.arg));
    if (xQueueSend(s_export_queue, &req, 0) != pdTRUE) {
        return false;
    }
    return true;
}

static void export_task(void *arg)
{
    (void)arg;
    export_req_t req;
    while (1) {
        if (xQueueReceive(s_export_queue, &req, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        switch (req.type) {
        case EXPORT_REQ_LS:
            export_run_with_pause(export_list_dir, req.arg);
            break;
        case EXPORT_REQ_GET:
            export_run_with_pause(export_handle_get, req.arg);
            break;
        case EXPORT_REQ_PULL_DAY:
            export_run_with_pause(export_handle_pull_day, req.arg);
            break;
        case EXPORT_REQ_PULL_DIR:
            export_run_with_pause(export_bundle_dir, req.arg);
            break;
        case EXPORT_REQ_PULL_ALL:
            export_run_with_pause(export_bundle_dir, EXPORT_ROOT);
            break;
        case EXPORT_REQ_RM:
            export_run_with_pause(export_handle_rm, req.arg);
            break;
        case EXPORT_REQ_RMDIR:
            export_run_with_pause(export_handle_rmdir, req.arg);
            break;
        case EXPORT_REQ_RM_RF:
            export_run_with_pause(export_handle_rm_rf, req.arg);
            break;
        case EXPORT_REQ_CLEAR_ALL:
            export_run_with_pause(export_handle_clear_all, req.arg);
            break;
        default:
            break;
        }
    }
}

static bool export_send_file_entry(const char *path, bool bundle_mode)
{
    if (!export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
        export_send_line("ERR FS_BUSY");
        return false;
    }
    struct stat st;
    if (stat(path, &st) != 0) {
        export_unlock_fs();
        export_send_line("ERR NOFILE %s", path);
        return false;
    }
    if (!S_ISREG(st.st_mode)) {
        export_unlock_fs();
        export_send_line("ERR NOTFILE %s", path);
        return false;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        export_unlock_fs();
        export_send_line("ERR OPEN %s %d", path, errno);
        return false;
    }
    export_unlock_fs();

    (void)bundle_mode;
    s_transfer_active = true;
    char header[EXPORT_BUF_SIZE];
    snprintf(header, sizeof(header), "FILE_BEGIN %s %ld", path, (long)st.st_size);
    if (!export_send_line_checked(header)) {
        (void)export_send_line_checked("ERR TX_TIMEOUT");
        s_transfer_active = false;
        if (export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
            fclose(f);
            export_unlock_fs();
        } else {
            fclose(f);
        }
        return false;
    }

    char io_buf[EXPORT_IO_CHUNK];
    size_t remaining = (size_t)st.st_size;
    size_t total_sent = 0;
    uint32_t blocks = 0;
    uint32_t debug_acc = 0;
    bool ok = true;
    while (remaining > 0) {
        size_t to_read = remaining > sizeof(io_buf) ? sizeof(io_buf) : remaining;
        if (!export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
            (void)export_send_line_checked("ERR FS_BUSY");
            ok = false;
            break;
        }
        size_t read_bytes = fread(io_buf, 1, to_read, f);
        int read_errno = errno;
        export_unlock_fs();
        if (read_bytes == 0) {
            if (feof(f)) {
                (void)export_send_line_checked("ERR READ_EOF");
            } else {
                char err_line[EXPORT_BUF_SIZE];
                snprintf(err_line, sizeof(err_line), "ERR READ %d", read_errno);
                (void)export_send_line_checked(err_line);
            }
            ok = false;
            break;
        }

        if (!export_tx_write((const uint8_t *)io_buf, read_bytes)) {
            (void)export_send_line_checked("ERR TX_TIMEOUT");
            ok = false;
            break;
        }
        remaining -= read_bytes;
        total_sent += read_bytes;
        blocks++;
        debug_acc += (uint32_t)read_bytes;
        if (s_debug_tx && debug_acc >= 1024U) {
            char dbg[EXPORT_BUF_SIZE];
            snprintf(dbg, sizeof(dbg), "TX sent=%lu/%lu last_write=%u",
                     (unsigned long)total_sent,
                     (unsigned long)st.st_size,
                     (unsigned)read_bytes);
            (void)export_send_line_checked(dbg);
            debug_acc = 0;
        }
        if ((blocks % EXPORT_YIELD_BLOCKS) == 0U) {
            vTaskDelay(pdMS_TO_TICKS(1));
#if CONFIG_ESP_TASK_WDT
            esp_task_wdt_reset();
#endif
        }
    }

    if (export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
        fclose(f);
        export_unlock_fs();
    } else {
        fclose(f);
    }

    if (!ok || total_sent != (size_t)st.st_size) {
        s_transfer_active = false;
        return false;
    }
    if (!export_send_line_checked("FILE_END")) {
        (void)export_send_line_checked("ERR TX_TIMEOUT");
        s_transfer_active = false;
        return false;
    }
    s_transfer_active = false;
    export_wait_tx_done();
    return true;
}

static void export_list_dir(const char *path)
{
    char norm_path[EXPORT_PATH_SIZE];
    if (!export_normalize_path(path, norm_path, sizeof(norm_path))) {
        export_send_line("ERR PATH %s", path ? path : "");
        return;
    }

    export_send_line("LS enter path=%s", norm_path);
    export_send_line("opendir start");
    if (!export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
        export_send_line("ERR FS_BUSY");
        return;
    }
    DIR *dir = opendir(norm_path);
    if (!dir) {
        export_unlock_fs();
        export_send_line("opendir fail errno=%d", errno);
        export_send_line("ERR OPENDIR %s %d", norm_path, errno);
        return;
    }
    export_unlock_fs();
    export_send_line("opendir ok");

    export_send_line("BEGIN_LIST %s", norm_path);
    int64_t start_us = esp_timer_get_time();
    uint32_t items = 0;
    struct dirent *entry = NULL;
    while (1) {
        if (!export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
            export_send_line("ERR FS_BUSY");
            break;
        }
        entry = readdir(dir);
        export_unlock_fs();
        if (!entry) {
            break;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        ++items;
        if ((items % 50U) == 0U) {
            export_send_line("readdir loop count=%u", (unsigned)items);
        }
        if (items >= EXPORT_LIST_MAX_ITEMS ||
            (esp_timer_get_time() - start_us) > EXPORT_LIST_MAX_US) {
            export_send_line("ERR LS_TIMEOUT");
            break;
        }

        char full_path[EXPORT_PATH_SIZE];
        if (!export_join_path(full_path, sizeof(full_path), norm_path, entry->d_name)) {
            export_send_line("ERR PATH_TOO_LONG %s %s", norm_path, entry->d_name);
            continue;
        }

        if (!export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
            export_send_line("ERR FS_BUSY");
            break;
        }
        struct stat st;
        if (stat(full_path, &st) != 0) {
            export_unlock_fs();
            continue;
        }
        export_unlock_fs();
        if (S_ISDIR(st.st_mode)) {
            export_send_line("D %s/", entry->d_name);
        } else if (S_ISREG(st.st_mode)) {
            export_send_line("F %s %ld", entry->d_name, (long)st.st_size);
        }
    }
    if (export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
        closedir(dir);
        export_unlock_fs();
    } else {
        closedir(dir);
    }
    export_send_line("LS done items=%u", (unsigned)items);
    export_send_line("END_LIST");
}

static bool export_send_dir_recursive(const char *path, export_walk_t *walk, bool log_root)
{
    if (log_root) {
        export_send_line("LS enter path=%s", path);
        export_send_line("opendir start");
    }
    if (!export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
        export_send_line("ERR FS_BUSY");
        return false;
    }
    DIR *dir = opendir(path);
    if (!dir) {
        export_unlock_fs();
        if (log_root) {
            export_send_line("opendir fail errno=%d", errno);
        }
        export_send_line("ERR OPENDIR %s %d", path, errno);
        return false;
    }
    export_unlock_fs();
    if (log_root) {
        export_send_line("opendir ok");
    }

    struct dirent *entry = NULL;
    while (1) {
        if (!export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
            export_send_line("ERR FS_BUSY");
            if (export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
                closedir(dir);
                export_unlock_fs();
            } else {
                closedir(dir);
            }
            return false;
        }
        entry = readdir(dir);
        export_unlock_fs();
        if (!entry) {
            break;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (strncmp(entry->d_name, "SYSTEM~", 7) == 0) {
            continue;
        }

        if (walk) {
            walk->items++;
            if (log_root && (walk->items % 50U) == 0U) {
                export_send_line("readdir loop count=%u", (unsigned)walk->items);
            }
            if (walk->items >= EXPORT_LIST_MAX_ITEMS ||
                (esp_timer_get_time() - walk->start_us) > EXPORT_LIST_MAX_US) {
                export_send_line("ERR LS_TIMEOUT");
                closedir(dir);
                return false;
            }
        }

        char full_path[EXPORT_PATH_SIZE];
        if (!export_join_path(full_path, sizeof(full_path), path, entry->d_name)) {
            export_send_line("ERR PATH_TOO_LONG %s %s", path, entry->d_name);
            continue;
        }

        if (!export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
            export_send_line("ERR FS_BUSY");
            if (export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
                closedir(dir);
                export_unlock_fs();
            } else {
                closedir(dir);
            }
            return false;
        }
        struct stat st;
        if (stat(full_path, &st) != 0) {
            export_unlock_fs();
            continue;
        }
        export_unlock_fs();
        if (S_ISDIR(st.st_mode)) {
            if (!export_send_dir_recursive(full_path, walk, false)) {
                if (export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
                    closedir(dir);
                    export_unlock_fs();
                } else {
                    closedir(dir);
                }
                return false;
            }
        } else if (S_ISREG(st.st_mode)) {
            if (!export_send_file_entry(full_path, true)) {
                if (export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
                    closedir(dir);
                    export_unlock_fs();
                } else {
                    closedir(dir);
                }
                return false;
            }
        }
        if (walk) {
            walk->start_us = esp_timer_get_time();
        }
    }

    if (export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
        closedir(dir);
        export_unlock_fs();
    } else {
        closedir(dir);
    }
    return true;
}

static void export_bundle_dir(const char *path)
{
    char norm_path[EXPORT_PATH_SIZE];
    if (!export_normalize_path(path, norm_path, sizeof(norm_path))) {
        export_send_line("ERR PATH %s", path ? path : "");
        return;
    }

    export_send_line("BEGIN_BUNDLE %s", norm_path);
    export_walk_t walk = {
        .items = 0,
        .start_us = esp_timer_get_time(),
        .log_root = true
    };
    if (!export_send_dir_recursive(norm_path, &walk, true)) {
        export_send_line("LS done items=%u", (unsigned)walk.items);
        export_send_line("END_BUNDLE");
        return;
    }
    export_send_line("LS done items=%u", (unsigned)walk.items);
    export_send_line("END_BUNDLE");
}

static void export_handle_get(const char *arg)
{
    char norm_path[EXPORT_PATH_SIZE];
    if (!export_normalize_path(arg, norm_path, sizeof(norm_path))) {
        export_send_line("ERR PATH %s", arg ? arg : "");
        return;
    }
    (void)export_send_file_entry(norm_path, false);
}

static void export_handle_pull_day(const char *arg)
{
    char day_buf[EXPORT_PATH_SIZE];
    export_copy_trimmed(arg, day_buf, sizeof(day_buf));
    const char *day = day_buf;
    const char *last = strrchr(day_buf, '/');
    if (last) {
        day = last + 1;
    }
    if (strlen(day) != 8) {
        export_send_line("ERR ARG");
        return;
    }
    char path[EXPORT_PATH_SIZE];
    if (!export_join_path(path, sizeof(path), EXPORT_ROOT, day)) {
        export_send_line("ERR PATH_TOO_LONG %s %s", EXPORT_ROOT, day);
        return;
    }
    export_bundle_dir(path);
}

static void export_handle_rm(const char *arg)
{
    char norm_path[EXPORT_PATH_SIZE];
    if (!export_normalize_path(arg, norm_path, sizeof(norm_path))) {
        export_send_line("ERR PATH %s", arg ? arg : "");
        return;
    }
    if (strcmp(norm_path, EXPORT_ROOT) == 0) {
        export_send_line("ERR ARG");
        return;
    }

    struct stat st;
    if (!export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
        export_send_line("ERR FS_BUSY");
        return;
    }
    int rc = stat(norm_path, &st);
    export_unlock_fs();
    if (rc != 0) {
        export_send_line("ERR NOFILE %s", norm_path);
        return;
    }
    if (S_ISDIR(st.st_mode)) {
        export_send_line("ERR ISDIR %s", norm_path);
        return;
    }
    export_delete_t ctx = {
        .items = 0,
        .start_us = esp_timer_get_time()
    };
    if (export_remove_file(norm_path, &ctx)) {
        export_send_line("OK RM %s", norm_path);
    }
}

static void export_handle_rmdir(const char *arg)
{
    char norm_path[EXPORT_PATH_SIZE];
    if (!export_normalize_path(arg, norm_path, sizeof(norm_path))) {
        export_send_line("ERR PATH %s", arg ? arg : "");
        return;
    }
    if (strcmp(norm_path, EXPORT_ROOT) == 0) {
        export_send_line("ERR ARG");
        return;
    }
    export_delete_t ctx = {
        .items = 0,
        .start_us = esp_timer_get_time()
    };
    if (export_remove_dir(norm_path, &ctx)) {
        export_send_line("OK RMDIR %s", norm_path);
    }
}

static void export_handle_rm_rf(const char *arg)
{
    char norm_path[EXPORT_PATH_SIZE];
    if (!export_normalize_path(arg, norm_path, sizeof(norm_path))) {
        export_send_line("ERR PATH %s", arg ? arg : "");
        return;
    }
    if (strcmp(norm_path, EXPORT_ROOT) == 0) {
        export_send_line("ERR ARG");
        return;
    }
    export_delete_t ctx = {
        .items = 0,
        .start_us = esp_timer_get_time()
    };
    if (export_delete_entry(norm_path, &ctx)) {
        export_send_line("OK RM_RF %s", norm_path);
    }
}

static void export_handle_clear_all(const char *arg)
{
    char token[EXPORT_PATH_SIZE];
    export_copy_trimmed(arg, token, sizeof(token));
    if (strcmp(token, "CONFIRM") != 0) {
        export_send_line("ERR CONFIRM");
        return;
    }

    export_delete_t ctx = {
        .items = 0,
        .start_us = esp_timer_get_time()
    };

    if (!export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
        export_send_line("ERR FS_BUSY");
        return;
    }
    DIR *dir = opendir(EXPORT_ROOT);
    int e = errno;
    export_unlock_fs();
    if (!dir) {
        export_send_line("ERR OPENDIR %s %d", EXPORT_ROOT, e);
        return;
    }

    while (1) {
        if (!export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
            export_send_line("ERR FS_BUSY");
            closedir(dir);
            return;
        }
        struct dirent *entry = readdir(dir);
        export_unlock_fs();
        if (!entry) {
            break;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (strncmp(entry->d_name, "SYSTEM~", 7) == 0) {
            continue;
        }

        char full_path[EXPORT_PATH_SIZE];
        if (!export_join_path(full_path, sizeof(full_path), EXPORT_ROOT, entry->d_name)) {
            export_send_line("ERR PATH_TOO_LONG %s %s", EXPORT_ROOT, entry->d_name);
            continue;
        }
        if (!export_delete_entry(full_path, &ctx)) {
            closedir(dir);
            return;
        }
    }

    if (export_lock_fs(EXPORT_LOCK_TIMEOUT_MS)) {
        closedir(dir);
        export_unlock_fs();
    } else {
        closedir(dir);
    }

    export_send_line("OK CLEAR_ALL");
}

static void export_run_with_pause(void (*fn)(const char *), const char *arg)
{
    bool restore_quiet = false;
    bool stopped = false;
    if (s_export_busy) {
        export_send_line("ERR BUSY");
        return;
    }
    s_export_busy = true;

    restore_quiet = app_control_is_quiet();
    app_control_set_quiet(true);

    app_control_stop_all();
    stopped = true;
    vTaskDelay(pdMS_TO_TICKS(200));

    if (s_transfer_active) {
        s_transfer_active = false;
    }

    fn(arg);

    if (stopped) {
        app_control_resume_all();
    }
    app_control_set_quiet(restore_quiet);
    s_export_busy = false;
}

bool app_sd_export_handle_line(const char *line)
{
    if (!line) {
        return false;
    }

    const char *p = export_skip_ws(line);
    if (strncmp(p, "sd_help", 7) == 0) {
        export_send_line("OK sd_ls [path]");
        export_send_line("OK sd_get <path>");
        export_send_line("OK sd_pull_day <YYYYMMDD>");
        export_send_line("OK sd_pull_dir <path>");
        export_send_line("OK sd_pull_all");
        export_send_line("OK sd_rm <path>");
        export_send_line("OK sd_rmdir <path>");
        export_send_line("OK sd_rm_rf <path>");
        export_send_line("OK sd_clear_all CONFIRM");
        export_send_line("OK sd_debug on|off");
        return true;
    }

    if (strncmp(p, "sd_debug", 8) == 0) {
        const char *arg = export_skip_ws(p + 8);
        if (strncmp(arg, "on", 2) == 0) {
            s_debug_tx = true;
            export_send_line("OK sd_debug on");
        } else if (strncmp(arg, "off", 3) == 0) {
            s_debug_tx = false;
            export_send_line("OK sd_debug off");
        } else {
            export_send_line("ERR ARG");
        }
        return true;
    }

    if (strncmp(p, "sd_ls", 5) == 0) {
        const char *arg = export_skip_ws(p + 5);
        if (!export_enqueue(EXPORT_REQ_LS, arg)) {
            export_send_line("ERR BUSY");
            return true;
        }
        export_send_line("OK QUEUED");
        return true;
    }

    if (strncmp(p, "sd_get", 6) == 0) {
        if (!export_enqueue(EXPORT_REQ_GET, p + 6)) {
            export_send_line("ERR BUSY");
            return true;
        }
        export_send_line("OK QUEUED");
        return true;
    }

    if (strncmp(p, "sd_pull_day", 11) == 0) {
        if (!export_enqueue(EXPORT_REQ_PULL_DAY, p + 11)) {
            export_send_line("ERR BUSY");
            return true;
        }
        export_send_line("OK QUEUED");
        return true;
    }

    if (strncmp(p, "sd_pull_dir", 11) == 0) {
        if (!export_enqueue(EXPORT_REQ_PULL_DIR, export_skip_ws(p + 11))) {
            export_send_line("ERR BUSY");
            return true;
        }
        export_send_line("OK QUEUED");
        return true;
    }

    if (strncmp(p, "sd_pull_all", 11) == 0) {
        if (!export_enqueue(EXPORT_REQ_PULL_ALL, EXPORT_ROOT)) {
            export_send_line("ERR BUSY");
            return true;
        }
        export_send_line("OK QUEUED");
        return true;
    }

    if (strncmp(p, "sd_rm_rf", 8) == 0) {
        if (!export_enqueue(EXPORT_REQ_RM_RF, p + 8)) {
            export_send_line("ERR BUSY");
            return true;
        }
        export_send_line("OK QUEUED");
        return true;
    }

    if (strncmp(p, "sd_rmdir", 8) == 0) {
        if (!export_enqueue(EXPORT_REQ_RMDIR, p + 8)) {
            export_send_line("ERR BUSY");
            return true;
        }
        export_send_line("OK QUEUED");
        return true;
    }

    if (strncmp(p, "sd_rm", 5) == 0) {
        if (!export_enqueue(EXPORT_REQ_RM, p + 5)) {
            export_send_line("ERR BUSY");
            return true;
        }
        export_send_line("OK QUEUED");
        return true;
    }

    if (strncmp(p, "sd_clear_all", 12) == 0) {
        if (!export_enqueue(EXPORT_REQ_CLEAR_ALL, p + 12)) {
            export_send_line("ERR BUSY");
            return true;
        }
        export_send_line("OK QUEUED");
        return true;
    }

    return false;
}
