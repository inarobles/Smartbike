set(LV_DEMO_DIR ../managed_components/lvgl__lvgl/demos)
file(GLOB_RECURSE LV_DEMOS_SOURCES ${LV_DEMO_DIR}/*.c)

idf_component_register(
    SRCS "main.c" "ui_main.c" "ui_training.c" "ui_settings.c" "ui_cardio.c" "ui_power.c" "ui_wifi.c" "ui_bike_config.c" "ui_app_config.c" "wifi_client.c" "wifi_manager.c" "delantero.c" "trasero.c" "icon_heart.c" "ble_client.c" "bike_config.c" "audio_manager.c" "mcp23017.c" "button_manager.c" "cadence_sensor.c" "ina3221.c" "power_calib_manager.c" "slope_simulator.c"
    INCLUDE_DIRS . ${LV_DEMO_DIR}
    REQUIRES bt esp_wifi nvs_flash esp_event esp_netif driver esp_http_client esp32_p4_function_ev_board bsp_extra esp_codec_dev app_state espressif__esp_hosted espressif__esp_wifi_remote)

idf_component_get_property(LVGL_LIB lvgl__lvgl COMPONENT_LIB)
target_compile_options(
    ${LVGL_LIB} 
    PRIVATE
        -DLV_LVGL_H_INCLUDE_SIMPLE
        -DLV_USE_DEMO_MUSIC
        -Wno-unused-function
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-function)
