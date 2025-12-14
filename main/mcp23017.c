#include "mcp23017.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "MCP23017";
static i2c_master_dev_handle_t mcp_handle = NULL;

static esp_err_t write_reg(uint8_t reg, uint8_t val) {
    uint8_t data[2] = {reg, val};
    return i2c_master_transmit(mcp_handle, data, 2, 100);
}

esp_err_t mcp23017_read_ports(uint8_t *port_a, uint8_t *port_b) {
    if (!mcp_handle) return ESP_FAIL;
    
    // Read GPIOA. Note: With auto-increment on (default), we can read GPIOA and then GPIOB sequentially.
    // However, let's just read explicit registers to be safe.
    // Actually, it's better to burst read GPIOA (0x12) and GPIOB (0x13).
    // By default IOCON.SEQOP is 0 (Sequential Operation enabled).
    
    uint8_t reg = MCP_GPIOA;
    uint8_t data[2];
    esp_err_t err = i2c_master_transmit_receive(mcp_handle, &reg, 1, data, 2, 100);
    if (err == ESP_OK) {
        if (port_a) *port_a = data[0];
        if (port_b) *port_b = data[1];
    }
    return err;
}

esp_err_t mcp23017_init(i2c_master_bus_handle_t bus_handle) {
    if (!bus_handle) return ESP_FAIL;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MCP_ADDR,
        .scl_speed_hz = 100000,
    };
    
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus_handle, &dev_cfg, &mcp_handle), TAG, "Add I2C device failed");
    
    // 1. Configure all pins as INPUTS (0xFF) in IODIR (0x00, 0x01)
    ESP_RETURN_ON_ERROR(write_reg(MCP_IODIRA, 0xFF), TAG, "IODIRA failed");
    ESP_RETURN_ON_ERROR(write_reg(MCP_IODIRB, 0xFF), TAG, "IODIRB failed");

    // 2. Enable Pull-Ups (0xFF) in GPPU (0x0C, 0x0D)
    ESP_RETURN_ON_ERROR(write_reg(MCP_GPPUA, 0xFF), TAG, "GPPUA failed");
    ESP_RETURN_ON_ERROR(write_reg(MCP_GPPUB, 0xFF), TAG, "GPPUB failed");

    // 3. Configure Input Polarity (optional, but let's keep default 0=Normal)
    // Buttons connect to GND, so Input = 0 when pressed.
    // If we invert polarity (IPOL=0xFF), Input = 1 when pressed.
    // Let's use IPOL=0xFF so that 1=Pressed. It makes logic easier.
    ESP_RETURN_ON_ERROR(write_reg(MCP_IPOLA, 0xFF), TAG, "IPOLA failed");
    ESP_RETURN_ON_ERROR(write_reg(MCP_IPOLB, 0xFF), TAG, "IPOLB failed");

    ESP_LOGI(TAG, "MCP23017 Initialized");
    return ESP_OK;
}
