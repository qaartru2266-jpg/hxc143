#ifndef APP_CONTROL_H
#define APP_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void app_control_stop_all(void);
void app_control_resume_all(void);
void app_control_stop_imu(void);
void app_control_resume_imu(void);
void app_control_stop_gps(void);
void app_control_resume_gps(void);
void app_control_stop_datalog(void);
void app_control_resume_datalog(void);
bool app_control_is_stopped(void);
void app_control_set_quiet(bool enable);
bool app_control_is_quiet(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_CONTROL_H */
