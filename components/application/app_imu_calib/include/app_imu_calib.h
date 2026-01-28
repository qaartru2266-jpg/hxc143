#ifndef APP_IMU_CALIB_H
#define APP_IMU_CALIB_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

void app_imu_calib_init(void);
void app_imu_calib_start_cmd(void);
void app_imu_calib_apply(int16_t *ax, int16_t *ay, int16_t *az,
                         int16_t *gx, int16_t *gy, int16_t *gz);
bool app_imu_calib_handle_line(const char *line);
esp_err_t app_imu_calib_run(int seconds);
esp_err_t app_imu_calib_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_IMU_CALIB_H */
