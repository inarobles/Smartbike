#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui_main.h"
#include "ui_settings.h"
#include "ui_wifi.h"
#include "wifi_client.h"
#include "wifi_manager.h"
#include "esp_log.h"
#include <string.h>
#include "lvgl.h"
#include "bsp/esp-bsp.h"

static const char *TAG = "UI_WIFI";

// --- Custom Event ---
static uint32_t LV_EVENT_WIFI_SCAN_DONE;

// --- Global variables for WiFi screens ---
static lv_obj_t *scr_wifi_password;
static lv_obj_t *wifi_password_textarea;
static lv_obj_t *wifi_password_ok_btn;
static lv_obj_t *scr_loading = NULL;

static char ssid_to_connect[WIFI_MANAGER_MAX_SSID_LEN];
static char ssid_to_edit[WIFI_MANAGER_MAX_SSID_LEN];

static wifi_network_info_t g_scanned_networks[WIFI_MANAGER_MAX_NETWORKS];
static uint16_t g_num_scanned_networks = 0;

// --- Forward declarations ---
void ui_open_wifi_list(void);
static void build_wifi_list(void);
static void loading_screen_event_cb(lv_event_t *e);
static void wifi_scan_task(void *pvParameters);
static void saved_network_delete_cb(lv_event_t *e);
static void saved_network_edit_cb(lv_event_t *e);
static void wifi_password_keyboard_edit_ok_cb(lv_event_t *e);
static void wifi_password_keyboard_connect_cb(lv_event_t *e);
static void wifi_network_connect_cb(lv_event_t *e);
static void back_to_settings_from_wifi_cb(lv_event_t *e);
static void free_user_data_cb(lv_event_t *e);

// --- Stub for audio ---
static void audio_play_beep(void) {
    // No audio yet
}

// --- Event Handlers ---

static void back_to_settings_from_wifi_cb(lv_event_t *e) {
    audio_play_beep();
    ui_settings_screen_init();
}

static void free_user_data_cb(lv_event_t *e) {
    void *data = lv_event_get_user_data(e);
    if (data) {
        free(data);
    }
}

static void saved_network_delete_cb(lv_event_t *e) {
    audio_play_beep();
    const char* ssid = (const char*)lv_event_get_user_data(e);
    ESP_LOGI(TAG, "Deleting WiFi credentials for %s", ssid);
    wifi_manager_delete_credentials(ssid);
    ui_open_wifi_list(); // Refresh the list
}

static void saved_network_edit_cb(lv_event_t *e) {
    audio_play_beep();
    const char* ssid = (const char*)lv_event_get_user_data(e);
    ESP_LOGI(TAG, "Editing WiFi credentials for %s", ssid);

    strncpy(ssid_to_edit, ssid, sizeof(ssid_to_edit) - 1);
    ssid_to_edit[sizeof(ssid_to_edit) - 1] = '\0';

    // Re-create password screen if needed or just load it
    // For simplicity, we assume scr_wifi_password is created when needed or globally
    // But in build_wifi_list we create screens.
    // Let's create the password screen here if it doesn't exist or just re-create it.
    // Actually, Consola created it in create_wifi_screens called at init.
    // We don't have a global init for wifi screens, so we create it on demand.
    
    scr_wifi_password = lv_obj_create(NULL);
    lv_obj_set_size(scr_wifi_password, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(scr_wifi_password, lv_color_black(), 0);

    wifi_password_textarea = lv_textarea_create(scr_wifi_password);
    lv_obj_set_size(wifi_password_textarea, LV_PCT(90), 60);
    lv_obj_align(wifi_password_textarea, LV_ALIGN_TOP_MID, 0, 20);
    lv_textarea_set_password_mode(wifi_password_textarea, true);
    lv_textarea_set_one_line(wifi_password_textarea, true);
    // lv_obj_set_style_text_font(wifi_password_textarea, &lv_font_montserrat_20, 0); // Use default font if 20 not avail

    lv_obj_t *kb = lv_keyboard_create(scr_wifi_password);
    lv_obj_set_size(kb, LV_PCT(100), LV_PCT(50));
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, wifi_password_textarea);

    wifi_password_ok_btn = lv_button_create(scr_wifi_password);
    lv_obj_set_size(wifi_password_ok_btn, 150, 50);
    lv_obj_align(wifi_password_ok_btn, LV_ALIGN_TOP_RIGHT, -10, 90);
    lv_obj_t *label_ok = lv_label_create(wifi_password_ok_btn);
    lv_label_set_text(label_ok, "GUARDAR");
    lv_obj_center(label_ok);

    lv_textarea_set_text(wifi_password_textarea, "");
    lv_textarea_set_placeholder_text(wifi_password_textarea, ssid);
    
    lv_obj_add_event_cb(wifi_password_ok_btn, wifi_password_keyboard_edit_ok_cb, LV_EVENT_CLICKED, NULL);

    lv_scr_load(scr_wifi_password);
}

