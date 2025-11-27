#include "ui_cardio.h"
#include "ui_settings.h"
#include "ble_client.h"
#include "lvgl.h"
#include <stdio.h>
#include "esp_log.h"

static lv_obj_t * list_devices;
static lv_style_t style_btn;

static void device_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        ble_addr_t * addr = (ble_addr_t *)lv_obj_get_user_data(btn);
        if (addr) {
            ESP_LOGI("UI_CARDIO", "Connecting to device...");
            ble_client_connect(*addr, BLE_UUID_HEART_RATE);
            // Optional: Show connecting status
        }
    }
}

// Callback from BLE client when a device is found
static void on_device_found(const char* name, ble_addr_t addr)
{
    // This might be called from another task, so use lv_async_call if needed, 
    // but for simplicity assuming LVGL lock is held or single task for now.
    // Ideally, use a queue or lv_async_call.
    
    // Check if device already in list (simple check by text, or just add all)
    // For now, just add a button
    
    lv_obj_t * btn = lv_list_add_button(list_devices, NULL, name);
    
    // Store address copy in user data (needs memory management, simplified here)
    // In a real app, manage a list of structs. 
    // For this demo, we'll just store the address in a static array or similar if needed.
    // BUT lv_obj_set_user_data stores a pointer. We need persistent storage for the addr.
    
    // Better approach: Just pass the address to the connect function if we can.
    // Let's allocate memory for the address
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
        lv_obj_clean(list_devices); // Clear list
        ble_client_start_scan(on_device_found, BLE_UUID_HEART_RATE);
    }
}

static void back_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        ui_settings_screen_init();
    }
}

void ui_cardio_screen_init(void)
{
    lv_obj_t * scr = lv_screen_active();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Header
    lv_obj_t * label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "Cardio BLE");
    lv_obj_set_style_text_color(label_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_30, 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 20);

    // Scan Button
    lv_obj_t * btn_scan = lv_button_create(scr);
    lv_obj_set_size(btn_scan, 150, 50);
    lv_obj_align(btn_scan, LV_ALIGN_TOP_RIGHT, -20, 20);
    lv_obj_add_event_cb(btn_scan, scan_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * label_scan = lv_label_create(btn_scan);
    lv_label_set_text(label_scan, "Escanear");
    lv_obj_center(label_scan);

    // Back Button
    lv_obj_t * btn_back = lv_button_create(scr);
    lv_obj_set_size(btn_back, 100, 50);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_add_event_cb(btn_back, back_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "Volver");
    lv_obj_center(label_back);

    // List for devices
    list_devices = lv_list_create(scr);
    lv_obj_set_size(list_devices, lv_pct(90), lv_pct(70));
    lv_obj_align(list_devices, LV_ALIGN_BOTTOM_MID, 0, -20);
}
