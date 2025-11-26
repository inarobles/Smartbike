#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the I2C bus
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t i2c_driver_init(void);

/**
 * @brief Get the I2C bus handle
 *
 * @return i2c_master_bus_handle_t Handle to the I2C bus, or NULL if not initialized
 */
i2c_master_bus_handle_t i2c_driver_get_handle(void);

#ifdef __cplusplus
}
#endif

#endif // I2C_DRIVER_H
