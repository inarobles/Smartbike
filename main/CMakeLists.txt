idf_component_register(SRCS "main.c"
                      "treadmill_state.c"
                      "audio.c"
                      "ui.c"
                      "ui_wifi.c"
                      "button_handler.c"
                      "ble_client.c"
                      "wifi_client.c"
                      "wifi_manager.c"
                      "cm_master.c"
                      "i2c_driver.c"
                      "display_driver.c"
                      "touch_driver.c"
                      "fonts/chivo_mono_100.c"
                      "fonts/chivo_mono_70.c"
                INCLUDE_DIRS "."
                PRIV_REQUIRES nvs_flash
                              bt
                              esp_hosted
                              esp_wifi_remote
                              esp_http_client
                              esp-tls
                              esp_lvgl_port
                              # esp_lcd_touch_gsl3680  # TODO: Crear componente local
                              esp_lcd_jd9165
                              cm_protocol
)

idf_component_get_property(LVGL_LIB lvgl__lvgl COMPONENT_LIB)
target_compile_options(
    ${LVGL_LIB}
    PRIVATE
        -DLV_LVGL_H_INCLUDE_SIMPLE
        -DLV_USE_DEMO_MUSIC
)

if(CONFIG_ESP_BROOKESIA_MEMORY_USE_CUSTOM)
    idf_build_get_property(build_components BUILD_COMPONENTS)
    foreach(COMPONENT ${build_components})
        if(COMPONENT MATCHES "esp-brookesia" OR COMPONENT MATCHES "espressif__esp-brookesia")
            set(TARGET_COMPONENT ${COMPONENT})
            break()
        endif()
    endforeach()

    if(TARGET_COMPONENT STREQUAL "")
        message(FATAL_ERROR "Component 'esp-brookesia' not found.")
    else()
        idf_component_get_property(ESP_BROOKESIA_LIB ${TARGET_COMPONENT} COMPONENT_LIB)
    endif()

    target_compile_options(
        ${ESP_BROOKESIA_LIB}
        PUBLIC
            "-DESP_BROOKESIA_MEMORY_CUSTOM_MALLOC(x)=heap_caps_aligned_alloc(1, x, MALLOC_CAP_SPIRAM)"
            "-DESP_BROOKESIA_MEMORY_CUSTOM_FREE=free"
    )
endif()

if(CONFIG_LV_MEM_CUSTOM)
    target_compile_options(
        ${LVGL_LIB}
        PRIVATE
            "-DLV_MEM_CUSTOM_ALLOC(x)=heap_caps_malloc(x, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)"
    )
endif()
