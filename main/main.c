/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "waveshare_rgb_lcd_port.h"

void app_main()
{
    waveshare_esp32_s3_rgb_lcd_init(); // Initialize the Waveshare ESP32-S3 RGB LCD
    // wavesahre_rgb_lcd_bl_on();  //Turn on the screen backlight
    // wavesahre_rgb_lcd_bl_off(); //Turn off the screen backlight

    ESP_LOGI(TAG, "Display Smartbike Main Menu");

    // Rotate display 90 degrees to portrait mode (without anti-tearing)
    if (lvgl_port_lock(-1)) {
        lv_disp_t *disp = lv_disp_get_default();
        lv_disp_set_rotation(disp, LV_DISP_ROT_90);
        lvgl_port_unlock();
    }

    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(-1)) {
        // lv_demo_stress();
        // lv_demo_benchmark();
        // lv_demo_music();
        // lv_demo_widgets();
        // example_lvgl_demo_ui();
        smartbike_main_menu(); // Custom Smartbike menu
        // Release the mutex
        lvgl_port_unlock();
    }
}
