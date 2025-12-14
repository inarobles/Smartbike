#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <stdbool.h>

void button_manager_init(void);

// Calibration API
void button_manager_start_calibration(void);
bool button_manager_is_calibrating(void);
bool button_manager_get_calibration_result(float *min_v, float *max_v);

// Motor API (Optional, if needed by other modules)
// void motor_start_forward();
// void motor_start_backward();
// void motor_stop();

#endif
