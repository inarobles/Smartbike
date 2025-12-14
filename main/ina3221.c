#include "ina3221.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h" // For common defines if needed, or pure driver I2C

static const char *TAG = "INA3221";
static i2c_master_dev_handle_t ina3221_handle = NULL;

esp_err_t ina3221_init(i2c_master_bus_handle_t bus_handle) {
    if (ina3221_handle != NULL) {
        return ESP_OK; // Already initialized
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = INA3221_ADDR_DEFAULT,
        .scl_speed_hz = 100000, // 100kHz standard
    };

    esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &ina3221_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add device to I2C bus");
        return err;
    }

    ESP_LOGI(TAG, "INA3221 Initialized");
    return ESP_OK;
}

static esp_err_t read_register16(uint8_t reg, uint16_t *val) {
    if (ina3221_handle == NULL) return ESP_ERR_INVALID_STATE;
    
    uint8_t tx_buf[1] = {reg};
    uint8_t rx_buf[2];
    
    esp_err_t err = i2c_master_transmit_receive(ina3221_handle, tx_buf, 1, rx_buf, 2, -1);
    if (err == ESP_OK) {
        *val = (rx_buf[0] << 8) | rx_buf[1];
    }
    return err;
}

esp_err_t ina3221_read_bus_voltage(uint8_t channel, float *voltage_v) {
    if (channel < 1 || channel > 3) return ESP_ERR_INVALID_ARG;

    uint8_t reg = 0;
    switch(channel) {
        case 1: reg = INA3221_REG_BUS_VOLTAGE_1; break;
        case 2: reg = INA3221_REG_BUS_VOLTAGE_2; break;
        case 3: reg = INA3221_REG_BUS_VOLTAGE_3; break;
    }

    uint16_t raw_val;
    esp_err_t err = read_register16(reg, &raw_val);
    if (err != ESP_OK) {
        return err;
    }

    // Register is 16-bit, but bottom 3 bits are reserved/flags.
    // Value is shifted by 3. LSB = 8mV.
    // Actually, datasheet says: "Bits D2-D0 are reserved. D15 is Sign bit."
    // It's a 13-bit value (plus sign), aligned to MSB?
    // "The value of the Bus Voltage register is stored in the 13 most significant bits of the register."
    // "One LSB = 8 mV"
    // So distinct value is Raw >> 3. Then multiply by 0.008V.

    int16_t signed_val = (int16_t)raw_val;
    signed_val = signed_val >> 3; 

    // Calculate voltage
    *voltage_v = signed_val * 0.008f; 

    return ESP_OK;
}
