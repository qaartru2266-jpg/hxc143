#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "app_antenna.h"
#include "app_time.h"

#if defined(CONFIG_BT_ENABLED) && CONFIG_BT_ENABLED
#define APP_ANTENNA_ENABLE_BLE 1
#else
#define APP_ANTENNA_ENABLE_BLE 0
#endif

#if APP_ANTENNA_ENABLE_BLE
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#endif

#define WIFI_SCAN_INTERVAL_MS 30000
#define WIFI_SSID "meizu"
#define WIFI_PASS "123456785"
#define SNTP_SERVER "ntp.aliyun.com"
#define BLE_DEVICE_NAME "shanruishanrui"
#define TIME_VALID_EPOCH 1700000000

static const char *TAG = "app_antenna";

static volatile bool s_wifi_enabled = false;
static volatile bool s_ble_enabled = false;
static bool s_wifi_inited = false;
static bool s_wifi_started = false;
static bool s_wifi_connected = false;
static bool s_wifi_connecting = false;
static bool s_wifi_event_registered = false;
#if APP_ANTENNA_ENABLE_BLE
static bool s_ble_inited = false;
static bool s_ble_bluedroid_inited = false;
static bool s_ble_adv_configured = false;
static bool s_ble_adv_active = false;
#endif
static bool s_sntp_enabled = false;
static bool s_sntp_inited = false;
static TaskHandle_t s_sntp_monitor_task = NULL;
static bool s_time_valid = false;
static app_antenna_time_source_t s_time_source = APP_ANTENNA_TIME_SOURCE_NONE;
static uint16_t s_last_scan_count = 0;

static TaskHandle_t s_wifi_task = NULL;
static bool s_netif_inited = false;
static esp_netif_t *s_wifi_netif = NULL;

#if APP_ANTENNA_ENABLE_BLE
static esp_ble_adv_params_t s_adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static esp_ble_adv_data_t s_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x20,
    .max_interval = 0x40,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT,
};
#endif

static void netif_init_once(void)
{
    if (s_netif_inited) {
        return;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }
    s_netif_inited = true;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            if (s_wifi_enabled) {
                s_wifi_connecting = true;
                esp_wifi_connect();
            }
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            s_wifi_connected = false;
            s_wifi_connecting = false;
            if (s_wifi_enabled) {
                ESP_LOGW(TAG, "Wi-Fi disconnected, retrying");
                s_wifi_connecting = true;
                esp_wifi_connect();
            }
            break;
        default:
            break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        s_wifi_connected = true;
        s_wifi_connecting = false;
        ESP_LOGI(TAG, "Wi-Fi connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        app_antenna_sntp_set_enabled(true);
    }
}

static void wifi_scan_once(void)
{
    if (!s_wifi_enabled || !s_wifi_started || s_wifi_connected || s_wifi_connecting) {
        return;
    }

    wifi_scan_config_t scan_config = {0};
    esp_err_t err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Wi-Fi scan start failed: %s", esp_err_to_name(err));
        return;
    }

    uint16_t ap_num = 0;
    err = esp_wifi_scan_get_ap_num(&ap_num);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Wi-Fi scan get AP num failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Scan done: %u AP found", (unsigned)ap_num);
    s_last_scan_count = ap_num;
    if (ap_num == 0) {
        return;
    }

    wifi_ap_record_t *ap_records = calloc(ap_num, sizeof(wifi_ap_record_t));
    if (!ap_records) {
        ESP_LOGW(TAG, "Wi-Fi scan alloc failed");
        return;
    }

    uint16_t ap_count = ap_num;
    err = esp_wifi_scan_get_ap_records(&ap_count, ap_records);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Wi-Fi scan get records failed: %s", esp_err_to_name(err));
        free(ap_records);
        return;
    }

    for (uint16_t i = 0; i < ap_count; ++i) {
        const wifi_ap_record_t *ap = &ap_records[i];
        ESP_LOGI(TAG, "SSID: %s, RSSI: %d dBm", (const char *)ap->ssid, ap->rssi);
    }

    free(ap_records);
}

