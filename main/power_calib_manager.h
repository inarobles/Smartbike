#ifndef POWER_CALIB_MANAGER_H
#define POWER_CALIB_MANAGER_H

#define CALIB_STEPS 9
#define CALIB_STEP_DURATION_MS 5000 // 5 seconds per step sampling

#include <stdbool.h>
#include <stdint.h>

// Calibration Step Info
typedef struct {
    uint8_t step_index;     // 1 to 5
    uint8_t total_steps;    // 5
    float target_voltage;   // Voltage being calibrated
    int current_rpm;        // Current RPM from sensor
    int current_watts;      // Current Watts from BLE
    bool is_stable;         // Is the system stable for sampling?
    float progress_percent; // 0.0 to 1.0 (overall progress)
} power_calib_status_t;

/**
 * @brief Initialize the power calibration manager
 */
void power_calib_init(void);

/**
 * @brief Start the power calibration process
 */
void power_calib_start(void);

/**
 * @brief Get the current status of the calibration
 * @return true if calibration is active, false otherwise
 */
bool power_calib_get_status(power_calib_status_t *status);

/**
 * @brief Get the estimated power based on current sensor readings (Voltage + RPM)
 * Uses the calibrated LUT.
 */
int16_t power_calib_get_estimate(void);

/**
 * @brief Check if calibration is finished (Success)
 */
bool power_calib_is_finished(void);

/**
 * @brief Check if calibration has failed
 */
bool power_calib_has_failed(void);

/**
 * @brief Stop/Cancel calibration
 */
void power_calib_stop(void);

#endif // POWER_CALIB_MANAGER_H
