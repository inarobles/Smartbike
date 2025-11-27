#include "ui_main.h"
#include "lvgl.h"
#include <stdio.h>
#include "esp_log.h"


#include "ui_training.h"
#include "ui_settings.h"
#include "ble_client.h"
#include "wifi_client.h"

static lv_style_t style_btn;
static lv_style_t style_label_status;
static lv_timer_t * s_hr_timer = NULL;
static lv_obj_t * s_hr_label = NULL;
static lv_obj_t * s_pwr_label = NULL;
static lv_obj_t * s_wifi_label = NULL;

static void update_hr_timer_cb(lv_timer_t * timer)
{
    // Update Heart Rate
    if (s_hr_label && lv_obj_is_valid(s_hr_label)) {
        if (ble_client_is_connected()) {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "Cardio BLE: %d ppm", ble_client_get_heart_rate());
            lv_label_set_text(s_hr_label, buffer);
            lv_obj_set_style_text_color(s_hr_label, lv_color_hex(0x00FF00), 0); // Green
        } else {
            lv_label_set_text(s_hr_label, "Cardio BLE: Desconectado");
            lv_obj_set_style_text_color(s_hr_label, lv_color_hex(0x888888), 0); // Grey
        }
    }

    // Update Power
    if (s_pwr_label && lv_obj_is_valid(s_pwr_label)) {
        if (ble_client_is_power_connected()) {
            lv_label_set_text(s_pwr_label, "Potencia BLE: Conectado");
            lv_obj_set_style_text_color(s_pwr_label, lv_color_hex(0x00FF00), 0); // Green
        } else {
            lv_label_set_text(s_pwr_label, "Potencia BLE: Desconectado");
            lv_obj_set_style_text_color(s_pwr_label, lv_color_hex(0x888888), 0); // Grey
        }
    }

    // Update WiFi
    if (s_wifi_label && lv_obj_is_valid(s_wifi_label)) {
        if (is_wifi_connected()) {
            lv_label_set_text(s_wifi_label, "WIFI: Conectado");
            lv_obj_set_style_text_color(s_wifi_label, lv_color_hex(0x00FF00), 0); // Green
        } else {
            lv_label_set_text(s_wifi_label, "WIFI: Desconectado");
            lv_obj_set_style_text_color(s_wifi_label, lv_color_hex(0x888888), 0); // Grey
        }
    }
}

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        const char * txt = lv_label_get_text(lv_obj_get_child(btn, 0));
        if (s_hr_timer) {
            lv_timer_del(s_hr_timer);
            s_hr_timer = NULL;
            s_hr_label = NULL;
            s_pwr_label = NULL;
            s_wifi_label = NULL;
        }

        if (strcmp(txt, "Ajustes") == 0) {
            ESP_LOGI("UI", "Loading settings screen");
            ui_settings_screen_init();
        } else {
            ESP_LOGI("UI", "Loading training screen");
            ui_training_screen_init();
        }
    }
}

static void create_button(lv_obj_t * parent, const char * text, bool add_event)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_width(btn, lv_pct(90)); // Increased width (approx +50px)
    lv_obj_set_height(btn, 120); // Height for font 32
    lv_obj_add_style(btn, &style_btn, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    
    // Always add event callback for all buttons now
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    
    // Try to use a larger font if available, otherwise default
    #if LV_FONT_MONTSERRAT_32
    lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);
    #elif LV_FONT_MONTSERRAT_30
    lv_obj_set_style_text_font(label, &lv_font_montserrat_30, 0);
    #elif LV_FONT_MONTSERRAT_28
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    #elif LV_FONT_MONTSERRAT_30
    lv_obj_set_style_text_font(label, &lv_font_montserrat_30, 0);
    #elif LV_FONT_MONTSERRAT_28
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    #elif LV_FONT_MONTSERRAT_26
    lv_obj_set_style_text_font(label, &lv_font_montserrat_26, 0);
    #elif LV_FONT_MONTSERRAT_24
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    #elif LV_FONT_MONTSERRAT_20
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    #elif LV_FONT_MONTSERRAT_16
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    #endif
}

static void create_status_label(lv_obj_t * parent, const char * text)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(label, lv_pct(90));
    lv_obj_add_style(label, &style_label_status, 0);

    // Increase font size for status labels
    #if LV_FONT_MONTSERRAT_20
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    #elif LV_FONT_MONTSERRAT_16
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    #endif
}

