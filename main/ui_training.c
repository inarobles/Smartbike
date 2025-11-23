/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "ui_training.h"
#include "icon_heart.h"
#include "delantero.h"
#include "trasero.h"

LV_FONT_DECLARE(lv_font_montserrat_48);
LV_FONT_DECLARE(lv_font_montserrat_36);

// Helper to create a grid cell
// Logical X = Visual Row Position
// Logical Y = Visual Col Position
static void create_grid_cell(lv_obj_t *parent, const char *text, const lv_font_t *font, int x, int w, int y, int h)
{
    // Container for the cell
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, w, h);
    lv_obj_set_pos(cont, x, y);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    // Style for rotated text (270 degrees)
    static lv_style_t style_text_rot;
    if (style_text_rot.prop_cnt == 0) {
        lv_style_init(&style_text_rot);
        lv_style_set_transform_angle(&style_text_rot, 2700); // 270 degrees
        lv_style_set_transform_pivot_x(&style_text_rot, LV_PCT(50));
        lv_style_set_transform_pivot_y(&style_text_rot, LV_PCT(50));
        lv_style_set_text_color(&style_text_rot, lv_color_hex(0xFFFFFF));
    }

    // Label
    lv_obj_t *lbl = lv_label_create(cont);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, font, LV_PART_MAIN);
    lv_obj_add_style(lbl, &style_text_rot, LV_PART_MAIN);
    lv_obj_center(lbl);
}

// Helper to create a grid cell with an image icon
static void create_grid_cell_icon(lv_obj_t *parent, const lv_img_dsc_t *img_src, int x, int w, int y, int h)
{
    // Container for the cell
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, w, h);
    lv_obj_set_pos(cont, x, y);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    // Style for rotated image (270 degrees)
    static lv_style_t style_img_rot;
    if (style_img_rot.prop_cnt == 0) {
        lv_style_init(&style_img_rot);
        lv_style_set_transform_angle(&style_img_rot, 2700); // 270 degrees
        lv_style_set_transform_pivot_x(&style_img_rot, LV_PCT(50));
        lv_style_set_transform_pivot_y(&style_img_rot, LV_PCT(50));
    }

    // Image
    lv_obj_t *img = lv_img_create(cont);
    lv_img_set_src(img, img_src);
    lv_obj_add_style(img, &style_img_rot, LV_PART_MAIN);
    lv_obj_set_style_img_recolor(img, lv_color_hex(0xFFFFFF), LV_PART_MAIN);  // White color
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_COVER, LV_PART_MAIN);        // Full recolor
    lv_obj_center(img);
}

