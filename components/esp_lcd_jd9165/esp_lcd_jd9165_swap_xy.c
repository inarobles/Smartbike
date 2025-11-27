
static esp_err_t panel_jd9165_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    jd9165_panel_t *jd9165 = (jd9165_panel_t *)panel->user_data;
    esp_lcd_panel_io_handle_t io = jd9165->io;
    uint8_t madctl_val = jd9165->madctl_val;

    ESP_RETURN_ON_FALSE(io, ESP_ERR_INVALID_STATE, TAG, "invalid panel IO");

    // Check if the panel supports swap_xy (usually bit 5 of MADCTL)
    // For now, we assume it's supported if we are asked to do it, or we just update the register.
    // However, based on the mirror implementation, this driver uses specific bits.
    // Standard MIPI DCS says bit 5 (0x20) is MV (Row/Column Exchange).
    
    if (swap_axes) {
        madctl_val |= (1 << 5); // Set MV bit
    } else {
        madctl_val &= ~(1 << 5); // Clear MV bit
    }

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t []) {
        madctl_val
    }, 1), TAG, "send command failed");
    jd9165->madctl_val = madctl_val;

    return ESP_OK;
}