static void wifi_scan_task(void *arg)
{
    (void)arg;
    while (1) {
        if (!s_wifi_enabled) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        wifi_scan_once();
        for (int i = 0; i < (WIFI_SCAN_INTERVAL_MS / 1000); ++i) {
            if (!s_wifi_enabled) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

#if APP_ANTENNA_ENABLE_BLE
static void ble_gap_event_handler(esp_gap_ble_cb_event_t event,
                                  esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        if (s_ble_enabled) {
            esp_ble_gap_start_advertising(&s_adv_params);
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGW(TAG, "BLE adv start failed: %d", param->adv_start_cmpl.status);
        } else {
            s_ble_adv_active = true;
            ESP_LOGI(TAG, "BLE advertising started");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGW(TAG, "BLE adv stop failed: %d", param->adv_stop_cmpl.status);
        } else {
            s_ble_adv_active = false;
            ESP_LOGI(TAG, "BLE advertising stopped");
        }
        break;
    default:
        break;
    }
}
#endif

static void wifi_init(void)
{
    if (s_wifi_inited) {
        return;
    }

    netif_init_once();
    s_wifi_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    if (!s_wifi_event_registered) {
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                   &wifi_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                   &wifi_event_handler, NULL));
        s_wifi_event_registered = true;
    }
    s_wifi_inited = true;
}

#if APP_ANTENNA_ENABLE_BLE
static void ble_init(void)
{
    if (s_ble_inited) {
        return;
    }

    esp_err_t err = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "BT classic mem release failed: %s", esp_err_to_name(err));
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    s_ble_inited = true;
}

static void ble_enable(void)
{
    ble_init();

    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_ENABLED) {
        ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    }

    if (!s_ble_bluedroid_inited) {
        ESP_ERROR_CHECK(esp_bluedroid_init());
        s_ble_bluedroid_inited = true;
    }
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
        ESP_ERROR_CHECK(esp_bluedroid_enable());
    }

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(ble_gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gap_set_device_name(BLE_DEVICE_NAME));

    if (!s_ble_adv_configured) {
        ESP_ERROR_CHECK(esp_ble_gap_config_adv_data(&s_adv_data));
        s_ble_adv_configured = true;
    } else {
        ESP_ERROR_CHECK(esp_ble_gap_start_advertising(&s_adv_params));
    }
}

static void ble_disable(void)
{
    if (s_ble_adv_active) {
        esp_err_t err = esp_ble_gap_stop_advertising();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "BLE adv stop failed: %s", esp_err_to_name(err));
        }
        s_ble_adv_active = false;
    }

    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED) {
        ESP_ERROR_CHECK(esp_bluedroid_disable());
    }

    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
        ESP_ERROR_CHECK(esp_bt_controller_disable());
    }
}
#else
static void ble_enable(void)
{
    ESP_LOGW(TAG, "BLE disabled in sdkconfig");
}

static void ble_disable(void)
{
}
#endif

static void sntp_time_sync_cb(struct timeval *tv)
{
    s_time_valid = true;
    s_time_source = APP_ANTENNA_TIME_SOURCE_SNTP;
    if (tv) {
        app_time_set_unix(tv->tv_sec, APP_TIME_SOURCE_SNTP);
    }
    app_time_save_now();
    ESP_LOGI(TAG, "SNTP time synced: %lld", tv ? (long long)tv->tv_sec : -1LL);
}

static void sntp_monitor_task(void *arg)
{
    (void)arg;
    const TickType_t wait_ticks = pdMS_TO_TICKS(15000);

    while (s_sntp_enabled) {
        esp_err_t err = esp_netif_sntp_sync_wait(wait_ticks);
        if (!s_sntp_enabled) {
            break;
        }

        if (err == ESP_OK) {
            unsigned int reachability = 0;
            if (esp_netif_sntp_reachability(0, &reachability) == ESP_OK) {
                ESP_LOGI(TAG, "SNTP sync ok, reachability: 0x%02x", reachability);
            } else {
                ESP_LOGI(TAG, "SNTP sync ok");
            }
            break;
        } else if (err == ESP_ERR_NOT_FINISHED) {
            ESP_LOGW(TAG, "SNTP sync in progress");
        } else if (err == ESP_ERR_TIMEOUT) {
            unsigned int reachability = 0;
            if (esp_netif_sntp_reachability(0, &reachability) == ESP_OK) {
                ESP_LOGW(TAG, "SNTP sync timeout, reachability: 0x%02x", reachability);
            } else {
                ESP_LOGW(TAG, "SNTP sync timeout");
            }
        } else {
            ESP_LOGW(TAG, "SNTP sync wait failed: %s", esp_err_to_name(err));
        }
    }

    s_sntp_monitor_task = NULL;
    vTaskDelete(NULL);
}