void ui_init(void)
{
    // Initialize Styles
    lv_style_init(&style_btn);
    lv_style_set_bg_color(&style_btn, lv_color_hex(0x2D2D2D)); // Dark Grey
    lv_style_set_bg_grad_color(&style_btn, lv_color_hex(0x1A1A1A));
    lv_style_set_bg_grad_dir(&style_btn, LV_GRAD_DIR_VER);
    lv_style_set_border_color(&style_btn, lv_color_hex(0x404040));
    lv_style_set_border_width(&style_btn, 2);
    lv_style_set_radius(&style_btn, 12);
    lv_style_set_shadow_width(&style_btn, 0);
    lv_style_set_text_color(&style_btn, lv_color_hex(0xFFFFFF));

    lv_style_init(&style_label_status);
    lv_style_set_text_color(&style_label_status, lv_color_hex(0x888888)); // Grey text
    
    // Log selected font for debugging
    #if LV_FONT_MONTSERRAT_32
        ESP_LOGI("UI", "Using font: Montserrat 32");
    #elif LV_FONT_MONTSERRAT_30
        ESP_LOGI("UI", "Using font: Montserrat 30");
    #elif LV_FONT_MONTSERRAT_30
        ESP_LOGI("UI", "Using font: Montserrat 30");
    #elif LV_FONT_MONTSERRAT_28
        ESP_LOGI("UI", "Using font: Montserrat 28");
    #elif LV_FONT_MONTSERRAT_26
        ESP_LOGI("UI", "Using font: Montserrat 26");
    #elif LV_FONT_MONTSERRAT_24
        ESP_LOGI("UI", "Using font: Montserrat 24");
    #elif LV_FONT_MONTSERRAT_20
        ESP_LOGW("UI", "Large fonts not found. Using fallback: Montserrat 20");
    #else
        ESP_LOGW("UI", "No custom large fonts found. Using default system font.");
    #endif

    lv_obj_t * scr = lv_screen_active();
    lv_obj_clean(scr); // Clear the screen
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0); // Black background
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Create a container for the layout
    lv_obj_t * cont = lv_obj_create(scr);
    lv_obj_set_size(cont, lv_pct(100), lv_pct(100));
    lv_obj_center(cont);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(cont, 47, 0); // 37 + 10 px top padding
    lv_obj_set_style_pad_row(cont, 25, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);

    // Buttons
    create_button(cont, "Entrenamiento libre", true);
    create_button(cont, "Descarga entrenamiento", true);
    create_button(cont, "Ajustes", false);

    // Container for status labels (fills remaining space and centers content)
    lv_obj_t * status_cont = lv_obj_create(cont);
    lv_obj_set_width(status_cont, lv_pct(100));
    lv_obj_set_flex_grow(status_cont, 1); // Take remaining space
    lv_obj_set_style_bg_opa(status_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status_cont, 0, 0);
    lv_obj_set_flex_flow(status_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(status_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(status_cont, 0, 0);
    lv_obj_set_style_pad_row(status_cont, 10, 0); // Gap between status labels

    // Status Labels
    // Always create the label and store it
    s_hr_label = lv_label_create(status_cont);
    lv_obj_set_style_text_align(s_hr_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_hr_label, lv_pct(90));
    lv_obj_add_style(s_hr_label, &style_label_status, 0);
    
    // Increase font size for status labels
    #if LV_FONT_MONTSERRAT_20
    lv_obj_set_style_text_font(s_hr_label, &lv_font_montserrat_20, 0);
    #elif LV_FONT_MONTSERRAT_16
    lv_obj_set_style_text_font(s_hr_label, &lv_font_montserrat_16, 0);
    #endif

    // Initial update
    if (ble_client_is_connected()) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "Cardio BLE: %d ppm", ble_client_get_heart_rate());
        lv_label_set_text(s_hr_label, buffer);
        lv_obj_set_style_text_color(s_hr_label, lv_color_hex(0x00FF00), 0); // Green
    } else {
        lv_label_set_text(s_hr_label, "Cardio BLE: Desconectado");
        lv_obj_set_style_text_color(s_hr_label, lv_color_hex(0x888888), 0); // Grey
    }

    // Start timer for updates
    if (s_hr_timer) {
        lv_timer_del(s_hr_timer); // Safety check
    }
    s_hr_timer = lv_timer_create(update_hr_timer_cb, 1000, NULL);

    // Power Label
    s_pwr_label = lv_label_create(status_cont);
    lv_obj_set_style_text_align(s_pwr_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_pwr_label, lv_pct(90));
    lv_obj_add_style(s_pwr_label, &style_label_status, 0);
    
    #if LV_FONT_MONTSERRAT_20
    lv_obj_set_style_text_font(s_pwr_label, &lv_font_montserrat_20, 0);
    #elif LV_FONT_MONTSERRAT_16
    lv_obj_set_style_text_font(s_pwr_label, &lv_font_montserrat_16, 0);
    #endif

    if (ble_client_is_power_connected()) {
        lv_label_set_text(s_pwr_label, "Potencia BLE: Conectado");
        lv_obj_set_style_text_color(s_pwr_label, lv_color_hex(0x00FF00), 0); // Green
    } else {
        lv_label_set_text(s_pwr_label, "Potencia BLE: Desconectado");
        lv_obj_set_style_text_color(s_pwr_label, lv_color_hex(0x888888), 0); // Grey
    }

    // WiFi Label
    s_wifi_label = lv_label_create(status_cont);
    lv_label_set_text(s_wifi_label, "WIFI: Desconectado");
    lv_obj_set_style_text_align(s_wifi_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_wifi_label, lv_pct(90));
    lv_obj_add_style(s_wifi_label, &style_label_status, 0);

    #if LV_FONT_MONTSERRAT_20
    lv_obj_set_style_text_font(s_wifi_label, &lv_font_montserrat_20, 0);
    #elif LV_FONT_MONTSERRAT_16
    lv_obj_set_style_text_font(s_wifi_label, &lv_font_montserrat_16, 0);
    #endif

    if (is_wifi_connected()) {
        lv_label_set_text(s_wifi_label, "WIFI: Conectado");
        lv_obj_set_style_text_color(s_wifi_label, lv_color_hex(0x00FF00), 0); // Green
    }
}
