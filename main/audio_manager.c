#include "audio_manager.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "bsp/esp-bsp.h" 
#include "driver/i2c_master.h" 
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <math.h>

static const char *TAG = "AUDIO_MANAGER";

// Hardware Pin Definition
#define AMP_EN_GPIO         GPIO_NUM_20

// I2S Config (48kHz High Quality)
#define AUDIO_SAMPLE_RATE   48000 
#define I2S_MCLK_IO         GPIO_NUM_NC
#define I2S_BCLK_IO         GPIO_NUM_12
#define I2S_WS_IO           GPIO_NUM_10
#define I2S_DOUT_IO         GPIO_NUM_9
#define I2S_DIN_IO          GPIO_NUM_11

// ES8311 Register Map
#define ES8311_ADDR             0x18
#define ES8311_RESET_REG00      0x00
#define ES8311_CLK_MAN1_REG01   0x01
#define ES8311_CLK_MAN2_REG02   0x02
#define ES8311_CLK_MAN3_REG03   0x03
#define ES8311_CLK_MAN4_REG04   0x04
#define ES8311_CLK_MAN5_REG05   0x05
#define ES8311_CLK_MAN6_REG06   0x06
#define ES8311_CLK_MAN7_REG07   0x07
#define ES8311_CLK_MAN8_REG08   0x08
#define ES8311_ADC_REG15        0x15
#define ES8311_ADC_REG17        0x17
#define ES8311_DAC_REG32        0x32
#define ES8311_SYSTEM_REG0D     0x0D
#define ES8311_SYSTEM_REG0E     0x0E
#define ES8311_SYSTEM_REG12     0x12 
#define ES8311_SYSTEM_REG13     0x13
#define ES8311_SYSTEM_REG14     0x14
#define ES8311_GP_REG45         0x45
#define ES8311_SDPIN_REG09      0x09
#define ES8311_SDPOUT_REG0A     0x0A

static i2s_chan_handle_t tx_handle = NULL;
static i2c_master_dev_handle_t es8311_handle = NULL;
static uint8_t s_volume = 128; // Default ~50%

static esp_err_t reg_write(uint8_t reg, uint8_t val) {
    if (!es8311_handle) return ESP_FAIL;
    uint8_t data[2] = {reg, val};
    return i2c_master_transmit(es8311_handle, data, 2, 50); // 50ms timeout
}

static esp_err_t init_i2s(void) {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; 
    
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &tx_handle, NULL), TAG, "I2S create failed");

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE), 
        // CRITICAL: Use PHILIPS format to match Codec I2S default
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCLK_IO,
            .bclk = I2S_BCLK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DOUT_IO,
            .din = I2S_DIN_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = true, // CRITICAL: Invert BCLK to fix Phase Timing Skew
                .ws_inv = false,
            },
        },
    };
    
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(tx_handle, &std_cfg), TAG, "I2S init std failed");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(tx_handle), TAG, "I2S enable failed");
    return ESP_OK;
}

static esp_err_t manual_es8311_init(void) {
    ESP_LOGI(TAG, "Starting Manual ES8311 Init (Production Profile)...");
    
    i2c_master_bus_handle_t bus_handle = bsp_i2c_get_handle();
    if (!bus_handle) {
        ESP_LOGW(TAG, "I2C bus not initialized, skipping ES8311");
        return ESP_FAIL;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ES8311_ADDR,
        .scl_speed_hz = 100000,
    };
    
    esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &es8311_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to add ES8311 to I2C bus: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    // 0. RESET / CSM SETUP
    reg_write(ES8311_RESET_REG00, 0x80); 
    
    // 1. Clock Configuration (Slave, MCLK from BCLK)
    reg_write(ES8311_CLK_MAN1_REG01, 0xBF); 
    reg_write(ES8311_CLK_MAN2_REG02, 0x18); 
    
    // Reg 03/04: OSR 16x for High Speed (48k)
    reg_write(ES8311_CLK_MAN3_REG03, 0x10); 
    reg_write(ES8311_CLK_MAN4_REG04, 0x10); 
    
    // Reg 05: Divs = 0
    reg_write(ES8311_CLK_MAN5_REG05, 0x00);

    // Reg 06: BCLK Div
    reg_write(ES8311_CLK_MAN6_REG06, 0x03); 

    // Reg 07, 08: LRCK Divider 256
    reg_write(ES8311_CLK_MAN7_REG07, 0x00);
    reg_write(ES8311_CLK_MAN8_REG08, 0xFF);

    // 2. Format (I2S, 16bit)
    reg_write(ES8311_SDPIN_REG09, 0x0C); 
    reg_write(ES8311_SDPOUT_REG0A, 0x0C);

    // 3. System Power Up
    reg_write(ES8311_SYSTEM_REG0D, 0x01); 
    reg_write(ES8311_SYSTEM_REG0E, 0x02); 
    reg_write(ES8311_SYSTEM_REG12, 0x00); 
    reg_write(ES8311_SYSTEM_REG13, 0x10); 
    reg_write(ES8311_SYSTEM_REG14, 0x1A);
    reg_write(ES8311_GP_REG45, 0x00);
    
    // 4. Volume (Clean Gain Profile)
    reg_write(ES8311_ADC_REG17, 0x80); 
    reg_write(ES8311_DAC_REG32, 0x80); 

    ESP_LOGI(TAG, "ES8311 Init Success");
    return ESP_OK;
}