void app_antenna_set_wifi_enabled(bool enabled)
{
    if (enabled == s_wifi_enabled) {
        return;
    }

    if (enabled) {
        wifi_init();
        if (!s_wifi_started) {
            ESP_ERROR_CHECK(esp_wifi_start());
            s_wifi_started = true;
        }
        s_wifi_enabled = true;
        s_wifi_connecting = true;
        esp_wifi_connect();

        if (!s_wifi_task) {
            xTaskCreate(wifi_scan_task, "app_antenna_wifi", 4096, NULL, 5, &s_wifi_task);
        }
    } else {
        s_wifi_enabled = false;
        s_wifi_connected = false;
        s_wifi_connecting = false;
        app_antenna_sntp_set_enabled(false);
        if (s_wifi_started) {
            esp_wifi_disconnect();
            esp_err_t err = esp_wifi_stop();
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "Wi-Fi stop failed: %s", esp_err_to_name(err));
            }
            s_wifi_started = false;
        }
    }
}

void app_antenna_set_ble_enabled(bool enabled)
{
#if APP_ANTENNA_ENABLE_BLE
    if (enabled == s_ble_enabled) {
        return;
    }

    if (enabled) {
        s_ble_enabled = true;
        ble_enable();
    } else {
        s_ble_enabled = false;
        ble_disable();
    }
#else
    (void)enabled;
    if (enabled) {
        ESP_LOGW(TAG, "BLE disabled in sdkconfig");
    }
    s_ble_enabled = false;
#endif
}

bool app_antenna_is_wifi_enabled(void)
{
    return s_wifi_enabled;
}

bool app_antenna_is_ble_enabled(void)
{
#if APP_ANTENNA_ENABLE_BLE
    return s_ble_enabled;
#else
    return false;
#endif
}

void app_antenna_sntp_set_enabled(bool enabled)
{
    if (enabled == s_sntp_enabled) {
        return;
    }

    s_sntp_enabled = enabled;
    if (enabled) {
        netif_init_once();
        if (!s_sntp_inited) {
            ESP_LOGI(TAG, "SNTP init with server: %s", SNTP_SERVER);
            esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(SNTP_SERVER);
            config.sync_cb = sntp_time_sync_cb;
            config.wait_for_sync = true;
            esp_err_t err = esp_netif_sntp_init(&config);
            if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
                ESP_LOGW(TAG, "SNTP init failed: %s", esp_err_to_name(err));
                return;
            }
            s_sntp_inited = true;
            ESP_LOGI(TAG, "SNTP started");
        } else {
            ESP_LOGI(TAG, "SNTP restart");
            esp_netif_sntp_start();
        }

        if (!s_sntp_monitor_task) {
            xTaskCreate(sntp_monitor_task, "app_sntp_mon", 3072, NULL, 5, &s_sntp_monitor_task);
        }
    } else {
        if (s_sntp_inited) {
            ESP_LOGI(TAG, "SNTP stopped");
            esp_netif_sntp_deinit();
            s_sntp_inited = false;
        }
        if (s_sntp_monitor_task) {
            vTaskDelete(s_sntp_monitor_task);
            s_sntp_monitor_task = NULL;
        }
    }
}

bool app_antenna_sntp_is_enabled(void)
{
    return s_sntp_enabled;
}

bool app_antenna_time_is_valid(void)
{
    if (s_time_valid) {
        return true;
    }
    time_t now = time(NULL);
    return now > TIME_VALID_EPOCH;
}

bool app_antenna_time_get_local(struct tm *out_tm)
{
    if (!out_tm) {
        return false;
    }
    time_t now = time(NULL);
    if (now <= 0) {
        return false;
    }
    localtime_r(&now, out_tm);
    return true;
}

void app_antenna_time_set_timezone(const char *tz)
{
    if (!tz || !tz[0]) {
        return;
    }
    setenv("TZ", tz, 1);
    tzset();
}

void app_antenna_time_set_unix(time_t unix_time, app_antenna_time_source_t source)
{
    if (unix_time <= 0) {
        return;
    }
    struct timeval tv = {
        .tv_sec = unix_time,
        .tv_usec = 0,
    };
    settimeofday(&tv, NULL);
    s_time_valid = true;
    s_time_source = source;
}

app_antenna_time_source_t app_antenna_time_get_source(void)
{
    return s_time_source;
}

uint16_t app_antenna_get_last_scan_count(void)
{
    return s_last_scan_count;
}

void app_antenna_start(void)
{
    static bool s_started = false;
    if (s_started) {
        return;
    }
    s_started = true;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}
