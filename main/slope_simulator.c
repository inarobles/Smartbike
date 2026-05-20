#include "slope_simulator.h"
#include "bike_config.h"
#include "button_manager.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "SLOPE_SIM";

static float s_current_slope = 0.0f;

// Limits for slope
#define MAX_SLOPE  20.0f
#define MIN_SLOPE -15.0f

static float clamp(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

void slope_simulator_update_resistance(void) {
    bike_config_t *cfg = bike_config_get();
    float ratio = bike_config_get_gear_ratio();
    float weight = cfg->bike_weight_kg + cfg->cyclist_weight_kg;
    
    // Formula conceptual: resistance = base + slope_factor + gear_factor + weight_factor
    // Base resistance at 0% slope, mid-gear, 80kg total
    float base = 0.20f; 
    
    // Each 1% of slope adds ~8% resistance (0.08 normalized voltage)
    float slope_factor = s_current_slope * 0.08f;
    
    // Gear ratio factor: higher ratio (harder) = more resistance
    // Ratio typical: 3.3 (50/15) to 1.3 (34/25)
    // Let's center at ~2.0
    float gear_factor = (ratio - 2.0f) * 0.15f;
    
    // Weight factor: Each 10kg above 80 adds ~5%
    float weight_factor = (weight - 80.0f) / 10.0f * 0.05f;
    
    float target_norm = base + slope_factor + gear_factor + weight_factor;
    
    // Clamp normalized value [0.0, 1.0] before mapping to calibrated voltage
    target_norm = clamp(target_norm, 0.0f, 1.0f);
    
    // Map to calibrated voltage with a small safety margin (2%) to avoid saturation timeouts
    float min_v = cfg->brake_min_voltage;
    float max_v = cfg->brake_max_voltage * 0.98f; // Stay slightly below absolute max
    
    if (!bike_config_is_calibrated()) {
        ESP_LOGW(TAG, "Brake not calibrated (%.2fV-%.2fV). Motor move aborted.", min_v, max_v);
        return;
    }

    float target_v = min_v + (target_norm * (max_v - min_v));
    
    ESP_LOGI(TAG, "Update: Slope=%.1f%%, Gear=%.2f, WeightTotal=%.1fkg -> Target=%.2fV", 
             s_current_slope, ratio, weight, target_v);
             
    button_manager_set_target_voltage(target_v);
}

esp_err_t slope_simulator_init(void) {
    s_current_slope = 0.0f;
    ESP_LOGI(TAG, "Initialized");
    return ESP_OK;
}

void slope_simulator_adjust_slope(float increment) {
    float new_slope = s_current_slope + increment;
    new_slope = clamp(new_slope, MIN_SLOPE, MAX_SLOPE);
    
    if (new_slope != s_current_slope) {
        s_current_slope = new_slope;
        ESP_LOGI(TAG, "Slope changed to %.1f%%", s_current_slope);
        slope_simulator_update_resistance();
    }
}

void slope_simulator_reset(void) {
    s_current_slope = 0.0f;
    ESP_LOGI(TAG, "Slope reset to 0%");
    slope_simulator_update_resistance();
}

float slope_simulator_get_current_slope(void) {
    return s_current_slope;
}
