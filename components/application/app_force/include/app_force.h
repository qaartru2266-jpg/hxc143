#ifndef APP_FORCE_H
#define APP_FORCE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APP_FORCE_DEFAULT_DURATION_SEC (30U * 60U)

void app_force_enter(int mode, uint32_t duration_sec, const char *reason);
void app_force_cancel(const char *reason);
void app_force_poll(void);

bool app_force_is_active(void);
int app_force_get_mode(void);
int app_force_get_current_mode(int pred_mode);
uint32_t app_force_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_FORCE_H */
