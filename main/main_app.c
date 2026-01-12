#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "app_sdcard.h"
#include "app_axis6.h"
#include "app_gps.h"
#include "app_gui.h"
#include "app_vibration.h"
#include "app_power.h"
#include "app_state.h"
#include "app_antenna.h"
#include "sdkconfig.h"

#if CONFIG_JOFTMODE_ENABLE_ML
#include "ml_window.h"
#endif

void app_main(void)
{
    app_state_init();
    app_vibration_init();

    app_axis6_start();
    app_gps_start();
    app_gui_start();

    app_power_start();

#if CONFIG_JOFTMODE_ENABLE_ML
    ml_window_init();
#endif

    app_sdcard_start();

    app_antenna_start();

    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
