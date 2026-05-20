#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <stdbool.h>

void button_manager_init(void);

// Calibration API
void button_manager_start_calibration(void);
bool button_manager_is_calibrating(void);
bool button_manager_get_calibration_result(float *min_v, float *max_v);

// Closed-loop control
void button_manager_set_target_voltage(float target_v);
bool button_manager_is_at_target(void);
void button_manager_stop(void);

// Motor API (Optional, if needed by other modules)
// void motor_start_forward();
// void motor_start_backward();
// void motor_stop();

#endif
