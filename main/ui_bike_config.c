#include "ui_bike_config.h"
#include "bike_config.h"
#include "ui_settings.h" // To go back
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h> // for strtol/atof
#include "esp_log.h"

static const char *TAG = "UI_BIKE_CONFIG";

static lv_obj_t *s_ta_cassette[MAX_CASSETTE_COGS];
static lv_obj_t *s_ta_chainring[MAX_CHAINRINGS];
static lv_obj_t *s_dropdown_tire;
// static lv_obj_t *s_roller_rim_front; // Or dropdown
// static lv_obj_t *s_roller_rim_rear;
static lv_obj_t *s_ta_weight;
static lv_obj_t *s_ta_cyclist_weight;
static lv_obj_t *s_kb;

// Styles
static lv_style_t style_title;
// static lv_style_t style_container;

static void save_btn_event_cb(lv_event_t * e)
{
    // Save logic
    bike_config_t *cfg = bike_config_get();
    
    // 1. Cassette
    memset(cfg->cassette_teeth, 0, MAX_CASSETTE_COGS);
    int c_idx = 0;
    for(int i=0; i<MAX_CASSETTE_COGS; i++) {
        const char* txt = lv_textarea_get_text(s_ta_cassette[i]);
        int val = atoi(txt);
        if (val > 0) {
            cfg->cassette_teeth[c_idx++] = (uint8_t)val;
        }
    }
    // TODO: Sort the cassette array (Small to Large or Large to Small?)
    // Convention: Smallest cog (Hardest gear) at index 0?
    // Let's sort ascending: 11, 12, 13...
    for(int i=0; i<MAX_CASSETTE_COGS-1; i++) {
        for(int j=0; j<MAX_CASSETTE_COGS-i-1; j++) {
            if (cfg->cassette_teeth[j] > cfg->cassette_teeth[j+1] && cfg->cassette_teeth[j+1] != 0) {
                uint8_t temp = cfg->cassette_teeth[j];
                cfg->cassette_teeth[j] = cfg->cassette_teeth[j+1];
                cfg->cassette_teeth[j+1] = temp;
            } else if (cfg->cassette_teeth[j] == 0 && cfg->cassette_teeth[j+1] != 0) {
                 // Push zeros to end
                uint8_t temp = cfg->cassette_teeth[j];
                cfg->cassette_teeth[j] = cfg->cassette_teeth[j+1];
                cfg->cassette_teeth[j+1] = temp;
            }
        }
    }
    
    // 2. Chainrings
    memset(cfg->chainring_teeth, 0, MAX_CHAINRINGS);
    int cr_idx = 0;
    for(int i=0; i<MAX_CHAINRINGS; i++) {
        const char* txt = lv_textarea_get_text(s_ta_chainring[i]);
        int val = atoi(txt);
        if (val > 0) {
            cfg->chainring_teeth[cr_idx++] = (uint8_t)val;
        }
    }
    // Sort ascending: 34, 50...
     for(int i=0; i<MAX_CHAINRINGS-1; i++) {
        for(int j=0; j<MAX_CHAINRINGS-i-1; j++) {
            if (cfg->chainring_teeth[j] > cfg->chainring_teeth[j+1] && cfg->chainring_teeth[j+1] != 0) {
                uint8_t temp = cfg->chainring_teeth[j];
                cfg->chainring_teeth[j] = cfg->chainring_teeth[j+1];
                cfg->chainring_teeth[j+1] = temp;
            } else if (cfg->chainring_teeth[j] == 0 && cfg->chainring_teeth[j+1] != 0) {
                uint8_t temp = cfg->chainring_teeth[j];
                cfg->chainring_teeth[j] = cfg->chainring_teeth[j+1];
                cfg->chainring_teeth[j+1] = temp;
            }
        }
    }

    // 3. Tires
    cfg->tire_selection_index = lv_dropdown_get_selected(s_dropdown_tire);
    
    // 4. Weight
    const char* w_txt = lv_textarea_get_text(s_ta_weight);
    cfg->bike_weight_kg = atof(w_txt);
    
    const char* cw_txt = lv_textarea_get_text(s_ta_cyclist_weight);
    cfg->cyclist_weight_kg = atof(cw_txt);
    
    // 5. Rims (optional storage)
    // cfg->rim_profile_front = lv_dropdown_get_selected(s_roller_rim_front); 
    
    bike_config_save();
    
    ESP_LOGI(TAG, "Configuration saved.");
    ui_settings_screen_init(); // Go back
}

static void cancel_btn_event_cb(lv_event_t * e)
{
    ui_settings_screen_init(); // Go back without saving
}

