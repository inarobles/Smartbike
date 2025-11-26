#include "i2c_driver.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "i2c_driver";

#define I2C_SCL_PIN           (GPIO_NUM_8)
#define I2C_SDA_PIN           (GPIO_NUM_7)

static i2c_master_bus_handle_t i2c_bus_handle = NULL;

esp_err_t i2c_driver_init(void)
{
    if (i2c_bus_handle != NULL) {
        ESP_LOGW(TAG, "I2C bus already initialized");
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_bus_conf = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_SCL_PIN,
        .sda_io_num = I2C_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&i2c_bus_conf, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2C bus initialized successfully");
    return ESP_OK;
}

i2c_master_bus_handle_t i2c_driver_get_handle(void)
{
    return i2c_bus_handle;
}
