#include "button_manager.h"
#include "mcp23017.h"
#include "bike_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "audio_manager.h" // For beep feedback
#include "bsp/esp-bsp.h"
#include "driver/gpio.h"

#include "ina3221.h"
#include "esp_timer.h"
#include <math.h>

static const char *TAG = "BTN_MGR";

// Motor Pins
#define MOTOR_PIN_1  GPIO_NUM_3
#define MOTOR_PIN_2  GPIO_NUM_2
#define MOTOR_PULSE_DURATION_MS 150

// Calibration Constants
#define CALIB_STABLE_THRESHOLD_V  0.02f // Voltage must change less than this
#define CALIB_STABLE_TIME_MS      1000  // Duration to consider stable (stall)
#define CALIB_TIMEOUT_MS          12000 // Max time to find a limit

// Button Mappings (Port A[0..7], Port B[0..7])
// ... (Mappings kept same)
#define BTN_PLATE_DEC  (1 << 7)
#define BTN_PLATE_RST  (1 << 6)
#define BTN_PLATE_INC  (1 << 5)

#define BTN_GEAR_DEC   (1 << 0)
#define BTN_GEAR_INC   (1 << 1)
#define BTN_GEAR_RST   (1 << 2)

typedef enum {
    CALIB_IDLE,
    CALIB_START,
    CALIB_FIND_MIN, // Moving Backward (Release) to 0%
    CALIB_FIND_MAX, // Moving Forward (Brake) to 100%
    CALIB_GOTO_VOLTAGE, // Closed-loop control to target
    CALIB_COMPLETE,
    CALIB_FAILED
} calib_state_t;

static calib_state_t s_calib_state = CALIB_IDLE;
static int64_t s_calib_start_time = 0;
static int64_t s_calib_stable_start_time = 0;
static float s_calib_last_voltage = -1.0f;
static float s_calib_result_min = 0.0f;
static float s_calib_result_max = 0.0f;
static float s_calib_target_voltage = 0.0f; // Kept for legacy compatibility if needed, but not used in new logic
static float s_calib_final_target_voltage = 0.0f;
static float s_calib_internal_target_voltage = 0.0f;
static bool  s_calib_hysteresis_active = false;

static void stop_motor() {
    gpio_set_level(MOTOR_PIN_1, 0);
    gpio_set_level(MOTOR_PIN_2, 0);
}

static void drive_motor_forward() {
    gpio_set_level(MOTOR_PIN_1, 1);
    gpio_set_level(MOTOR_PIN_2, 0);
}

static void drive_motor_backward() {
    gpio_set_level(MOTOR_PIN_1, 0);
    gpio_set_level(MOTOR_PIN_2, 1);
}

static void motor_pulse(bool forward) {
    if (forward) drive_motor_forward();
    else drive_motor_backward();
    vTaskDelay(pdMS_TO_TICKS(MOTOR_PULSE_DURATION_MS));
    stop_motor();
}

// --- Calibration API ---
void button_manager_start_calibration(void) {
    s_calib_state = CALIB_START;
}

void button_manager_set_target_voltage(float target_v) {
    s_calib_state = CALIB_GOTO_VOLTAGE;
    s_calib_final_target_voltage = target_v; // Always set final target
    
    // Hysteresis capability: Always approach from below
    // If we are currently ABOVE the target, we must go BELOW it first.
    float current_v = 0;
    if (ina3221_read_bus_voltage(1, &current_v) == ESP_OK) {
        if (current_v > target_v) {
            // We are above. Go lower first.
            s_calib_internal_target_voltage = target_v - 0.15f; // Go 150mV below
            if (s_calib_internal_target_voltage < 0) s_calib_internal_target_voltage = 0; // Ensure not negative
            s_calib_hysteresis_active = true;
            ESP_LOGI(TAG, "Hysteresis: Moving to %.2fV first, then %.2fV", s_calib_internal_target_voltage, s_calib_final_target_voltage);
        } else {
            // We are below or at target. Go direct.
            s_calib_internal_target_voltage = target_v;
            s_calib_hysteresis_active = false;
        }
    } else {
         // If sensor read fails, proceed without hysteresis for now
         s_calib_internal_target_voltage = target_v;
         s_calib_hysteresis_active = false;
         ESP_LOGW(TAG, "Failed to read voltage for hysteresis decision, going direct.");
    }
    
    s_calib_start_time = esp_timer_get_time() / 1000;
}