static void ta_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED || code == LV_EVENT_FOCUSED) {
        if(s_kb) {
            lv_keyboard_set_textarea(s_kb, ta);
            lv_obj_clear_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if(code == LV_EVENT_DEFOCUSED) {
        if(s_kb) {
             lv_keyboard_set_textarea(s_kb, NULL);
             lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui_bike_configuration_screen_init(void)
{
    bike_config_t *cfg = bike_config_get();

    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, lv_color_hex(0xAAAAAA));
    lv_style_set_text_font(&style_title, &lv_font_montserrat_20); // Fallback if needed
    
    lv_obj_t * scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_color(scr, lv_color_white(), 0);

    lv_obj_t * cont = lv_obj_create(scr);
    lv_obj_set_size(cont, lv_pct(100), lv_pct(100)); // Leave space for KB?
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);

    // Title
    lv_obj_t * l_title = lv_label_create(cont);
    lv_label_set_text(l_title, "Configurar Bicicleta");
    lv_obj_set_style_text_color(l_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(l_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(l_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(l_title);

    // --- Section: Piñones ---
    lv_obj_t * sec_cassette = lv_obj_create(cont);
    lv_obj_set_size(sec_cassette, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(sec_cassette, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(sec_cassette, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sec_cassette, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(sec_cassette, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(sec_cassette, 5, 0);
    
    lv_obj_t * l_cass = lv_label_create(cont); // Put label before container
    lv_label_set_text(l_cass, "Cassette");
    lv_obj_set_style_text_color(l_cass, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_parent(l_cass, cont);
    lv_obj_move_to_index(l_cass, 1);
    
    for(int i=0; i<MAX_CASSETTE_COGS; i++) {
        s_ta_cassette[i] = lv_textarea_create(sec_cassette);
        lv_obj_set_size(s_ta_cassette[i], 50, 40);
        lv_textarea_set_one_line(s_ta_cassette[i], true);
        lv_textarea_set_max_length(s_ta_cassette[i], 2);
        if (cfg->cassette_teeth[i] > 0) {
            char buf[4];
            sprintf(buf, "%d", cfg->cassette_teeth[i]);
            lv_textarea_set_text(s_ta_cassette[i], buf);
        } else {
            lv_textarea_set_text(s_ta_cassette[i], "");
        }
        lv_textarea_set_placeholder_text(s_ta_cassette[i], "-");
        lv_obj_add_event_cb(s_ta_cassette[i], ta_event_cb, LV_EVENT_ALL, NULL);
    }

    // --- Section: Platos ---
    lv_obj_t * l_rings = lv_label_create(cont);
    lv_label_set_text(l_rings, "Platos");
    lv_obj_set_style_text_color(l_rings, lv_color_hex(0xFFFFFF), 0);
    
    lv_obj_t * sec_rings = lv_obj_create(cont);
    lv_obj_set_size(sec_rings, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(sec_rings, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(sec_rings, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sec_rings, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(sec_rings, 5, 0);

    for(int i=0; i<MAX_CHAINRINGS; i++) {
        s_ta_chainring[i] = lv_textarea_create(sec_rings);
        lv_obj_set_size(s_ta_chainring[i], 50, 40);
        lv_textarea_set_one_line(s_ta_chainring[i], true);
        lv_textarea_set_max_length(s_ta_chainring[i], 2);
        if (cfg->chainring_teeth[i] > 0) {
            char buf[4];
            sprintf(buf, "%d", cfg->chainring_teeth[i]);
            lv_textarea_set_text(s_ta_chainring[i], buf);
        } else {
             lv_textarea_set_text(s_ta_chainring[i], "");
        }
        lv_textarea_set_placeholder_text(s_ta_chainring[i], "-");
        lv_obj_add_event_cb(s_ta_chainring[i], ta_event_cb, LV_EVENT_ALL, NULL);
    }
    
    // --- Section: Wheels & Weight ---
    
    // Split into two sub-containers or just use labels
    
    lv_obj_t * sec_misc = lv_obj_create(cont);
    lv_obj_set_size(sec_misc, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(sec_misc, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(sec_misc, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sec_misc, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(sec_misc, 30, 0); // Gap between tire and weight sections

    // Tire Section
    lv_obj_t * cont_tire = lv_obj_create(sec_misc);
    lv_obj_set_size(cont_tire, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(cont_tire, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont_tire, 0, 0);
    lv_obj_set_flex_flow(cont_tire, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont_tire, 0, 0);
    
    lv_obj_t * l_tire = lv_label_create(cont_tire);
    lv_label_set_text(l_tire, "Cubierta");
    lv_obj_set_style_text_color(l_tire, lv_color_hex(0xFFFFFF), 0);
    
    s_dropdown_tire = lv_dropdown_create(cont_tire);
    lv_dropdown_set_options(s_dropdown_tire, "700x23C\n700x25C\n700x28C\n700x30C\n700x32C");
    lv_dropdown_set_selected(s_dropdown_tire, cfg->tire_selection_index);
    lv_obj_set_width(s_dropdown_tire, 150);

    // Bike Weight Section
    lv_obj_t * cont_weight = lv_obj_create(sec_misc);
    lv_obj_set_size(cont_weight, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(cont_weight, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont_weight, 0, 0);
    lv_obj_set_flex_flow(cont_weight, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont_weight, 0, 0);

    lv_obj_t * l_weight = lv_label_create(cont_weight);
    lv_label_set_text(l_weight, "Peso bicicleta");
    lv_obj_set_style_text_color(l_weight, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t * sub_weight = lv_obj_create(cont_weight);
    lv_obj_set_size(sub_weight, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(sub_weight, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sub_weight, 0, 0);
    lv_obj_set_flex_flow(sub_weight, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(sub_weight, 0, 0);
    lv_obj_set_style_pad_gap(sub_weight, 10, 0);

    s_ta_weight = lv_textarea_create(sub_weight);
    lv_obj_set_width(s_ta_weight, 80);
    lv_textarea_set_one_line(s_ta_weight, true);
    char w_buf[10];
    sprintf(w_buf, "%.1f", cfg->bike_weight_kg);
    lv_textarea_set_text(s_ta_weight, w_buf);
    lv_obj_add_event_cb(s_ta_weight, ta_event_cb, LV_EVENT_ALL, NULL);
    
    lv_obj_t * l_kg = lv_label_create(sub_weight);
    lv_label_set_text(l_kg, "kg");
    lv_obj_set_style_text_color(l_kg, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(l_kg);

    // Cyclist Weight Section
    lv_obj_t * cont_cyclist = lv_obj_create(sec_misc);
    lv_obj_set_size(cont_cyclist, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(cont_cyclist, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont_cyclist, 0, 0);
    lv_obj_set_flex_flow(cont_cyclist, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(cont_cyclist, 0, 0);

    lv_obj_t * l_cweight = lv_label_create(cont_cyclist);
    lv_label_set_text(l_cweight, "Peso ciclista");
    lv_obj_set_style_text_color(l_cweight, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t * sub_cweight = lv_obj_create(cont_cyclist);
    lv_obj_set_size(sub_cweight, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(sub_cweight, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sub_cweight, 0, 0);
    lv_obj_set_flex_flow(sub_cweight, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(sub_cweight, 0, 0);
    lv_obj_set_style_pad_gap(sub_cweight, 10, 0);

    s_ta_cyclist_weight = lv_textarea_create(sub_cweight);
    lv_obj_set_width(s_ta_cyclist_weight, 80);
    lv_textarea_set_one_line(s_ta_cyclist_weight, true);
    char cw_buf[10];
    sprintf(cw_buf, "%.1f", cfg->cyclist_weight_kg);
    lv_textarea_set_text(s_ta_cyclist_weight, cw_buf);
    lv_obj_add_event_cb(s_ta_cyclist_weight, ta_event_cb, LV_EVENT_ALL, NULL);
    
    lv_obj_t * l_ckg = lv_label_create(sub_cweight);
    lv_label_set_text(l_ckg, "kg");
    lv_obj_set_style_text_color(l_ckg, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(l_ckg);

    // --- Buttons ---
    lv_obj_t * btn_cont = lv_obj_create(cont);
    lv_obj_set_size(btn_cont, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(btn_cont, 20, 0);

    lv_obj_t * btn_save = lv_button_create(btn_cont);
    lv_obj_t * l_save = lv_label_create(btn_save);
    lv_label_set_text(l_save, "GUARDAR");
    lv_obj_add_event_cb(btn_save, save_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(btn_save, 140, 50);

    lv_obj_t * btn_cancel = lv_button_create(btn_cont);
    lv_obj_t * l_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(l_cancel, "CANCELAR");
    lv_obj_add_event_cb(btn_cancel, cancel_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x888888), 0);
    lv_obj_set_size(btn_cancel, 140, 50);

    // Keyboard (Hidden initially)
    s_kb = lv_keyboard_create(scr);
    lv_keyboard_set_mode(s_kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(s_kb, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_screen_load(scr);
}