void ui_training_init(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN); // Black background

    // -------------------------------------------------------------------------
    // CONT_TOP (Data): X=0, 285x480. Black.
    // 5 Rows (Visual) x 4 Cols (Visual)
    // -------------------------------------------------------------------------
    lv_obj_t *cont_top = lv_obj_create(scr);
    lv_obj_set_size(cont_top, 285, 480);
    lv_obj_set_pos(cont_top, 0, 0);
    lv_obj_set_style_bg_color(cont_top, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_top, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(cont_top, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont_top, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(cont_top, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(cont_top, LV_OBJ_FLAG_SCROLLABLE);

    // Row Heights (Logical Widths)
    int h_header = 30;
    int h_row = 62;
    
    // Row Positions (Logical X)
    int x_r0 = 0;
    int x_r1 = x_r0 + h_header;
    int x_r2 = x_r1 + h_row;
    int x_r3 = x_r2 + h_row;
    int x_r4 = x_r3 + h_row;

    // Column Widths (Logical Heights)
    // Visual Col 0 (Left-most) -> Logical Y Highest
    // Visual Col 3 (Right-most) -> Logical Y Lowest (0)
    int w_col_label = 90;  // Visual Col 0
    int w_col_data = 130;  // Visual Cols 1, 2, 3

    // Column Positions (Logical Y)
    // Y=0 is Visual Right
    int y_c3 = 0;                   // Visual Col 3 (MAX)
    int y_c2 = y_c3 + w_col_data;   // Visual Col 2 (ACT)
    int y_c1 = y_c2 + w_col_data;   // Visual Col 1 (MED)
    int y_c0 = y_c1 + w_col_data;   // Visual Col 0 (Label) -> Should be 390

    // Row 0: Headers
    create_grid_cell(cont_top, "", &lv_font_montserrat_16, x_r0, h_header, y_c0, w_col_label);
    create_grid_cell(cont_top, "MED", &lv_font_montserrat_16, x_r0, h_header, y_c1, w_col_data);
    create_grid_cell(cont_top, "ACT", &lv_font_montserrat_16, x_r0, h_header, y_c2, w_col_data);
    create_grid_cell(cont_top, "MAX", &lv_font_montserrat_16, x_r0, h_header, y_c3, w_col_data);

    // Row 1: KPH
    create_grid_cell(cont_top, "KPH", &lv_font_montserrat_24, x_r1, h_row, y_c0, w_col_label);
    create_grid_cell(cont_top, "27", &lv_font_montserrat_48, x_r1, h_row, y_c1, w_col_data);
    create_grid_cell(cont_top, "33", &lv_font_montserrat_48, x_r1, h_row, y_c2, w_col_data);
    create_grid_cell(cont_top, "57", &lv_font_montserrat_48, x_r1, h_row, y_c3, w_col_data);

    // Row 2: RPM
    create_grid_cell(cont_top, "RPM", &lv_font_montserrat_24, x_r2, h_row, y_c0, w_col_label);
    create_grid_cell(cont_top, "69", &lv_font_montserrat_48, x_r2, h_row, y_c1, w_col_data);
    create_grid_cell(cont_top, "75", &lv_font_montserrat_48, x_r2, h_row, y_c2, w_col_data);
    create_grid_cell(cont_top, "98", &lv_font_montserrat_48, x_r2, h_row, y_c3, w_col_data);

    // Row 3: WAT
    create_grid_cell(cont_top, "WAT", &lv_font_montserrat_24, x_r3, h_row, y_c0, w_col_label);
    create_grid_cell(cont_top, "198", &lv_font_montserrat_48, x_r3, h_row, y_c1, w_col_data);
    create_grid_cell(cont_top, "205", &lv_font_montserrat_48, x_r3, h_row, y_c2, w_col_data);
    create_grid_cell(cont_top, "345", &lv_font_montserrat_48, x_r3, h_row, y_c3, w_col_data);

    // Row 4: Heart (BPM)
    create_grid_cell_icon(cont_top, &icon_heart, x_r4, h_row, y_c0, w_col_label); // Heart icon
    create_grid_cell(cont_top, "105", &lv_font_montserrat_48, x_r4, h_row, y_c1, w_col_data);
    create_grid_cell(cont_top, "123", &lv_font_montserrat_48, x_r4, h_row, y_c2, w_col_data);
    create_grid_cell(cont_top, "167", &lv_font_montserrat_48, x_r4, h_row, y_c3, w_col_data);


    // -------------------------------------------------------------------------
    // CONT_MID (Summary): X=285, 162x480. Dark Grey.
    // -------------------------------------------------------------------------
    lv_obj_t *cont_mid = lv_obj_create(scr);
    lv_obj_set_size(cont_mid, 162, 480);
    lv_obj_set_pos(cont_mid, 285, 0);
    lv_obj_set_style_bg_color(cont_mid, lv_palette_darken(LV_PALETTE_GREY, 3), LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_mid, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(cont_mid, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont_mid, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(cont_mid, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(cont_mid, LV_OBJ_FLAG_SCROLLABLE);

    // Row Heights (Logical Widths) - Total 162
    int h_mid_header = 30; // Same as data panel header
    int h_mid_row = 62;    // Same as data panel rows

    // Row Positions (Logical X)
    int x_m0 = 3;  // Offset 3px down (visual)
    int x_m1 = x_m0 + h_mid_header;
    int x_m2 = x_m1 + h_mid_row;

    // Column Widths (Logical Heights) - Total 480
    int w_mid_col = 240;

    // Column Positions (Logical Y) - Visual Right (Y=0) to Visual Left (Y=480)
    int y_m_lap = 0;            // Visual Right
    int y_m_total = w_mid_col;  // Visual Left

    // Row 0: Headers (Montserrat 16 like MED)
    create_grid_cell(cont_mid, "TOTAL", &lv_font_montserrat_16, x_m0, h_mid_header, y_m_total, w_mid_col);
    create_grid_cell(cont_mid, "LAP", &lv_font_montserrat_16, x_m0, h_mid_header, y_m_lap, w_mid_col);

    // Row 1: Distance (Montserrat 48)
    create_grid_cell(cont_mid, "12.4", &lv_font_montserrat_48, x_m1, h_mid_row, y_m_total, w_mid_col);
    create_grid_cell(cont_mid, "2.2", &lv_font_montserrat_48, x_m1, h_mid_row, y_m_lap, w_mid_col);

    // Row 2: Time (Montserrat 48)
    create_grid_cell(cont_mid, "0:45:12", &lv_font_montserrat_48, x_m2, h_mid_row, y_m_total, w_mid_col);
    create_grid_cell(cont_mid, "0:05:30", &lv_font_montserrat_48, x_m2, h_mid_row, y_m_lap, w_mid_col);

    // -------------------------------------------------------------------------
    // CONT_GREY_RIGHT (Right Grey Block - Visual Right): X=447, 130x90. Dark Grey.
    // -------------------------------------------------------------------------
    lv_obj_t *cont_grey_right = lv_obj_create(scr);
    lv_obj_set_size(cont_grey_right, 130, 90);
    lv_obj_set_pos(cont_grey_right, 447, 0);
    lv_obj_set_style_bg_color(cont_grey_right, lv_palette_darken(LV_PALETTE_GREY, 3), LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_grey_right, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(cont_grey_right, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont_grey_right, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(cont_grey_right, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(cont_grey_right, LV_OBJ_FLAG_SCROLLABLE);

    // Delantero icon (88x88 scaled to 44x44)
    lv_obj_t *img_delantero = lv_img_create(cont_grey_right);
    lv_img_set_src(img_delantero, &delantero);
    lv_obj_set_style_transform_angle(img_delantero, 2700, 0); // 270 degrees
    lv_obj_set_style_transform_zoom(img_delantero, 128, 0); // 50% scale (256 = 100%)
    lv_obj_set_style_transform_pivot_x(img_delantero, LV_PCT(50), 0);
    lv_obj_set_style_transform_pivot_y(img_delantero, LV_PCT(50), 0);
    lv_obj_set_style_img_recolor(img_delantero, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(img_delantero, 255, LV_PART_MAIN);
    lv_obj_align(img_delantero, LV_ALIGN_CENTER, -35, 0); // -35 X = 35px up (visual), 0 Y = 15px left (visual)

    // Label "1/2" below delantero icon
    lv_obj_t *lbl_delantero = lv_label_create(cont_grey_right);
    lv_label_set_text(lbl_delantero, "1/2");
    lv_obj_set_style_text_font(lbl_delantero, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_transform_angle(lbl_delantero, 2700, 0); // 270 degrees
    lv_obj_set_style_transform_pivot_x(lbl_delantero, LV_PCT(50), 0);
    lv_obj_set_style_transform_pivot_y(lbl_delantero, LV_PCT(50), 0);
    lv_obj_set_style_text_color(lbl_delantero, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(lbl_delantero, LV_ALIGN_CENTER, 24, 0); // Below icon: +X = visual down

    // -------------------------------------------------------------------------
    // CONT_WHITE (White Strip): X=447, 130x300. White.
    // -------------------------------------------------------------------------
    lv_obj_t *cont_white = lv_obj_create(scr);
    lv_obj_set_size(cont_white, 130, 300);
    lv_obj_set_pos(cont_white, 447, 90);
    lv_obj_set_style_bg_color(cont_white, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_white, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(cont_white, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont_white, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(cont_white, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(cont_white, LV_OBJ_FLAG_SCROLLABLE);

    // -------------------------------------------------------------------------
    // CONT_GREY_LEFT (Left Grey Block - Visual Left): X=447, 130x90. Dark Grey.
    // -------------------------------------------------------------------------
    lv_obj_t *cont_grey_left = lv_obj_create(scr);
    lv_obj_set_size(cont_grey_left, 130, 90);
    lv_obj_set_pos(cont_grey_left, 447, 390);
    lv_obj_set_style_bg_color(cont_grey_left, lv_palette_darken(LV_PALETTE_GREY, 3), LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_grey_left, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(cont_grey_left, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont_grey_left, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(cont_grey_left, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(cont_grey_left, LV_OBJ_FLAG_SCROLLABLE);

    // Trasero icon (88x88 scaled to 44x44)
    lv_obj_t *img_trasero = lv_img_create(cont_grey_left);
    lv_img_set_src(img_trasero, &trasero);
    lv_obj_set_style_transform_angle(img_trasero, 2700, 0); // 270 degrees
    lv_obj_set_style_transform_zoom(img_trasero, 128, 0); // 50% scale (256 = 100%)
    lv_obj_set_style_transform_pivot_x(img_trasero, LV_PCT(50), 0);
    lv_obj_set_style_transform_pivot_y(img_trasero, LV_PCT(50), 0);
    lv_obj_set_style_img_recolor(img_trasero, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(img_trasero, 255, LV_PART_MAIN);
    lv_obj_align(img_trasero, LV_ALIGN_CENTER, -35, -2); // -35 X = 35px up (visual), -2 Y = 13px left (visual)

    // Label "3/11" below trasero icon
    lv_obj_t *lbl_trasero = lv_label_create(cont_grey_left);
    lv_label_set_text(lbl_trasero, "3/11");
    lv_obj_set_style_text_font(lbl_trasero, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_transform_angle(lbl_trasero, 2700, 0); // 270 degrees
    lv_obj_set_style_transform_pivot_x(lbl_trasero, LV_PCT(50), 0);
    lv_obj_set_style_transform_pivot_y(lbl_trasero, LV_PCT(50), 0);
    lv_obj_set_style_text_color(lbl_trasero, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(lbl_trasero, LV_ALIGN_CENTER, 24, -2); // Below icon: +X = visual down

    // -------------------------------------------------------------------------
    // CONT_BOT (Chart): X=577, 223x480. Black.
    // -------------------------------------------------------------------------
    lv_obj_t *cont_bot = lv_obj_create(scr);
    lv_obj_set_size(cont_bot, 223, 480);
    lv_obj_set_pos(cont_bot, 577, 0);
    lv_obj_set_style_bg_color(cont_bot, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_border_width(cont_bot, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(cont_bot, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont_bot, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(cont_bot, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(cont_bot, LV_OBJ_FLAG_SCROLLABLE);

    // Messages Label (Top Visual -> Left Logical)
    static lv_style_t style_text_rot;
    if (style_text_rot.prop_cnt == 0) {
        lv_style_init(&style_text_rot);
        lv_style_set_transform_angle(&style_text_rot, 2700);
        lv_style_set_transform_pivot_x(&style_text_rot, LV_PCT(50));
        lv_style_set_transform_pivot_y(&style_text_rot, LV_PCT(50));
        lv_style_set_text_color(&style_text_rot, lv_color_hex(0xFFFFFF));
    }
    lv_obj_t *lbl_msg = lv_label_create(cont_bot);
    lv_label_set_text(lbl_msg, "MENSAJES: ZONA 3 - MANTENER RITMO");
    lv_obj_set_style_text_font(lbl_msg, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_add_style(lbl_msg, &style_text_rot, LV_PART_MAIN);
    lv_obj_align(lbl_msg, LV_ALIGN_LEFT_MID, 20, 0); // Aligned to left of container (Top visual)

    // Chart
    // We need to rotate the chart object itself 270 degrees
    lv_obj_t *chart = lv_chart_create(cont_bot);
    lv_obj_set_size(chart, 400, 200); // Swap W/H for rotation logic
    lv_obj_align(chart, LV_ALIGN_CENTER, 20, 0);
    
    // Rotate the chart
    lv_obj_set_style_transform_angle(chart, 2700, 0);
    lv_obj_set_style_transform_pivot_x(chart, LV_PCT(50), 0);
    lv_obj_set_style_transform_pivot_y(chart, LV_PCT(50), 0);

    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x202020), LV_PART_MAIN);
    lv_chart_set_point_count(chart, 20);
    
    lv_chart_series_t *ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    for(int i = 0; i < 20; i++) {
        lv_chart_set_next_value(chart, ser1, lv_rand(10, 90));
    }
}