bool button_manager_is_at_target(void) {
    // Only "At Target" if we are stable at the FINAL target and NOT in hysteresis intermediate step
    if (s_calib_state != CALIB_GOTO_VOLTAGE || s_calib_hysteresis_active) return false;
    
    float voltage = 0.0f;
    if (ina3221_read_bus_voltage(1, &voltage) == ESP_OK) {
        if (fabs(voltage - s_calib_final_target_voltage) < 0.03f) { // 30mV tolerance
             return true;
        }
    }
    return false;
}

bool button_manager_is_calibrating(void) {
    return (s_calib_state != CALIB_IDLE && s_calib_state != CALIB_COMPLETE && s_calib_state != CALIB_FAILED);
}

bool button_manager_get_calibration_result(float *min_v, float *max_v) {
    if (s_calib_state == CALIB_COMPLETE) {
        *min_v = s_calib_result_min;
        *max_v = s_calib_result_max;
        return true;
    }
    return false;
}

static void processing_calibration(void) {
    float voltage = 0.0f;
    // Read Channel 1
    if (ina3221_read_bus_voltage(1, &voltage) != ESP_OK) {
        ESP_LOGE(TAG, "Calib: Failed to read voltage");
        return; 
    }

    int64_t now = esp_timer_get_time() / 1000; // ms

    switch (s_calib_state) {
        case CALIB_START:
            ESP_LOGI(TAG, "Calib: Starting. Finding MIN (Backward)...");
            stop_motor();
            // Need to set up state
            s_calib_start_time = now;
            s_calib_stable_start_time = now;
            s_calib_last_voltage = voltage;
            drive_motor_backward(); // Start moving back
            s_calib_state = CALIB_FIND_MIN;
            break;

        case CALIB_FIND_MIN:
        case CALIB_FIND_MAX:
            // Check Timeout
            if ((now - s_calib_start_time) > CALIB_TIMEOUT_MS) {
                ESP_LOGE(TAG, "Calib: Timeout finding limit");
                stop_motor();
                s_calib_state = CALIB_FAILED;
                break;
            }

            // Check Stability (Stall detection)
            if (fabs(voltage - s_calib_last_voltage) < CALIB_STABLE_THRESHOLD_V) {
                // Voltage is stable
                if ((now - s_calib_stable_start_time) > CALIB_STABLE_TIME_MS) {
                    // Stable for enough time -> Found Limit
                    stop_motor();
                    
                    if (s_calib_state == CALIB_FIND_MIN) {
                        s_calib_result_min = voltage;
                        ESP_LOGI(TAG, "Calib: Found MIN: %.2fV. Now finding MAX (Forward)...", voltage);
                        // Setup for Next Phase
                        s_calib_start_time = now; // Reset timeout for next phase
                        s_calib_stable_start_time = now + 1000; // Give time for motor start surge/change?
                         // Actually, give a small pause before reversing?
                        vTaskDelay(pdMS_TO_TICKS(500)); 
                        
                        s_calib_last_voltage = voltage; // Reset tracking
                        drive_motor_forward();
                        s_calib_state = CALIB_FIND_MAX;
                        
                        // Wait a bit to avoid immediate stability detection on start
                         vTaskDelay(pdMS_TO_TICKS(200)); 
                         s_calib_stable_start_time = esp_timer_get_time() / 1000; 
                    } else {
                        // Finding MAX
                        s_calib_result_max = voltage;
                        ESP_LOGI(TAG, "Calib: Found MAX: %.2fV. Done!", voltage);
                        
                        // Save config
                        bike_config_t *cfg = bike_config_get();
                        cfg->brake_min_voltage = s_calib_result_min;
                        cfg->brake_max_voltage = s_calib_result_max;
                        bike_config_save();
                        
                        s_calib_state = CALIB_COMPLETE;
                    }
                }
            } else {
                // Voltage changed significantly, reset stable timer
                s_calib_stable_start_time = now;
                s_calib_last_voltage = voltage;
            }
            break;

        case CALIB_GOTO_VOLTAGE:
            // Closed Loop Control with Hysteresis Support
            
            // Check if we are in hysteresis phase (approaching intermediate target)
            float effective_target = s_calib_hysteresis_active ? s_calib_internal_target_voltage : s_calib_final_target_voltage;
            float tolerance = s_calib_hysteresis_active ? 0.05f : 0.02f; // Wider tolerance for intermediate step

            // Simple P-controller
            if (voltage < (effective_target - tolerance)) {
                drive_motor_forward();
            } else if (voltage > (effective_target + tolerance)) {
                drive_motor_backward();
            } else {
                stop_motor(); 
                
                // If we reached target
                if (s_calib_hysteresis_active) {
                    // We reached the intermediate "below" target.
                    // Now engage final target (which is higher)
                    ESP_LOGI(TAG, "Hysteresis: Reached intermediate %.2fV. Now going to final %.2fV", voltage, s_calib_final_target_voltage);
                    s_calib_hysteresis_active = false;
                    // Reset start time to avoid timeout
                    s_calib_start_time = esp_timer_get_time() / 1000;
                    // Loop continues next cycle
                } else {
                    // We are at final target. Ready.
                }
            }
            
            // Timeout safety
            if ((now - s_calib_start_time) > 10000) { 
                 ESP_LOGW(TAG, "Calib: Timeout reaching target %.2f (Curr: %.2f)", effective_target, voltage);
                 stop_motor();
                 // If hysteresis timeout, maybe just abort hysteresis and try direct? 
                 if (s_calib_hysteresis_active) {
                     s_calib_hysteresis_active = false;
                     s_calib_start_time = now;
                 }
            }
            break;
            
        case CALIB_IDLE:
        case CALIB_COMPLETE:
        case CALIB_FAILED:
            stop_motor();
            break;
    }
}

