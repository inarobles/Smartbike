idf_component_register(
    SRCS "main.c"
         "waveshare_rgb_lcd_port.c"
         "lvgl_port.c"
         "ui_training.c"
         "icon_heart.c"
         "delantero.c"
         "trasero.c"
    INCLUDE_DIRS "."
)

# Esto silencia advertencias específicas de LVGL (opcional pero útil)
idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)