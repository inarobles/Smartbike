#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdbool.h>

/**
 * @brief Global training mode status.
 * When true: Motor is allowed, Touch is disabled.
 * When false: Motor is forced to Stop-High (1-1), Touch is enabled.
 */
void app_state_set_training_mode(bool active);
bool app_state_is_training_active(void);

#endif // APP_STATE_H