static void button_task(void *arg) {
    uint8_t last_port_b = 0;
    mcp23017_read_ports(NULL, &last_port_b);

    while (1) {
        // If Calibrating, take over control
        if (s_calib_state != CALIB_IDLE && s_calib_state != CALIB_COMPLETE && s_calib_state != CALIB_FAILED) {
            processing_calibration();
             vTaskDelay(pdMS_TO_TICKS(100));
             continue; // Skip button reading
        }

        uint8_t curr_port_b = 0;
        esp_err_t err = mcp23017_read_ports(NULL, &curr_port_b);
        
        if (err == ESP_OK) {
            uint8_t changed = curr_port_b ^ last_port_b;
            uint8_t pressed = changed & curr_port_b;

            if (pressed) {
                // ... (Button logic remains the same) ...
                ESP_LOGI(TAG, "Pressed mask: 0x%02X", pressed);
                bool action_taken = false;

                // --- Plate Logic ---
                if (pressed & BTN_PLATE_DEC) {
                    if (bike_config_shift_chainring_down()) action_taken = true;
                }
                if (pressed & BTN_PLATE_INC) {
                    if (bike_config_shift_chainring_up()) action_taken = true;
                }
                if (pressed & BTN_PLATE_RST) {
                    // 3rd Button Plate Side -> Increase Resistance (Motor Forward)
                    motor_pulse(true);
                    action_taken = true;
                }

                // --- Gear Logic ---
                if (pressed & BTN_GEAR_DEC) { 
                    bike_config_shift_cassette_up(); 
                    action_taken = true;
                }
                if (pressed & BTN_GEAR_INC) { 
                    bike_config_shift_cassette_down();
                    action_taken = true;
                }
                if (pressed & BTN_GEAR_RST) { 
                    // 3rd Button Gear Side -> Decrease Resistance (Motor Backward)
                    motor_pulse(false);
                    action_taken = true;
                }
                
                if (action_taken) {
                    audio_manager_play_beep(); 
                    bike_config_save(); 
                }
            }
            last_port_b = curr_port_b;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // 10Hz polling
    }
}

void button_manager_init(void) {
    // ... (Init logic remains the same) ...
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MOTOR_PIN_1) | (1ULL << MOTOR_PIN_2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    stop_motor(); 

    i2c_master_bus_handle_t i2c_bus = bsp_i2c_get_handle();
    mcp23017_init(i2c_bus);
    
    xTaskCreate(button_task, "btn_mgr", 4096, NULL, 5, NULL);
}
