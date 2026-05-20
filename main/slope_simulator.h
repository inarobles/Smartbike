#ifndef SLOPE_SIMULATOR_H
#define SLOPE_SIMULATOR_H

#include "esp_err.h"

/**
 * @brief Initialize the slope simulator module
 */
esp_err_t slope_simulator_init(void);

/**
 * @brief Adjust current slope by an increment
 * @param increment Positive or negative value to add to current slope
 */
void slope_simulator_adjust_slope(float increment);

/**
 * @brief Reset slope to 0% (typically at session start)
 */
void slope_simulator_reset(void);

/**
 * @brief Get current slope percentage
 * @return Current slope in percent (e.g., 5.0 for 5%)
 */
float slope_simulator_get_current_slope(void);

/**
 * @brief Request recalculation of resistance based on current state
 * Call this when gears are changed or weight is updated.
 */
void slope_simulator_update_resistance(void);

#endif // SLOPE_SIMULATOR_H
