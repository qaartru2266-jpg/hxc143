#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "app_datalog.h"

#define ENABLE_MOCK_TEST 0
#define DEV_STOP_ALL_ON_BOOT 0
#define DEV_DISABLE_DEFAULT_RAW_ON_BOOT 1

#if ENABLE_MOCK_TEST
#include "app_logic.h"
#endif
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
#include "ml_window.h"
#endif

void app_main(void)
{
    app_state_init();
    app_vibration_init();
    app_time_start();
    app_imu_calib_init();
    app_quiet_start();

    app_axis6_start();
    app_gps_start();
    app_gui_start();

    app_power_start();
    app_bat_adc_start();

#if CONFIG_JOFTMODE_ENABLE_ML
    ml_window_init();
#endif

    app_datalog_start();
    printf("MODE: DATA_COLLECTION (MOCK disabled)\n");
#if DEV_DISABLE_DEFAULT_RAW_ON_BOOT
    app_datalog_set_default_raw_enabled(false);
    printf("DATALOG: default raw logging disabled\n");
#else
    printf("DATALOG: raw logging enabled\n");
#endif

#if ENABLE_MOCK_TEST
    app_logic_start();
#endif
#if DEV_STOP_ALL_ON_BOOT
    app_control_stop_all();
#endif

    app_antenna_start();
    app_antenna_set_wifi_enabled(true);

    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
