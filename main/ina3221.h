#ifndef INA3221_H
#define INA3221_H

#include "driver/i2c_master.h"
#include "esp_err.h"

#define INA3221_ADDR_DEFAULT 0x40

// Registers
#define INA3221_REG_CONFIG          0x00
#define INA3221_REG_SHUNT_VOLTAGE_1 0x01
#define INA3221_REG_BUS_VOLTAGE_1   0x02
#define INA3221_REG_SHUNT_VOLTAGE_2 0x03
#define INA3221_REG_BUS_VOLTAGE_2   0x04
#define INA3221_REG_SHUNT_VOLTAGE_3 0x05
#define INA3221_REG_BUS_VOLTAGE_3   0x06
#define INA3221_REG_MANUFACTURER_ID 0xFE
#define INA3221_REG_DIE_ID          0xFF

esp_err_t ina3221_init(i2c_master_bus_handle_t bus_handle);
esp_err_t ina3221_read_bus_voltage(uint8_t channel, float *voltage_v);

#endif
