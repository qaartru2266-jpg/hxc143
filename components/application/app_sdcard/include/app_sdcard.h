#ifndef APP_SDCARD_H
#define APP_SDCARD_H

#include "sdkconfig.h"

#if CONFIG_JOFTMODE_ENABLE_ML
#include "ml_window.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void app_sdcard_start(void);
bool app_sdcard_is_ready(void);
#if CONFIG_JOFTMODE_ENABLE_ML
bool app_ml_get_latest(ml_result_t *out);
#endif

#ifdef __cplusplus
}
#endif

#endif /* APP_SDCARD_H */
