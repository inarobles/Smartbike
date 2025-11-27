#include "ui_training.h"
#include <stdio.h>
#include "esp_log.h"

static lv_style_t style_cell;
static lv_style_t style_header;

static void create_value_unit_cell(lv_obj_t * parent, const char * val, const char * unit, int col, int row)
{
    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_pad_gap(cont, 5, 0);

    lv_obj_t * l_val = lv_label_create(cont);
    lv_label_set_text(l_val, val);
    lv_obj_add_style(l_val, &style_cell, 0);

    lv_obj_t * l_unit = lv_label_create(cont);
    lv_label_set_text(l_unit, unit);
    lv_obj_add_style(l_unit, &style_header, 0);

    lv_obj_set_grid_cell(cont, LV_GRID_ALIGN_CENTER, col, 1, LV_GRID_ALIGN_CENTER, row, 1);
}

void ui_training_screen_init(void)
{
    // Initialize Styles
    lv_style_init(&style_cell);
    lv_style_set_text_color(&style_cell, lv_color_hex(0xFFFFFF)); // White text
    // Use extra large font for numbers (approx 48)
    #if LV_FONT_MONTSERRAT_48
    lv_style_set_text_font(&style_cell, &lv_font_montserrat_48);
    #elif LV_FONT_MONTSERRAT_46
    lv_style_set_text_font(&style_cell, &lv_font_montserrat_46);
    #elif LV_FONT_MONTSERRAT_44
    lv_style_set_text_font(&style_cell, &lv_font_montserrat_44);
    #elif LV_FONT_MONTSERRAT_42
    lv_style_set_text_font(&style_cell, &lv_font_montserrat_42);
    #elif LV_FONT_MONTSERRAT_40
    lv_style_set_text_font(&style_cell, &lv_font_montserrat_40);
    #endif

    lv_style_init(&style_header);
    lv_style_set_text_color(&style_header, lv_color_hex(0x888888)); // Grey text for headers
    // Use medium-large font for letters (approx 30)
    #if LV_FONT_MONTSERRAT_30
    lv_style_set_text_font(&style_header, &lv_font_montserrat_30);
    #elif LV_FONT_MONTSERRAT_28
    lv_style_set_text_font(&style_header, &lv_font_montserrat_28);
    #endif

    lv_obj_t * scr = lv_obj_create(NULL); // Create a new screen
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0); // Black background
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Create a grid container
    static int32_t col_dsc[] = {80, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST}; // First col fixed 80px
    static int32_t row_dsc[] = {50, 70, 70, 70, 70, LV_GRID_TEMPLATE_LAST}; // Header row 50px, others 70px

    lv_obj_t * grid = lv_obj_create(scr);
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);

    lv_obj_set_size(grid, lv_pct(95), LV_SIZE_CONTENT);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 10, 10);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_row(grid, 0, 0);
    lv_obj_set_style_pad_column(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    
    // Row 1: Header (Empty, MED, ACT, MAX)
    lv_obj_t * cell;
    
    // 0,0 Empty
    cell = lv_label_create(grid);
    lv_label_set_text(cell, "");
    lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    // 0,1 MED
    cell = lv_label_create(grid);
    lv_label_set_text(cell, "MED");
    lv_obj_add_style(cell, &style_header, 0);
    lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    // 0,2 ACT
    cell = lv_label_create(grid);
    lv_label_set_text(cell, "ACT");
    lv_obj_add_style(cell, &style_header, 0);
    lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    // 0,3 MAX
    cell = lv_label_create(grid);
    lv_label_set_text(cell, "MAX");
    lv_obj_add_style(cell, &style_header, 0);
    lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_CENTER, 0, 1);


    // Helper macro to add a row of data
    #define ADD_ROW(row_idx, label_text, val1, val2, val3) \
        cell = lv_label_create(grid); \
        lv_label_set_text(cell, label_text); \
        lv_obj_add_style(cell, &style_header, 0); \
        lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, row_idx, 1); \
        \
        cell = lv_label_create(grid); \
        lv_label_set_text(cell, val1); \
        lv_obj_add_style(cell, &style_cell, 0); \
        lv_obj_set_style_transform_scale(cell, 282, 0); \
        lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, row_idx, 1); \
        \
        cell = lv_label_create(grid); \
        lv_label_set_text(cell, val2); \
        lv_obj_add_style(cell, &style_cell, 0); \
        lv_obj_set_style_transform_scale(cell, 282, 0); \
        lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, row_idx, 1); \
        \
        cell = lv_label_create(grid); \
        lv_label_set_text(cell, val3); \
        lv_obj_add_style(cell, &style_cell, 0); \
        lv_obj_set_style_transform_scale(cell, 282, 0); \
        lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_CENTER, row_idx, 1);

    // Row 2: KPH, 27, 33, 65
    ADD_ROW(1, "KPH", "27", "33", "65");

    // Row 3: RPM, 65, 72, 97
    ADD_ROW(2, "RPM", "65", "72", "97");

    // Row 4: WAT, 187, 198, 348
    ADD_ROW(3, "WAT", "187", "198", "348");

    // Row 5: PUL, 97, 102, 168
    ADD_ROW(4, "PUL", "97", "102", "168");

    // Bottom Strip
    lv_obj_t * bottom_strip = lv_obj_create(scr);
    lv_obj_set_size(bottom_strip, lv_pct(100), 205); // Reduced height by 10px (215 -> 205)
    lv_obj_align(bottom_strip, LV_ALIGN_TOP_MID, 10, 355); // Position unchanged
    lv_obj_set_style_bg_color(bottom_strip, lv_color_hex(0x333333), 0); // Dark Grey background
    lv_obj_set_style_bg_opa(bottom_strip, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bottom_strip, 0, 0);
    lv_obj_set_style_radius(bottom_strip, 0, 0);
    lv_obj_set_scrollbar_mode(bottom_strip, LV_SCROLLBAR_MODE_OFF); // Disable scrollbar

    static int32_t col_dsc_bottom[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static int32_t row_dsc_bottom[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST}; // 3 rows

    lv_obj_t * grid_bottom = lv_obj_create(bottom_strip);
    lv_obj_set_grid_dsc_array(grid_bottom, col_dsc_bottom, row_dsc_bottom);
    lv_obj_set_size(grid_bottom, lv_pct(100), lv_pct(100));
    lv_obj_align(grid_bottom, LV_ALIGN_CENTER, 0, -10); // Adjusted offset (-15 -> -10) to keep content static
    lv_obj_set_style_bg_opa(grid_bottom, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid_bottom, 0, 0);
    lv_obj_set_scrollbar_mode(grid_bottom, LV_SCROLLBAR_MODE_OFF); // Disable scrollbar
    lv_obj_set_style_pad_row(grid_bottom, 20, 0); // Reduced separation slightly to fit 3 rows

    lv_obj_t * label;

    // Row 1: TOTAL | LAP 1
    label = lv_label_create(grid_bottom);
    lv_label_set_text(label, "TOTAL");
    lv_obj_add_style(label, &style_header, 0);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    label = lv_label_create(grid_bottom);
    lv_label_set_text(label, "LAP 1");
    lv_obj_add_style(label, &style_header, 0);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    // Row 2: 32,5 km | 7,3 km
    create_value_unit_cell(grid_bottom, "32,5", "km", 0, 1);
    create_value_unit_cell(grid_bottom, "7,3", "km", 1, 1);

    // Row 3: 1:03:34 | 0:19:21
    label = lv_label_create(grid_bottom);
    lv_label_set_text(label, "1:03:34");
    lv_obj_add_style(label, &style_cell, 0);
    lv_obj_set_style_translate_y(label, 10, 0); // Shift down 10px
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);

    label = lv_label_create(grid_bottom);
    lv_label_set_text(label, "0:19:21");
    lv_obj_add_style(label, &style_cell, 0);
    lv_obj_set_style_translate_y(label, 10, 0); // Shift down 10px
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 2, 1);

    lv_screen_load(scr);

    // Third Strip
    lv_obj_t * third_strip = lv_obj_create(scr);
    lv_obj_set_size(third_strip, lv_pct(100), 165); // Height increased by 15px (150 -> 165)
    lv_obj_align(third_strip, LV_ALIGN_TOP_MID, 10, 565); // Below bottom strip (355 + 205 + 5)
    lv_obj_set_style_bg_opa(third_strip, LV_OPA_TRANSP, 0); // Transparent container
    lv_obj_set_style_border_width(third_strip, 0, 0);
    lv_obj_set_style_pad_all(third_strip, 0, 0);
    lv_obj_set_scrollbar_mode(third_strip, LV_SCROLLBAR_MODE_OFF);

    static int32_t col_dsc_third[] = {LV_GRID_FR(1), LV_GRID_FR(4), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static int32_t row_dsc_third[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    lv_obj_t * grid_third = lv_obj_create(third_strip);
    lv_obj_set_grid_dsc_array(grid_third, col_dsc_third, row_dsc_third);
    lv_obj_set_size(grid_third, lv_pct(100), lv_pct(100));
    lv_obj_center(grid_third);
    lv_obj_set_style_bg_opa(grid_third, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid_third, 0, 0);
    lv_obj_set_style_pad_all(grid_third, 0, 0);
    lv_obj_set_style_pad_column(grid_third, 0, 0); // No gap between columns
    lv_obj_set_scrollbar_mode(grid_third, LV_SCROLLBAR_MODE_OFF);

    // Left Part (Grey)
    lv_obj_t * part_left = lv_obj_create(grid_third);
    lv_obj_set_style_bg_color(part_left, lv_color_hex(0x333333), 0); // Dark Grey
    lv_obj_set_style_bg_opa(part_left, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(part_left, 0, 0);
    lv_obj_set_style_radius(part_left, 0, 0);
    lv_obj_set_grid_cell(part_left, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    
    LV_IMG_DECLARE(trasero);
    lv_obj_t * img_trasero = lv_image_create(part_left);
    lv_image_set_src(img_trasero, &trasero);
    lv_image_set_scale(img_trasero, 179); // 70% scale (256 * 0.7)
    lv_obj_set_style_image_recolor(img_trasero, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_image_recolor_opa(img_trasero, LV_OPA_COVER, 0);
    lv_obj_set_style_translate_y(img_trasero, -30, 0); // Shift up 30px
    lv_obj_center(img_trasero);

    lv_obj_t * label_left = lv_label_create(part_left);
    lv_label_set_text(label_left, "1/11");
    lv_obj_set_style_text_color(label_left, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_left, &lv_font_montserrat_38, 0); // Font 38
    lv_obj_align(label_left, LV_ALIGN_CENTER, 0, 30); // Position below the icon

    // Center Part (White)
    lv_obj_t * part_center = lv_obj_create(grid_third);
    lv_obj_set_style_bg_color(part_center, lv_color_hex(0xFFFFFF), 0); // White
    lv_obj_set_style_bg_opa(part_center, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(part_center, 0, 0);
    lv_obj_set_style_radius(part_center, 0, 0);
    lv_obj_set_grid_cell(part_center, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    // Right Part (Grey)
    lv_obj_t * part_right = lv_obj_create(grid_third);
    lv_obj_set_style_bg_color(part_right, lv_color_hex(0x333333), 0); // Dark Grey
    lv_obj_set_style_bg_opa(part_right, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(part_right, 0, 0);
    lv_obj_set_style_radius(part_right, 0, 0);
    lv_obj_set_grid_cell(part_right, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    LV_IMG_DECLARE(delantero);
    lv_obj_t * img_delantero = lv_image_create(part_right);
    lv_image_set_src(img_delantero, &delantero);
    lv_image_set_scale(img_delantero, 179); // 70% scale (256 * 0.7)
    lv_obj_set_style_image_recolor(img_delantero, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_image_recolor_opa(img_delantero, LV_OPA_COVER, 0);
    lv_obj_set_style_translate_y(img_delantero, -30, 0); // Shift up 30px
    lv_obj_center(img_delantero);

    lv_obj_t * label_right = lv_label_create(part_right);
    lv_label_set_text(label_right, "1/2");
    lv_obj_set_style_text_color(label_right, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_right, &lv_font_montserrat_38, 0); // Font 38
    lv_obj_align(label_right, LV_ALIGN_CENTER, 0, 30); // Position below the icon
}
