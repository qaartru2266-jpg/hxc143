#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#include "app_datalog.h"

#define DEV_STOP_ALL_ON_BOOT 0
#define DEV_DISABLE_DEFAULT_RAW_ON_BOOT 1
#define DEV_ENABLE_WIFI_ON_BOOT 1

#if DEV_STOP_ALL_ON_BOOT
#include "app_control.h"
#endif
#include "app_axis6.h"
#include "app_gps.h"
#include "app_gui.h"
#include "app_vibration.h"
#include "app_power.h"
#include "app_bat_adc.h"
#include "app_state.h"
#include "app_antenna.h"
#include "app_time.h"
#include "app_imu_calib.h"
#include "app_quiet.h"
#include "sdkconfig.h"

#if CONFIG_JOFTMODE_ENABLE_ML
#include "app_ml.h"
#endif



void app_main(void)
{
    esp_reset_reason_t reason = esp_reset_reason();
    ESP_LOGI("app_main", "reset reason=%d", (int)reason);
    app_state_init();
    app_vibration_init();
    app_time_start();
    app_imu_calib_init();
    app_quiet_start();

    app_axis6_start();
    app_gps_start();

    app_power_start();
    app_bat_adc_start();

    if (DEV_DISABLE_DEFAULT_RAW_ON_BOOT) {
        app_datalog_set_ml_enabled(false);
    }
    app_datalog_start();
    printf("MODE: DATA_COLLECTION (MOCK disabled)\n");
#if DEV_DISABLE_DEFAULT_RAW_ON_BOOT
    app_datalog_set_default_raw_enabled(false);
    printf("DATALOG: default raw logging disabled\n");
#else
    printf("DATALOG: raw logging enabled\n");
#endif

#if DEV_STOP_ALL_ON_BOOT
    app_control_stop_all();
#endif

    app_antenna_start();
#if DEV_ENABLE_WIFI_ON_BOOT
    app_antenna_time_sync_request(30000);
#else
    app_antenna_set_wifi_enabled(false);
#endif

#if CONFIG_JOFTMODE_ENABLE_ML
    if (DEV_DISABLE_DEFAULT_RAW_ON_BOOT) {
        ESP_LOGI("app_main", "ML disabled on boot");
    } else {
        app_ml_init();
        // Force-link ML task implementation and start it (safe if already started).
        app_ml_task_start();
    }
#endif

    app_gui_start();

    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
