#include "ui_power.h"
#include "ui_settings.h"
#include "ble_client.h"
#include "audio_manager.h"
#include "lvgl.h"
#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"

static lv_obj_t * list_devices;
static int64_t s_enter_time = 0;

static void device_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        audio_manager_play_event(AUDIO_EVENT_BUTTON);
        ble_addr_t * addr = (ble_addr_t *)lv_obj_get_user_data(btn);
        if (addr) {
            ESP_LOGI("UI_POWER", "Connecting to device...");
            ble_client_connect(*addr, BLE_UUID_CYCLING_POWER);
        }
    }
}

static void on_device_found(const char* name, ble_addr_t addr)
{
    lv_obj_t * btn = lv_list_add_button(list_devices, NULL, name);
    
    ble_addr_t * addr_copy = malloc(sizeof(ble_addr_t));
    if (addr_copy) {
        memcpy(addr_copy, &addr, sizeof(ble_addr_t));
        lv_obj_set_user_data(btn, addr_copy);
        lv_obj_add_event_cb(btn, device_btn_event_cb, LV_EVENT_CLICKED, NULL);
    }
}

static void scan_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        audio_manager_play_event(AUDIO_EVENT_BUTTON);
        lv_obj_clean(list_devices); 
        ble_client_start_scan(on_device_found, BLE_UUID_CYCLING_POWER);
    }
}

static void back_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    ESP_LOGI("UI_POWER", "Back button event code: %d", (int)code);
    
    if(code == LV_EVENT_CLICKED || code == LV_EVENT_RELEASED) {
        // Debounce: Ignore clicks if we just entered the screen (e.g. < 500ms)
        if ((esp_timer_get_time() / 1000 - s_enter_time) < 500) {
            ESP_LOGW("UI_POWER", "Back click ignored (debounce)");
            return;
        }

        ESP_LOGI("UI_POWER", "Going back to settings");
        audio_manager_play_event(AUDIO_EVENT_BUTTON);
        ui_settings_screen_init();
    }
}

void ui_power_screen_init(void)
{
    ESP_LOGI("UI_POWER", "Initializing Power Screen");
    s_enter_time = esp_timer_get_time() / 1000;

    lv_obj_t * scr = lv_screen_active();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Main container
    lv_obj_t * cont = lv_obj_create(scr);
    lv_obj_set_size(cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(cont, 20, 0);
    lv_obj_set_style_pad_row(cont, 20, 0);

    // Title
    lv_obj_t * label_title = lv_label_create(cont);
    lv_label_set_text(label_title, "Potencia BLE");
    lv_obj_set_style_text_color(label_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_30, 0);

    // Scan Button
    lv_obj_t * btn_scan = lv_button_create(cont);
    lv_obj_set_size(btn_scan, 200, 60);
    lv_obj_add_event_cb(btn_scan, scan_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * label_scan = lv_label_create(btn_scan);
    lv_label_set_text(label_scan, "Escanear");
    lv_obj_set_style_text_font(label_scan, &lv_font_montserrat_24, 0);
    lv_obj_center(label_scan);

    // List for devices
    list_devices = lv_list_create(cont);
    lv_obj_set_width(list_devices, lv_pct(90));
    lv_obj_set_flex_grow(list_devices, 1); // Take available space
    lv_obj_set_style_bg_color(list_devices, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_border_color(list_devices, lv_color_hex(0x404040), 0);

    // Back Button (Created at screen level to ensure it is always on top)
    lv_obj_t * btn_back = lv_button_create(scr);
    lv_obj_set_size(btn_back, 160, 70);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    // Explicit style for visibility
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_border_color(btn_back, lv_color_hex(0x888888), 0);
    lv_obj_set_style_border_width(btn_back, 2, 0);
    lv_obj_set_style_radius(btn_back, 12, 0);

    lv_obj_add_event_cb(btn_back, back_btn_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t * label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "Volver");
    lv_obj_set_style_text_font(label_back, &lv_font_montserrat_28, 0);
    lv_obj_center(label_back);
}

