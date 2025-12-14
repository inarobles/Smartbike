#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_memory_utils.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp_board_extra.h"
#include "lv_demos.h"

#include "ui_main.h"
#include "ble_client.h"
#include "wifi_manager.h"
#include "wifi_client.h"

#include "bike_config.h"
#include "audio_manager.h"
#include "button_manager.h"
#include "cadence_sensor.h"
#include "ina3221.h"

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(ret);

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
            .sw_rotate = true,
        }
    };
    lv_display_t *disp = bsp_display_start_with_config(&cfg);
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
    
    // Initialize Audio (After BSP I2C is ready)
    // Initialize Bike Configuration (Load from NVS)
    bike_config_init();

    // Initialize Audio (After BSP I2C is ready)
    audio_manager_init();

    // Initialize MCP23017 Button Manager (Shares BSP I2C)
    // Note: button_manager_init -> mcp23017_init requires BSP I2C handle.
    // mcp23017 driver uses bsp_i2c_get_handle() internally.
    // bsp_display_start_with_config initializes I2C if not already (or checks it).
    // Let's ensure button_manager_init is called.
    button_manager_init();

    bsp_display_backlight_on();

    bsp_display_lock(portMAX_DELAY);
    ui_init(); 
    bsp_display_unlock();

    // Initialize INA3221 (Shares BSP I2C)
    // Needs BSP I2C handle which is available after display start (if display manages I2C)
    // or we get it via bsp_i2c_get_handle().
    ina3221_init(bsp_i2c_get_handle());

    // Initialize WiFi after UI is ready
    wifi_manager_init();
    wifi_client_init();

    // Initialize BLE Client
    ble_client_init();

    // Initialize Cadence Sensor
    cadence_sensor_init();
}
