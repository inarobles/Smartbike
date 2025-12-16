#include "ui_settings.h"
#include "ui_main.h"
#include "audio_manager.h"
#include "ui_cardio.h" // Include cardio screen header
#include "ui_power.h" // Include power screen header
#include "ui_wifi.h" // Include wifi screen header
#include "ui_bike_config.h" // Include bike config screen header
#include "ui_app_config.h" // Include app config screen header
#include "lvgl.h"
#include <stdio.h>
#include "esp_log.h"

#include "ble_client.h"

static lv_style_t style_btn;

#include "button_manager.h" // For calibration API
#include "power_calib_manager.h"

static lv_obj_t * s_calib_msgbox = NULL;
static lv_obj_t * s_calib_label = NULL; // To hold the text label handle
static lv_obj_t * s_check_pwr_msgbox = NULL;
static lv_obj_t * s_check_pwr_label = NULL;
static lv_timer_t * s_calib_timer = NULL;
static lv_timer_t * s_pwr_calib_timer = NULL; // Separate timer for power calibration
static lv_timer_t * s_check_pwr_timer = NULL;

static void msgbox_close_event_cb(lv_event_t * e)
{
    lv_obj_t * mbox = lv_event_get_user_data(e);
    lv_msgbox_close(mbox);
    
    // Clear handles
    if (mbox == s_calib_msgbox) {
        s_calib_msgbox = NULL;
        s_calib_label = NULL;
    }
    if (mbox == s_check_pwr_msgbox) {
        if (s_check_pwr_timer) {
            lv_timer_del(s_check_pwr_timer);
            s_check_pwr_timer = NULL;
        }
        s_check_pwr_msgbox = NULL;
        s_check_pwr_label = NULL;
    }
}

static void check_pwr_timer_cb(lv_timer_t * timer) {
    if (s_check_pwr_label && lv_obj_is_valid(s_check_pwr_label)) {
        int16_t ble_pwr = ble_client_get_power();
        int16_t est_pwr = power_calib_get_estimate();
        int16_t diff = est_pwr - ble_pwr;
        float err_pct = 0.0f;
        if (ble_pwr > 0) {
            err_pct = ((float)diff / (float)ble_pwr) * 100.0f;
        }
        
        char msg[256];
        snprintf(msg, sizeof(msg),
            "Potencia Real (BLE): %d W\n"
            "Calculada: %d W\n"
            "Diferencia: %d W\n"
            "ERROR: %.1f %%",
            ble_pwr, est_pwr, diff, err_pct);
            
        lv_label_set_text(s_check_pwr_label, msg);
    }
}

static void pwr_calib_timer_cb(lv_timer_t * timer) {
    power_calib_status_t status;
    if (power_calib_get_status(&status)) {
        // Update MsgBox
        if (s_calib_msgbox && s_calib_label) {
             const char * stage_str = status.is_stable ? "Muestreando..." : "Estabilizando...";
             if (status.current_rpm < 40) stage_str = "PEDALEA MAS RAPIDO!";
             
             char msg[256];
             snprintf(msg, sizeof(msg), 
                      "Paso %d/%d (%.0f%%)\n"
                      "Voltaje: %.2f V\n"
                      "RPM: %d  |  Potencia: %d W\n\n"
                      "Estado: %s", 
                      status.step_index, status.total_steps, status.progress_percent * 100,
                      status.target_voltage,
                      status.current_rpm, status.current_watts,
                      stage_str);
             
             lv_label_set_text(s_calib_label, msg);
        }
    } else if (power_calib_is_finished()) {
        // Finished
        if (s_calib_msgbox) {
             lv_msgbox_close(s_calib_msgbox);
             s_calib_msgbox = NULL;
             s_calib_label = NULL;
        }
        
        lv_obj_t * mbox = lv_msgbox_create(NULL);
        lv_msgbox_add_title(mbox, "Calibracion Potencia");
        lv_msgbox_add_text(mbox, "Proceso completado correctamente.\nTabla guardada.");
        lv_obj_t * btn = lv_msgbox_add_footer_button(mbox, "OK");
        lv_obj_add_event_cb(btn, msgbox_close_event_cb, LV_EVENT_CLICKED, mbox);
        lv_obj_center(mbox);

        lv_timer_del(timer);
        s_pwr_calib_timer = NULL;
    } else if (power_calib_has_failed()) {
        // Failed
        if (s_calib_msgbox) {
             lv_msgbox_close(s_calib_msgbox);
             s_calib_msgbox = NULL;
             s_calib_label = NULL;
        }
        
        lv_obj_t * mbox = lv_msgbox_create(NULL);
        lv_msgbox_add_title(mbox, "Error Calibracion");
        lv_msgbox_add_text(mbox, "Fallo el proceso.\nPosible causa: Freno no calibrado.\n\nPor favor, ejecute 'Calibrar Freno' primero.");
        lv_obj_t * btn = lv_msgbox_add_footer_button(mbox, "OK");
        lv_obj_add_event_cb(btn, msgbox_close_event_cb, LV_EVENT_CLICKED, mbox);
        lv_obj_center(mbox);

        lv_timer_del(timer);
        s_pwr_calib_timer = NULL;
    }
}

