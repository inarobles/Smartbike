#include "ui_app_config.h"
#include "ui_settings.h"
#include "audio_manager.h"
#include "lvgl.h"
#include <stdio.h>
#include "esp_log.h"

static lv_style_t style_arc;
static lv_style_t style_arc_knob;
static lv_style_t style_header;
static lv_style_t style_val;
static lv_style_t style_btn;

static lv_obj_t * s_vol_label = NULL;
static lv_obj_t * s_vol_arc = NULL;
static lv_obj_t * s_debug_label = NULL; // New debug label

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        audio_manager_play_event(AUDIO_EVENT_BUTTON);
        audio_manager_save_volume(); // Save to NVS
        ui_settings_screen_init(); // BACK to settings
    }
}

static void arc_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * arc = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        int32_t v = lv_arc_get_value(arc);
        if (s_vol_label) {
            lv_label_set_text_fmt(s_vol_label, "%d%%", (int)v);
        }
        
        // Map 0-100% to 0-255
        uint8_t vol_reg = (uint8_t)((v * 255) / 100);
        audio_manager_set_volume(vol_reg);
        
        if (s_debug_label) lv_label_set_text_fmt(s_debug_label, "Vol: %d", v);
    }
    // Debug other events
    else if (code == LV_EVENT_PRESSED) {
        if (s_debug_label) lv_label_set_text(s_debug_label, "Event: PRESSED");
        audio_manager_play_event(AUDIO_EVENT_BUTTON); // Audible feedback on press
    }
    else if (code == LV_EVENT_RELEASED) {
        if (s_debug_label) lv_label_set_text(s_debug_label, "Event: RELEASED");
    }
}

void ui_app_config_screen_init(void)
{
    // Initialize Styles
    static bool style_inited = false;
    if (!style_inited) {
        lv_style_init(&style_header);
        lv_style_set_text_color(&style_header, lv_color_hex(0x888888));
        #if LV_FONT_MONTSERRAT_28
        lv_style_set_text_font(&style_header, &lv_font_montserrat_28);
        #endif

        lv_style_init(&style_val);
        lv_style_set_text_color(&style_val, lv_color_hex(0xFFFFFF));
        #if LV_FONT_MONTSERRAT_48
        lv_style_set_text_font(&style_val, &lv_font_montserrat_48);
        #endif

        lv_style_init(&style_arc);
        lv_style_set_arc_color(&style_arc, lv_color_hex(0x404040)); // Back color
        lv_style_set_arc_width(&style_arc, 15);
        
        lv_style_init(&style_arc_knob);
        lv_style_set_bg_color(&style_arc_knob, lv_color_hex(0xFFFFFF));
        lv_style_set_pad_all(&style_arc_knob, 10); // Make knob larger


        lv_style_init(&style_btn);
        lv_style_set_bg_color(&style_btn, lv_color_hex(0x2D2D2D));
        lv_style_set_border_color(&style_btn, lv_color_hex(0x404040));
        lv_style_set_border_width(&style_btn, 2);
        lv_style_set_radius(&style_btn, 12);
        lv_style_set_text_color(&style_btn, lv_color_hex(0xFFFFFF));

        style_inited = true;
    }

    lv_obj_t * scr = lv_screen_active();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Title ("Configurar APP")
    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, "Configurar APP");
    lv_obj_add_style(title, &style_header, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Volume Container (300x300, WITH Scrollable default)
    lv_obj_t * vol_cont = lv_obj_create(scr);
    lv_obj_set_size(vol_cont, 300, 300);
    lv_obj_center(vol_cont);
    lv_obj_set_style_bg_opa(vol_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(vol_cont, 0, 0);
    // SCROLLABLE REMOVED to hide gray lines
    lv_obj_remove_flag(vol_cont, LV_OBJ_FLAG_SCROLLABLE);

    // Label "Volumen" (Moved down to 70)
    lv_obj_t * lbl = lv_label_create(vol_cont);
    lv_label_set_text(lbl, "Volumen");
    lv_obj_add_style(lbl, &style_header, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 70);

    // Arc (300x300)
    s_vol_arc = lv_arc_create(vol_cont);
    lv_obj_set_size(s_vol_arc, 300, 300); 
    lv_arc_set_rotation(s_vol_arc, 135);
    lv_arc_set_bg_angles(s_vol_arc, 0, 270);
    
    // Get current volume and map to %
    uint8_t current_vol = audio_manager_get_volume();
    int current_pct = (current_vol * 100) / 255;
    
    lv_arc_set_value(s_vol_arc, current_pct);
    lv_obj_add_style(s_vol_arc, &style_arc, 0);
    lv_obj_add_style(s_vol_arc, &style_arc_knob, LV_PART_KNOB); // Add knob style
    lv_obj_set_style_arc_color(s_vol_arc, lv_color_hex(0x00FF00), LV_PART_INDICATOR); // Green indicator
    lv_obj_set_style_arc_width(s_vol_arc, 15, LV_PART_INDICATOR);
    lv_obj_align(s_vol_arc, LV_ALIGN_CENTER, 0, 10);
    
    // CRITICAL: Ensure arc receives input
    lv_obj_add_flag(s_vol_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(s_vol_arc, LV_OBJ_FLAG_SCROLLABLE);
    
    // LISTEN TO LV_EVENT_ALL (Matches Step 962)
    lv_obj_add_event_cb(s_vol_arc, arc_event_cb, LV_EVENT_ALL, NULL);

    // Debug Label (Matches Step 962, but Hidden by Color)
    s_debug_label = lv_label_create(scr);
    lv_label_set_text(s_debug_label, "No Events");
    lv_obj_set_style_text_color(s_debug_label, lv_color_hex(0x000000), 0); // BLACK to hide
    lv_obj_align(s_debug_label, LV_ALIGN_TOP_LEFT, 10, 10);

    // Volume Value Label (Inside Arc)
    s_vol_label = lv_label_create(vol_cont);
    lv_label_set_text_fmt(s_vol_label, "%d%%", current_pct);
    lv_obj_add_style(s_vol_label, &style_val, 0);
    lv_obj_align(s_vol_label, LV_ALIGN_CENTER, 0, 10);


    // Back Button
    lv_obj_t * btn = lv_button_create(scr);
    lv_obj_set_size(btn, 140, 60);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_add_style(btn, &style_btn, 0);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Volver");
    lv_obj_center(btn_lbl);
    #if LV_FONT_MONTSERRAT_28
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_28, 0);
    #endif
}
