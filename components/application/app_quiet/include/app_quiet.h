#ifndef APP_QUIET_H
#define APP_QUIET_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void app_quiet_start(void);
bool app_quiet_handle_line(const char *line);

#ifdef __cplusplus
}
#endif

#endif /* APP_QUIET_H */
