#include "ui_settings.h"
#include "ui_main.h"
#include "ui_cardio.h" // Include cardio screen header
#include "ui_power.h" // Include power screen header
#include "ui_wifi.h" // Include wifi screen header
#include "lvgl.h"
#include <stdio.h>
#include "esp_log.h"

static lv_style_t style_btn;

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        const char * txt = lv_label_get_text(lv_obj_get_child(btn, 0));
        ESP_LOGI("UI_SETTINGS", "Button clicked: %s", txt);
        
        if (strcmp(txt, "Cardio BLE") == 0) {
            ui_cardio_screen_init();
        } else if (strcmp(txt, "Potencia BLE") == 0) {
            ui_power_screen_init();
        } else if (strcmp(txt, "WIFI") == 0) {
            ui_open_wifi_list();
        } else if (strcmp(txt, "Volver") == 0) {
            ui_init();
        }
        // Future: Handle other buttons
    }
}

static void create_button(lv_obj_t * parent, const char * text)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_width(btn, lv_pct(90));
    lv_obj_set_height(btn, 80); // Slightly smaller height to fit 5 buttons
    lv_obj_add_style(btn, &style_btn, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    
    // Font selection matching ui_main.c
    #if LV_FONT_MONTSERRAT_32
    lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);
    #elif LV_FONT_MONTSERRAT_30
    lv_obj_set_style_text_font(label, &lv_font_montserrat_30, 0);
    #elif LV_FONT_MONTSERRAT_28
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    #else
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    #endif
}

void ui_settings_screen_init(void)
{
    // Initialize Styles (Same as ui_main.c)
    static bool style_inited = false;
    if (!style_inited) {
        lv_style_init(&style_btn);
        lv_style_set_bg_color(&style_btn, lv_color_hex(0x2D2D2D)); // Dark Grey
        lv_style_set_bg_grad_color(&style_btn, lv_color_hex(0x1A1A1A));
        lv_style_set_bg_grad_dir(&style_btn, LV_GRAD_DIR_VER);
        lv_style_set_border_color(&style_btn, lv_color_hex(0x404040));
        lv_style_set_border_width(&style_btn, 2);
        lv_style_set_radius(&style_btn, 12);
        lv_style_set_shadow_width(&style_btn, 0);
        lv_style_set_text_color(&style_btn, lv_color_hex(0xFFFFFF));
        style_inited = true;
    }

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
    lv_obj_set_style_pad_top(cont, 20, 0);
    lv_obj_set_style_pad_row(cont, 15, 0); // Reduced gap to fit 5 buttons
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);

    // Buttons
    create_button(cont, "Cardio BLE");
    create_button(cont, "Potencia BLE");
    create_button(cont, "WIFI");
    create_button(cont, "Calibrar potencia");
    create_button(cont, "Calibrar freno");
    create_button(cont, "Volver");
}