static void wifi_password_keyboard_edit_ok_cb(lv_event_t *e) {
    audio_play_beep();
    const char *password = lv_textarea_get_text(wifi_password_textarea);

    if (password && strlen(password) > 0) {
        ESP_LOGI(TAG, "Saving new password for: %s", ssid_to_edit);
        wifi_manager_save_credentials(ssid_to_edit, password);
    } else {
        ESP_LOGW(TAG, "Password is empty, aborting edit.");
    }
    ui_open_wifi_list();
}

static void wifi_password_keyboard_connect_cb(lv_event_t *e) {
    audio_play_beep();
    const char *password = lv_textarea_get_text(wifi_password_textarea);

    if (password && strlen(password) > 0) {
        ESP_LOGI(TAG, "Attempting to connect to %s", ssid_to_connect);
        wifi_manager_save_credentials(ssid_to_connect, password);
        
        // Show loading screen
        scr_loading = lv_obj_create(NULL);
        lv_obj_set_size(scr_loading, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(scr_loading, lv_color_black(), 0);
        lv_obj_t *spinner = lv_spinner_create(scr_loading);
        lv_spinner_set_anim_params(spinner, 1000, 60);
        lv_obj_set_size(spinner, 100, 100);
        lv_obj_center(spinner);
        lv_obj_t *label = lv_label_create(scr_loading);
        lv_label_set_text(label, "Conectando...");
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_align_to(label, spinner, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
        lv_scr_load(scr_loading);

        wifi_client_connect(ssid_to_connect, password);
    } else {
        ESP_LOGW(TAG, "Password is empty, aborting connection.");
        ui_open_wifi_list();
    }
}

static void wifi_network_connect_cb(lv_event_t *e) {
    audio_play_beep();
    wifi_network_info_t *info = (wifi_network_info_t *)lv_event_get_user_data(e);

    if (info->auth_mode == WIFI_AUTH_OPEN) {
        ESP_LOGI(TAG, "Connecting to open network: %s", info->ssid);
        
        // Show loading screen
        scr_loading = lv_obj_create(NULL);
        lv_obj_set_size(scr_loading, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(scr_loading, lv_color_black(), 0);
        lv_obj_t *spinner = lv_spinner_create(scr_loading);
        lv_spinner_set_anim_params(spinner, 1000, 60);
        lv_obj_set_size(spinner, 100, 100);
        lv_obj_center(spinner);
        lv_obj_t *label = lv_label_create(scr_loading);
        lv_label_set_text(label, "Conectando...");
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_align_to(label, spinner, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
        lv_scr_load(scr_loading);

        wifi_client_connect(info->ssid, NULL);
        return;
    }

    char password[WIFI_MANAGER_MAX_PASSWORD_LEN];
    if (wifi_manager_load_credentials(info->ssid, password) == ESP_OK) {
        ESP_LOGI(TAG, "Connecting to saved network: %s", info->ssid);
        
        // Show loading screen
        scr_loading = lv_obj_create(NULL);
        lv_obj_set_size(scr_loading, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(scr_loading, lv_color_black(), 0);
        lv_obj_t *spinner = lv_spinner_create(scr_loading);
        lv_spinner_set_anim_params(spinner, 1000, 60);
        lv_obj_set_size(spinner, 100, 100);
        lv_obj_center(spinner);
        lv_obj_t *label = lv_label_create(scr_loading);
        lv_label_set_text(label, "Conectando...");
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_align_to(label, spinner, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
        lv_scr_load(scr_loading);

        wifi_client_connect(info->ssid, password);
    } else {
        ESP_LOGI(TAG, "Password required for network: %s", info->ssid);
        strncpy(ssid_to_connect, info->ssid, sizeof(ssid_to_connect) - 1);
        ssid_to_connect[sizeof(ssid_to_connect) - 1] = '\0';

        // Create password screen
        scr_wifi_password = lv_obj_create(NULL);
        lv_obj_set_size(scr_wifi_password, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(scr_wifi_password, lv_color_black(), 0);

        wifi_password_textarea = lv_textarea_create(scr_wifi_password);
        lv_obj_set_size(wifi_password_textarea, LV_PCT(90), 60);
        lv_obj_align(wifi_password_textarea, LV_ALIGN_TOP_MID, 0, 20);
        lv_textarea_set_password_mode(wifi_password_textarea, true);
        lv_textarea_set_one_line(wifi_password_textarea, true);

        lv_obj_t *kb = lv_keyboard_create(scr_wifi_password);
        lv_obj_set_size(kb, LV_PCT(100), LV_PCT(50));
        lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_keyboard_set_textarea(kb, wifi_password_textarea);

        wifi_password_ok_btn = lv_button_create(scr_wifi_password);
        lv_obj_set_size(wifi_password_ok_btn, 150, 50);
        lv_obj_align(wifi_password_ok_btn, LV_ALIGN_TOP_RIGHT, -10, 90);
        lv_obj_t *label_ok = lv_label_create(wifi_password_ok_btn);
        lv_label_set_text(label_ok, "CONECTAR");
        lv_obj_center(label_ok);

        lv_textarea_set_text(wifi_password_textarea, "");
        lv_textarea_set_placeholder_text(wifi_password_textarea, info->ssid);
        
        lv_obj_add_event_cb(wifi_password_ok_btn, wifi_password_keyboard_connect_cb, LV_EVENT_CLICKED, NULL);

        lv_scr_load(scr_wifi_password);
    }
}

static void create_section_title(lv_obj_t *parent, const char *title) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, title);
    // lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);
    lv_obj_set_style_pad_top(label, 20, 0);
    lv_obj_set_style_pad_bottom(label, 10, 0);
}

static void build_wifi_list(void) {
    lv_obj_t *scr_wifi_list = lv_obj_create(NULL);
    lv_obj_set_size(scr_wifi_list, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(scr_wifi_list, lv_color_black(), 0);

    lv_obj_t *title = lv_label_create(scr_wifi_list);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    // lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_label_set_text(title, "Redes WiFi");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *main_container = lv_obj_create(scr_wifi_list);
    lv_obj_set_size(main_container, LV_PCT(95), LV_PCT(80));
    lv_obj_align(main_container, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(main_container, 10, 0);
    lv_obj_set_style_pad_row(main_container, 10, 0);
    lv_obj_set_scrollbar_mode(main_container, LV_SCROLLBAR_MODE_AUTO);

    // --- Data Gathering ---
    char current_ssid[WIFI_MANAGER_MAX_SSID_LEN] = {0};
    bool is_connected = is_wifi_connected();
    wifi_ap_record_t ap_info;
    if (is_connected) {
        esp_wifi_sta_get_ap_info(&ap_info);
        strlcpy(current_ssid, (char *)ap_info.ssid, sizeof(current_ssid));
    }

    wifi_network_info_t saved_networks[WIFI_MANAGER_MAX_NETWORKS];
    uint16_t num_saved_networks = 0;
    wifi_manager_get_saved_ssids(saved_networks, WIFI_MANAGER_MAX_NETWORKS, &num_saved_networks);

    // --- Filter Scanned Networks ---
    wifi_network_info_t filtered_networks[WIFI_MANAGER_MAX_NETWORKS];
    uint16_t num_filtered_networks = 0;
    for (int i = 0; i < g_num_scanned_networks; i++) {
        bool found = false;
        for (int j = 0; j < num_filtered_networks; j++) {
            if (strcmp(g_scanned_networks[i].ssid, filtered_networks[j].ssid) == 0) {
                found = true;
                if (g_scanned_networks[i].rssi > filtered_networks[j].rssi) {
                    filtered_networks[j] = g_scanned_networks[i];
                }
                break;
            }
        }
        if (!found) {
            filtered_networks[num_filtered_networks++] = g_scanned_networks[i];
        }
    }

    // --- UI Building ---

    // 1. Connected Network
    if (is_connected) {
        lv_obj_t *item = lv_obj_create(main_container);
        lv_obj_set_size(item, LV_PCT(100), 80);
        lv_obj_set_style_bg_color(item, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_obj_set_layout(item, LV_LAYOUT_FLEX);
        lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *label = lv_label_create(item);
        lv_label_set_text_fmt(label, "Conectado a: %s (%d)", current_ssid, ap_info.rssi);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
    }

    // 2. Available Networks
    create_section_title(main_container, "Redes Disponibles:");
    bool has_available = false;
    for (int i = 0; i < num_filtered_networks; i++) {
        if (strcmp(current_ssid, filtered_networks[i].ssid) != 0) {
            has_available = true;

            lv_obj_t *item = lv_obj_create(main_container);
            lv_obj_set_size(item, LV_PCT(100), 80);
            lv_obj_set_layout(item, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(item, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

            lv_obj_t *ssid_label = lv_label_create(item);
            lv_label_set_text_fmt(ssid_label, "%s (%d)", filtered_networks[i].ssid, filtered_networks[i].rssi);
            lv_obj_set_flex_grow(ssid_label, 1);

            lv_obj_t *btn_connect = lv_button_create(item);
            
            // Allocate memory for the network info to persist after this function returns
            wifi_network_info_t *btn_info = malloc(sizeof(wifi_network_info_t));
            if (btn_info) {
                *btn_info = filtered_networks[i];
                lv_obj_add_event_cb(btn_connect, wifi_network_connect_cb, LV_EVENT_CLICKED, btn_info);
                // Free memory when button is deleted
                lv_obj_add_event_cb(btn_connect, free_user_data_cb, LV_EVENT_DELETE, NULL);
            }
            
            lv_obj_t *label_connect = lv_label_create(btn_connect);
            lv_label_set_text(label_connect, "Conectar");

            bool is_saved = false;
            for (int j = 0; j < num_saved_networks; j++) {
                if (strcmp(filtered_networks[i].ssid, saved_networks[j].ssid) == 0) {
                    is_saved = true;
                    break;
                }
            }

            if (is_saved) {
                lv_obj_t *btn_edit = lv_button_create(item);
                // For edit/delete, we just need the SSID string. 
                // Since we need it to persist, we strdup it.
                char *ssid_copy_edit = strdup(filtered_networks[i].ssid);
                if (ssid_copy_edit) {
                    lv_obj_add_event_cb(btn_edit, saved_network_edit_cb, LV_EVENT_CLICKED, ssid_copy_edit);
                    lv_obj_add_event_cb(btn_edit, free_user_data_cb, LV_EVENT_DELETE, NULL);
                }

                lv_obj_t *label_edit = lv_label_create(btn_edit);
                lv_label_set_text(label_edit, "Editar");

                lv_obj_t *btn_del = lv_button_create(item);
                lv_obj_set_style_bg_color(btn_del, lv_palette_main(LV_PALETTE_RED), 0);
                char *ssid_copy_del = strdup(filtered_networks[i].ssid);
                if (ssid_copy_del) {
                    lv_obj_add_event_cb(btn_del, saved_network_delete_cb, LV_EVENT_CLICKED, ssid_copy_del);
                    lv_obj_add_event_cb(btn_del, free_user_data_cb, LV_EVENT_DELETE, NULL);
                }
                lv_obj_t *label_del = lv_label_create(btn_del);
                lv_label_set_text(label_del, "Borrar");
            }
        }
    }
    if (!has_available) {
        lv_obj_t *label = lv_label_create(main_container);
        lv_label_set_text(label, "No se encontraron redes.");
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
    }

    // 3. Saved Networks
    create_section_title(main_container, "Redes Guardadas:");
    bool has_saved = false;
    for (int i = 0; i < num_saved_networks; i++) {
        bool is_scanned = false;
        for (int j = 0; j < num_filtered_networks; j++) {
            if (strcmp(saved_networks[i].ssid, filtered_networks[j].ssid) == 0) {
                is_scanned = true;
                break;
            }
        }
        if (strcmp(current_ssid, saved_networks[i].ssid) != 0 && !is_scanned) {
            has_saved = true;
            lv_obj_t *item = lv_obj_create(main_container);
            lv_obj_set_size(item, LV_PCT(100), 80);
            lv_obj_set_layout(item, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(item, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

            lv_obj_t *ssid_label = lv_label_create(item);
            lv_label_set_text(ssid_label, saved_networks[i].ssid);
            lv_obj_set_flex_grow(ssid_label, 1);

            lv_obj_t *btn_edit = lv_button_create(item);
            char *ssid_copy_edit = strdup(saved_networks[i].ssid);
            if (ssid_copy_edit) {
                lv_obj_add_event_cb(btn_edit, saved_network_edit_cb, LV_EVENT_CLICKED, ssid_copy_edit);
                lv_obj_add_event_cb(btn_edit, free_user_data_cb, LV_EVENT_DELETE, NULL);
            }

            lv_obj_t *label_edit = lv_label_create(btn_edit);
            lv_label_set_text(label_edit, "Editar");

            lv_obj_t *btn_del = lv_button_create(item);
            lv_obj_set_style_bg_color(btn_del, lv_palette_main(LV_PALETTE_RED), 0);
            char *ssid_copy_del = strdup(saved_networks[i].ssid);
            if (ssid_copy_del) {
                lv_obj_add_event_cb(btn_del, saved_network_delete_cb, LV_EVENT_CLICKED, ssid_copy_del);
                lv_obj_add_event_cb(btn_del, free_user_data_cb, LV_EVENT_DELETE, NULL);
            }
            lv_obj_t *label_del = lv_label_create(btn_del);
            lv_label_set_text(label_del, "Borrar");
        }
    }
    if (!has_saved) {
        lv_obj_t *label = lv_label_create(main_container);
        lv_label_set_text(label, "No hay otras redes guardadas.");
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
    }

    lv_obj_t *btn_back = lv_button_create(scr_wifi_list);
    lv_obj_set_size(btn_back, 150, 50);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btn_back, back_to_settings_from_wifi_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "Volver");
    lv_obj_center(label_back);

    lv_scr_load(scr_wifi_list);
}

static void loading_screen_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_WIFI_SCAN_DONE) {
        build_wifi_list();
    }
}

static void wifi_scan_task(void *pvParameters) {
    lv_obj_t *scr_loading = (lv_obj_t *)pvParameters;

    wifi_manager_scan_networks(g_scanned_networks, WIFI_MANAGER_MAX_NETWORKS, &g_num_scanned_networks);

    bsp_display_lock(0);
    lv_obj_send_event(scr_loading, LV_EVENT_WIFI_SCAN_DONE, NULL);
    bsp_display_unlock();

    vTaskDelete(NULL);
}

void ui_open_wifi_list(void) {
    bsp_display_lock(0);

    // Register event if not already
    if (LV_EVENT_WIFI_SCAN_DONE == 0) {
        LV_EVENT_WIFI_SCAN_DONE = lv_event_register_id();
    }

    // Create a loading screen
    scr_loading = lv_obj_create(NULL);
    lv_obj_set_size(scr_loading, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(scr_loading, lv_color_black(), 0);
    lv_obj_add_event_cb(scr_loading, loading_screen_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *spinner = lv_spinner_create(scr_loading);
    lv_spinner_set_anim_params(spinner, 1000, 60);
    lv_obj_set_size(spinner, 100, 100);
    lv_obj_center(spinner);

    lv_obj_t *label = lv_label_create(scr_loading);
    lv_label_set_text(label, "Escaneando redes wifi, espere un momento.");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    // lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_obj_align_to(label, spinner, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

    lv_scr_load(scr_loading);

    // Create a task to perform the scan
    xTaskCreate(wifi_scan_task, "wifi_scan_task", 4096, scr_loading, 5, NULL);

    bsp_display_unlock();
}

void ui_loading_complete(void) {
    ESP_LOGI(TAG, "Loading complete, going to settings or wifi list");
    // If we were connecting, we might want to go back to settings or stay in wifi list
    // For now, let's go to settings as a safe default or reload wifi list
    // But typically this is called after connection attempt.
    // Let's reload wifi list to show connected status
    ui_open_wifi_list();
}

void ui_upload_complete(bool success) {
    // Not used yet
}
