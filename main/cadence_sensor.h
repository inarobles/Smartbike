#ifndef CADENCE_SENSOR_H
#define CADENCE_SENSOR_H

#include <stdint.h>

/**
 * @brief Initialize the cadence sensor on GPIO 48.
 *        Configures GPIO, ISR, and internal timers.
 */
void cadence_sensor_init(void);

/**
 * @brief Get the current RPM (Revolutions Per Minute).
 *        Based on the latest pulse interval.
 */
uint16_t cadence_sensor_get_rpm(void);

/**
 * @brief Get the average RPM of the current session.
 *        Resets when session resets (or functionality to result is added).
 */
uint16_t cadence_sensor_get_avg_rpm(void);

/**
 * @brief Get the maximum RPM recorded in the current session.
 */
uint16_t cadence_sensor_get_max_rpm(void);

/**
 * @brief Get the timestamp (in microseconds) of the last detected pulse.
 *        Used for UI visual feedback.
 */
int64_t cadence_sensor_get_last_pulse_time(void);

/**
 * @brief Get total ISR firing count for debugging.
 */
uint32_t cadence_sensor_get_isr_count(void);

/**
 * @brief Reset stats (Average, Max). 
 *        Call this when starting a new training session.
 */
void cadence_sensor_reset_stats(void);

#endif // CADENCE_SENSOR_H