static void calib_timer_cb(lv_timer_t * timer) {
    if (!button_manager_is_calibrating()) {
        // Finished
        float min_v=0, max_v=0;
        bool success = button_manager_get_calibration_result(&min_v, &max_v);
        
        // Close the "Calibrating..." msgbox
        if (s_calib_msgbox) {
             lv_msgbox_close(s_calib_msgbox);
             s_calib_msgbox = NULL;
             s_calib_label = NULL;
        }

        // Create Result MsgBox
        lv_obj_t * mbox = lv_msgbox_create(NULL);
        
        if (success) {
             char msg[128];
             snprintf(msg, sizeof(msg), "Min: %.2f V\nMax: %.2f V", min_v, max_v);
             lv_msgbox_add_title(mbox, "Calibracion completada");
             lv_msgbox_add_text(mbox, msg);
        } else {
             lv_msgbox_add_title(mbox, "Error");
             lv_msgbox_add_text(mbox, "Fallo en la calibracion.\nVer logs.");
        }
        
        // Add Close Button (X in header) and/or Footer Button
        // lv_msgbox_add_close_button(mbox); 
        lv_obj_t * btn = lv_msgbox_add_footer_button(mbox, "OK");
        lv_obj_add_event_cb(btn, msgbox_close_event_cb, LV_EVENT_CLICKED, mbox);

        lv_obj_center(mbox);
        
        // Stop timer
        lv_timer_del(timer);
        s_calib_timer = NULL;
    }
}

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        audio_manager_play_event(AUDIO_EVENT_BUTTON);
        const char * txt = lv_label_get_text(lv_obj_get_child(btn, 0));
        ESP_LOGI("UI_SETTINGS", "Button clicked: %s", txt);
        
        if (strcmp(txt, "Cardio BLE") == 0) {
            ui_cardio_screen_init();
        } else if (strcmp(txt, "Potencia BLE") == 0) {
            ui_power_screen_init();
        } else if (strcmp(txt, "WIFI") == 0) {
            ui_open_wifi_list();
        } else if (strcmp(txt, "Configurar bicicleta") == 0) {
            ui_bike_configuration_screen_init();
        } else if (strcmp(txt, "Configurar APP") == 0) {
            ui_app_config_screen_init();
        } else if (strcmp(txt, "Calibrar freno") == 0) {
            // Start Calibration
            button_manager_start_calibration();
            
            // Show MsgBox (Calibrating...)
            s_calib_msgbox = lv_msgbox_create(NULL);
            lv_msgbox_add_title(s_calib_msgbox, "Calibrando Freno");
            lv_msgbox_add_text(s_calib_msgbox, "Buscando topes mecanicos...\nMovimiento automatico.");
            // No buttons -> user waits
            lv_obj_center(s_calib_msgbox);
            
            // Start polling timer
            if (s_calib_timer) lv_timer_del(s_calib_timer);
            s_calib_timer = lv_timer_create(calib_timer_cb, 500, NULL);
            
        } else if (strcmp(txt, "Calibrar potencia") == 0) {
             // Start Power Calibration
             power_calib_init(); // Ensure task is created
             power_calib_start();
             
             s_calib_msgbox = lv_msgbox_create(NULL);
             lv_msgbox_add_title(s_calib_msgbox, "Calibrando Potencia");
             s_calib_label = lv_msgbox_add_text(s_calib_msgbox, "Iniciando...\nMantengase pedaleando."); // Capture label
             lv_obj_center(s_calib_msgbox);
             
             if (s_pwr_calib_timer) lv_timer_del(s_pwr_calib_timer);
             s_pwr_calib_timer = lv_timer_create(pwr_calib_timer_cb, 500, NULL);

        } else if (strcmp(txt, "Comprobar potencia") == 0) {
             s_check_pwr_msgbox = lv_msgbox_create(NULL);
             lv_msgbox_add_title(s_check_pwr_msgbox, "Verificacion Potencia");
             s_check_pwr_label = lv_msgbox_add_text(s_check_pwr_msgbox, "Esperando datos...");
             
             lv_obj_t * btn = lv_msgbox_add_footer_button(s_check_pwr_msgbox, "Cerrar");
             lv_obj_add_event_cb(btn, msgbox_close_event_cb, LV_EVENT_CLICKED, s_check_pwr_msgbox);
             
             lv_obj_center(s_check_pwr_msgbox);
             
             if (s_check_pwr_timer) lv_timer_del(s_check_pwr_timer);
             s_check_pwr_timer = lv_timer_create(check_pwr_timer_cb, 500, NULL);
             
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
    create_button(cont, "Comprobar potencia");
    create_button(cont, "Calibrar freno");
    create_button(cont, "Configurar bicicleta");
    create_button(cont, "Configurar APP");
    create_button(cont, "Volver");
}
