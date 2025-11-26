#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

#ifdef __cplusplus
extern "C" {
#endif

// Display configuration structure
typedef struct {
    lvgl_port_cfg_t lvgl_port_cfg;
    uint32_t buffer_size;
    bool double_buffer;
    struct {
        bool buff_dma;
        bool buff_spiram;
        bool sw_rotate;
    } flags;
} bsp_display_cfg_t;

// Default buffer size and configuration
#define BSP_LCD_DRAW_BUFF_SIZE     (1024 * 600 / 10)
#define BSP_LCD_DRAW_BUFF_DOUBLE   1

esp_err_t bsp_display_init(esp_lcd_panel_handle_t *panel_handle, SemaphoreHandle_t *refresh_finish_sem);
esp_lcd_panel_handle_t bsp_display_get_panel_handle(void);

// New functions to replace BSP
lv_display_t *bsp_display_start_with_config(const bsp_display_cfg_t *cfg);
void bsp_display_backlight_on(void);
void bsp_display_rotate(lv_display_t *disp, lv_display_rotation_t rotation);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_DRIVER_H