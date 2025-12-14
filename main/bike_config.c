#include "bike_config.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "BIKE_CONFIG";
static const char *NVS_NAMESPACE = "bike_cfg";

static bike_config_t s_config;

// Circumference lookupt table in mm
static const uint16_t TIRE_CIRCUMFERENCE_MM[] = {
    2096, // 700x23C
    2105, // 700x25C
    2136, // 700x28C
    2146, // 700x30C
    2155  // 700x32C
};

static const char* TIRE_NAMES[] = {
    "700x23C",
    "700x25C",
    "700x28C",
    "700x30C",
    "700x32C"
};

void bike_config_init(void) {
    // Set defaults
    memset(&s_config, 0, sizeof(s_config));
    
    // Default Cassette: 11-28 (11 speed)
    uint8_t default_cassette[] = {11, 12, 13, 14, 15, 17, 19, 21, 23, 25, 28};
    for(int i=0; i<11; i++) s_config.cassette_teeth[i] = default_cassette[i];
    
    // Default Chainrings: 50/34
    s_config.chainring_teeth[0] = 34;
    s_config.chainring_teeth[1] = 50;
    
    s_config.current_cassette_index = 0; // Smallest cog (hardest) usually at index 0 or N? 
    // Usually cassettes are listed small to large. Let's assume index 0 = smallest tooth count (Hardest gear)
    // Actually, usually index 0 is the smallest cog (11t).
    
    s_config.current_chainring_index = 0; // Smallest ring (34t)
    
    s_config.tire_selection_index = TIRE_700_25C;
    s_config.wheel_circumference_mm = TIRE_CIRCUMFERENCE_MM[TIRE_700_25C];
    
    s_config.bike_weight_kg = 7.5f;

    // Default Brake Voltages (Safe defaults before calibration)
    s_config.brake_min_voltage = 0.0f; 
    s_config.brake_max_voltage = 3.3f;
    
    // Load from NVS
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        size_t required_size = sizeof(bike_config_t);
        err = nvs_get_blob(my_handle, "config", &s_config, &required_size);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG, "Error reading config from NVS");
        }
        nvs_close(my_handle);
    } else {
        ESP_LOGE(TAG, "Error opening NVS handle");
    }
    
    ESP_LOGI(TAG, "Config initialized. Cassette[0]: %d, Tire: %s, Brake: %.2fV-%.2fV", 
             s_config.cassette_teeth[0], TIRE_NAMES[s_config.tire_selection_index],
             s_config.brake_min_voltage, s_config.brake_max_voltage);
}

void bike_config_save(void) {
    // Update derived values first
    if (s_config.tire_selection_index < TIRE_TYPE_MAX) {
        s_config.wheel_circumference_mm = TIRE_CIRCUMFERENCE_MM[s_config.tire_selection_index];
    }

    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        err = nvs_set_blob(my_handle, "config", &s_config, sizeof(bike_config_t));
        if (err == ESP_OK) {
            nvs_commit(my_handle);
            ESP_LOGI(TAG, "Config saved to NVS");
        } else {
            ESP_LOGE(TAG, "Failed to write blob");
        }
        nvs_close(my_handle);
    }
}

bike_config_t* bike_config_get(void) {
    return &s_config;
}

float bike_config_calculate_speed(float cadence_rpm) {
    if (cadence_rpm <= 0.1f) return 0.0f;
    
    uint8_t teeth_front = s_config.chainring_teeth[s_config.current_chainring_index];
    uint8_t teeth_rear = s_config.cassette_teeth[s_config.current_cassette_index];
    
    if (teeth_front == 0 || teeth_rear == 0) return 0.0f; // Safety
    
    float ratio = (float)teeth_front / (float)teeth_rear;
    float meters_per_minute = cadence_rpm * ratio * (s_config.wheel_circumference_mm / 1000.0f);
    float kph = meters_per_minute * 60.0f / 1000.0f;
    
    return kph;
}

uint8_t bike_config_get_cassette_count(void) {
    uint8_t count = 0;
    for (int i=0; i<MAX_CASSETTE_COGS; i++) {
        if (s_config.cassette_teeth[i] != 0) count++;
    }
    return count;
}

uint8_t bike_config_get_chainring_count(void) {
    uint8_t count = 0;
    for (int i=0; i<MAX_CHAINRINGS; i++) {
        if (s_config.chainring_teeth[i] != 0) count++;
    }
    return count;
}

bool bike_config_shift_cassette_up(void) {
    // "Up" usually means harder gear -> smaller cog -> LOWER index in our sorted array (assuming sorted small to large)
    // Wait, physically "Shifting Up" (Subir marcha) means going FASTER -> Smaller cog.
    // "Shifting Down" (Bajar marcha) means EASIER -> Larger cog.
    // Let's assume the user sorts them or we sort them... 
    // Ideally we should sort the array for consistency. 
    // For now, let's assume index 0 is smallest cog (Hardest).
    
    if (s_config.current_cassette_index > 0) {
        s_config.current_cassette_index--;
        return true;
    }
    return false;
}

bool bike_config_shift_cassette_down(void) {
    // "Down" -> Easier -> Larger cog -> Higher index
    uint8_t count = bike_config_get_cassette_count();
    if (s_config.current_cassette_index < count - 1) {
        s_config.current_cassette_index++;
        return true;
    }
    return false;
}

bool bike_config_shift_chainring_up(void) {
    // "Up" -> Harder -> Larger ring -> Higher index (usually small ring is 0, big is 1)
    uint8_t count = bike_config_get_chainring_count();
    if (s_config.current_chainring_index < count - 1) {
        s_config.current_chainring_index++;
        return true;
    }
    return false;
}

bool bike_config_shift_chainring_down(void) {
    // "Down" -> Easier -> Smaller ring -> Lower index
    if (s_config.current_chainring_index > 0) {
        s_config.current_chainring_index--;
        return true;
    }
    return false;
}

bool bike_config_set_cassette_index(uint8_t index) {
    uint8_t count = bike_config_get_cassette_count();
    if (index < count && s_config.current_cassette_index != index) {
        s_config.current_cassette_index = index;
        return true;
    }
    return false;
}

bool bike_config_set_chainring_index(uint8_t index) {
    uint8_t count = bike_config_get_chainring_count();
    if (index < count && s_config.current_chainring_index != index) {
        s_config.current_chainring_index = index;
        return true;
    }
    return false;
}

const char* bike_config_get_tire_name(uint8_t index) {
    if (index >= TIRE_TYPE_MAX) return "Unknown";
    return TIRE_NAMES[index];
}