esp_err_t audio_manager_init(void) {
    // 1. Initialize I2S first
    if (init_i2s() != ESP_OK) {
        ESP_LOGE(TAG, "I2S Init Failed");
        return ESP_FAIL;
    }

    // 2. Send silence to prevent initial pops (I2S needs to start with data)
    size_t silence_size = 16384; 
    void* silence = calloc(1, silence_size);
    if(silence) {
        size_t bytes_written;
        i2s_channel_write(tx_handle, silence, silence_size, &bytes_written, 100);
        free(silence);
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    // 3. Initialize ES8311 codec (BEFORE enabling amplifier!)
    if (manual_es8311_init() != ESP_OK) {
        ESP_LOGE(TAG, "ES8311 codec init failed");
        return ESP_FAIL;
    }
    
    // Small delay to let codec stabilize
    vTaskDelay(pdMS_TO_TICKS(50));

    // 4. Configure amplifier GPIO
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << AMP_EN_GPIO);
    gpio_config(&io_conf);
    
    // 5. Load desired volume from NVS BEFORE enabling amplifier
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        uint8_t stored_vol = 0;
        err = nvs_get_u8(my_handle, "vol_level", &stored_vol);
        if (err == ESP_OK) {
            s_volume = stored_vol;
            ESP_LOGI(TAG, "Loaded Volume from NVS: %d", s_volume);
        } else {
            ESP_LOGI(TAG, "No volume saved in NVS, using default: %d", s_volume);
        }
        nvs_close(my_handle);
    } else {
        ESP_LOGE(TAG, "Error opening NVS handle");
    }
    
    // 6. Apply volume to codec while amp is still OFF (prevents pop)
    audio_manager_set_volume(s_volume);
    
    // 7. Small delay before enabling amplifier
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 8. Finally, enable amplifier with codec already configured
    gpio_set_level(AMP_EN_GPIO, 1);
    ESP_LOGI(TAG, "Amplifier Enabled");

    return ESP_OK;
}

void audio_manager_play(const void *data, size_t size) {
    if (!tx_handle) return;
    size_t bytes_written = 0;
    i2s_channel_write(tx_handle, data, size, &bytes_written, portMAX_DELAY);
}

static void generate_tone(double freq, int duration_ms, double amplitude) {
    size_t samp_rate = AUDIO_SAMPLE_RATE;
    size_t buffer_size = (samp_rate * duration_ms) / 1000 * 2 * sizeof(int16_t);
    int16_t *buffer = malloc(buffer_size);
    if (!buffer) return;

    for (size_t i = 0; i < buffer_size / sizeof(int16_t); i += 2) {
        double t = (double)i / (2.0 * samp_rate);
        int16_t value = (int16_t)(amplitude * sin(2.0 * M_PI * freq * t));
        buffer[i] = value;
        buffer[i+1] = value;
    }
    audio_manager_play(buffer, buffer_size);
    free(buffer);
}

static void play_event_task(void *pvParameters) {
    audio_event_t event = (audio_event_t)pvParameters;
    switch (event) {
        case AUDIO_EVENT_STARTUP:
            // C5 (523), E5 (659), G5 (783) - Power Chord Arpeggio
            generate_tone(523.25, 150, 5000.0);
            generate_tone(659.25, 150, 5000.0);
            generate_tone(783.99, 400, 5000.0);
            break;
        case AUDIO_EVENT_BUTTON:
            // Short High Pitch Tick
            generate_tone(2000.0, 50, 4000.0);
            break;
        case AUDIO_EVENT_COUNTDOWN_STEP:
            // 3... 2... 1... (Low Beep)
            generate_tone(500.0, 300, 5000.0);
            break;
        case AUDIO_EVENT_COUNTDOWN_GO:
            // GO! (High Long Beep)
            generate_tone(1000.0, 600, 5000.0);
            break;
    }
    vTaskDelete(NULL);
}

void audio_manager_play_event(audio_event_t event) {
    // Spawn a temporary task to handle the audio event to prevent blocking the UI
    // Cast event to void* (safe as it's an enum/int fitting in 32-bit pointer)
    xTaskCreate(play_event_task, "audio_evt", 4096, (void*)event, 5, NULL);
}

void audio_manager_play_beep(void) {
    // Legacy mapping (just use Button sound for beep)
     audio_manager_play_event(AUDIO_EVENT_BUTTON);
}

void audio_manager_set_volume(uint8_t volume) {
    // Volume 0-255 (0x00 - 0xFF)
    s_volume = volume;
    
    // Only write to codec if handle is initialized
    if (es8311_handle) {
        esp_err_t ret;
        ret = reg_write(ES8311_DAC_REG32, volume);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to set DAC volume: %s", esp_err_to_name(ret));
        }
        ret = reg_write(ES8311_ADC_REG17, volume);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to set ADC volume: %s", esp_err_to_name(ret));
        }
        ESP_LOGI(TAG, "Volume set to %d", volume);
    } else {
        ESP_LOGW(TAG, "ES8311 handle not initialized, volume not applied");
    }
}

uint8_t audio_manager_get_volume(void) {
    return s_volume;
}

void audio_manager_save_volume(void) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        err = nvs_set_u8(my_handle, "vol_level", s_volume);
        if (err == ESP_OK) {
            err = nvs_commit(my_handle);
            ESP_LOGI(TAG, "Volume Saved to NVS: %d", s_volume);
        }
        nvs_close(my_handle);
    } else {
        ESP_LOGE(TAG, "Error opening NVS to save volume");
    }
}
