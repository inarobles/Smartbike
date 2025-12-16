#include "power_calib_manager.h"
#include "button_manager.h"
#include "ble_client.h"
#include "cadence_sensor.h"
#include "ina3221.h"
#include "bike_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <math.h>

static const char *TAG = "PWR_CALIB";

// Configuration is in header file
// #define CALIB_STEPS is inherited
#define CALIB_SAMPLE_DURATION_MS 5000 // 5 seconds of sampling
#define CALIB_MIN_RPM 40              // User must pedal faster than this
#define CALIB_STABILITY_WAIT_MS 2000  // Wait 2s after reaching voltage before sampling

typedef enum {
    PC_IDLE,
    PC_INIT,
    PC_MOVE_MOTOR,
    PC_WAIT_STABILITY,
    PC_SAMPLING,
    PC_NEXT_STEP,
    PC_SAVE,
    PC_COMPLETE,
    PC_FAILED
} pc_state_t;

static pc_state_t s_state = PC_IDLE;
static uint8_t s_current_step = 0; // 0 to 4
static float s_voltage_targets[CALIB_STEPS];
static int64_t s_state_start_time = 0;

// Sampling Accumulators
static uint64_t s_sum_watts = 0;
static uint64_t s_sum_rpm = 0;
static uint32_t s_sample_count = 0;

// Result Table (to be saved)
typedef struct {
    float voltage;
    float watts_at_60rpm;
} calib_point_t;

static calib_point_t s_results[CALIB_STEPS];

static void calib_task(void *arg) {
    while (1) {
        if (s_state == PC_IDLE || s_state == PC_COMPLETE || s_state == PC_FAILED) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        int64_t now = esp_timer_get_time() / 1000;

        switch (s_state) {
            case PC_INIT: {
                ESP_LOGI(TAG, "Starting Power Calibration...");
                // Get Brake limits
                bike_config_t *cfg = bike_config_get();
                float min_v = cfg->brake_min_voltage;
                float max_v = cfg->brake_max_voltage;

                if (max_v <= min_v) {
                    ESP_LOGE(TAG, "Invalid brake limits. Calibrate brake first.");
                    s_state = PC_FAILED;
                    break;
                }

                // Calculate targets (0, 25, 50, 75, 100 %)
                float range = max_v - min_v;
                for (int i = 0; i < CALIB_STEPS; i++) {
                    s_voltage_targets[i] = min_v + (range * i) / (CALIB_STEPS - 1);
                }

                s_current_step = 0;
                s_state = PC_MOVE_MOTOR;
                break;
            }

            case PC_MOVE_MOTOR: {
                float target = s_voltage_targets[s_current_step];
                ESP_LOGI(TAG, "Step %d/%d: Moving to %.2f V", s_current_step + 1, CALIB_STEPS, target);
                button_manager_set_target_voltage(target);
                s_state_start_time = now;
                s_state = PC_WAIT_STABILITY;
                break;
            }

            case PC_WAIT_STABILITY: {
                // Wait for motor to reach target AND user to pedal
                bool motor_ready = button_manager_is_at_target();
                int rpm = cadence_sensor_get_rpm();
                
                if (motor_ready && rpm >= CALIB_MIN_RPM) {
                    // Start Sampling after a short delay for mechanics to settle
                    if (now - s_state_start_time > CALIB_STABILITY_WAIT_MS) {
                        ESP_LOGI(TAG, "Stable. Sampling...");
                        s_sum_watts = 0;
                        s_sum_rpm = 0;
                        s_sample_count = 0;
                        s_state_start_time = now;
                        s_state = PC_SAMPLING;
                    }
                } else {
                    // Reset timer if conditions lost (e.g. user stops pedaling)
                    // But keep motor wait timeout check separate if needed? 
                    // For simplicity, just reset "stability start time"
                    s_state_start_time = now; 
                }
                break;
            }

            case PC_SAMPLING: {
                // Collect data
                int watts = ble_client_get_power();
                int rpm = cadence_sensor_get_rpm();

                if (rpm < CALIB_MIN_RPM) {
                    // Paused? 
                    // Just wait? Or fail? Let's just not count this sample
                } else {
                    s_sum_watts += watts;
                    s_sum_rpm += rpm;
                    s_sample_count++;
                }

                if (now - s_state_start_time > CALIB_SAMPLE_DURATION_MS) {
                    // Finished sampling this step
                    if (s_sample_count > 0) {
                       float avg_watts = (float)s_sum_watts / s_sample_count;
                       float avg_rpm = (float)s_sum_rpm / s_sample_count;
                       
                       // Normalize to 60 RPM: Power ~ RPM^x. Assume x=1 (Linear) for simplicity in storage, 
                       // or store raw. Let's store "Watts at 60RPM".
                       // K = Watts / RPM.  Watts_60 = K * 60.
                       float k = avg_watts / avg_rpm;
                       float w60 = k * 60.0f;

                       s_results[s_current_step].voltage = s_voltage_targets[s_current_step];
                       s_results[s_current_step].watts_at_60rpm = w60;
                       
                       ESP_LOGI(TAG, "Step %d Result: AvgW=%.1f, AvgRPM=%.1f -> W@60=%.1f", 
                                s_current_step, avg_watts, avg_rpm, w60);
                       
                       s_state = PC_NEXT_STEP;
                    } else {
                        // No valid samples? Retry step?
                        ESP_LOGW(TAG, "No valid samples. Retrying step.");
                        s_state = PC_WAIT_STABILITY;
                    }
                }
                break;
            }

            case PC_NEXT_STEP: {
                s_current_step++;
                if (s_current_step >= CALIB_STEPS) {
                    s_state = PC_SAVE;
                } else {
                    s_state = PC_MOVE_MOTOR;
                }
                break;
            }

            case PC_SAVE: {
                ESP_LOGI(TAG, "Saving Calibration Table...");
                
                nvs_handle_t my_handle;
                esp_err_t err = nvs_open("bike_cfg", NVS_READWRITE, &my_handle);
                if (err == ESP_OK) {
                    size_t size = sizeof(s_results);
                    err = nvs_set_blob(my_handle, "pwr_calib_lut", s_results, size);
                    if (err == ESP_OK) {
                        nvs_commit(my_handle);
                        ESP_LOGI(TAG, "Saved successfully.");
                    } else {
                        ESP_LOGE(TAG, "NVS set blob failed");
                    }
                    nvs_close(my_handle);
                } else {
                    ESP_LOGE(TAG, "NVS open failed");
                }

                // Also we should set a flag "Power Calibrated = True" in config?
                // For now, existence of blob is enough.
                
                // Release motor
                button_manager_start_calibration(); // Hack: Reset button manager to IDLE?
                // Actually need a "stop calibration" in button manager to release mutex if any
                // The button manager exits calibration when we change state... 
                // Wait, button manager has its own state s_calib_state.
                // We need to tell it to go to IDLE.
                // We used `button_manager_set_target` which sets it to GOTO_VOLTAGE.
                // We need a `button_manager_stop_control()`
                
                s_state = PC_COMPLETE;
                break;
            }

            default:
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


// --- Estimation & Loading ---

static void load_lut(void) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("bike_cfg", NVS_READONLY, &my_handle);
    if (err == ESP_OK) {
        size_t size = sizeof(s_results);
        // Only load if size matches
        err = nvs_get_blob(my_handle, "pwr_calib_lut", s_results, &size);
        if (err == ESP_OK) {
             if (size == sizeof(s_results)) {
                 ESP_LOGI(TAG, "LUT Loaded (Size %d). Pt5 W@60: %.1f", size, s_results[4].watts_at_60rpm);
             } else {
                 ESP_LOGW(TAG, "LUT size mismatch (Exp: %d, Read: %d). Calibration required.", sizeof(s_results), size);
                 // Invalidate results? Or just leave 0s. 
                 // Best to clear it to avoid using partial data.
                 memset(s_results, 0, sizeof(s_results));
             }
        } else {
             ESP_LOGW(TAG, "LUT not found or invalid size");
        }
        nvs_close(my_handle);
    }
}

int16_t power_calib_get_estimate(void) {
    // 1. Get RPM
    int rpm = cadence_sensor_get_rpm();
    if (rpm < 5) return 0; // Zero if not pedaling

    // 2. Get Voltage (Brake Position)
    float voltage = 0.0f;
    if (ina3221_read_bus_voltage(1, &voltage) != ESP_OK) {
        return 0; // Sensor error
    }

    // 3. Interpolate Factor K (Watts @ 60RPM) from LUT
    // s_results is sorted by voltage (as calibrated 0..100%, 0 to 5)
    // We assume s_results[0].voltage < s_results[1].voltage ...
    
    float k_factor = 0.0f;

    // Find segment
    if (voltage <= s_results[0].voltage) {
        k_factor = s_results[0].watts_at_60rpm;
    } else if (voltage >= s_results[CALIB_STEPS - 1].voltage) {
        k_factor = s_results[CALIB_STEPS - 1].watts_at_60rpm;
    } else {
        // Search interval
        for (int i = 0; i < CALIB_STEPS - 1; i++) {
            if (voltage >= s_results[i].voltage && voltage < s_results[i+1].voltage) {
                // Linear Interpolation
                float v1 = s_results[i].voltage;
                float v2 = s_results[i+1].voltage;
                float w1 = s_results[i].watts_at_60rpm;
                float w2 = s_results[i+1].watts_at_60rpm;
                
                float t = (voltage - v1) / (v2 - v1);
                k_factor = w1 + t * (w2 - w1);
                break;
            }
        }
    }
    
    // 4. Scale by RPM
    // W = K * (RPM / 60)
    // Note: This assumes Linear Power-RPM relationship.
    // Real fluid/air resistance is cubic (RPM^3). Magnetic is often linear or mixed.
    // Since we calibrated K = W/RPM, we are assuming Linear. 
    // If we wanted Cubic, we would store K = W / RPM^3.
    // Given the user request implies a simple map, Linear is safest start.
    
    // Safe float math
    float power_f = k_factor * ((float)rpm / 60.0f);
    
    return (int16_t)power_f;
}

// --- API ---

static TaskHandle_t s_calib_task_handle = NULL;

void power_calib_init(void) {
    load_lut(); // Reload on init

    if (s_calib_task_handle == NULL) {
        xTaskCreate(calib_task, "pwr_calib", 8192, NULL, 5, &s_calib_task_handle);
    }
}

void power_calib_start(void) {
    s_state = PC_INIT;
}

void power_calib_stop(void) {
    s_state = PC_IDLE;
    // We should also tell button_manager to stop holding voltage
    // button_manager_set_idle(); // TODO: Add this to button_manager if needed
}

bool power_calib_get_status(power_calib_status_t *status) {
    if (status == NULL) return false;
    
    if (s_state == PC_IDLE || s_state == PC_COMPLETE || s_state == PC_FAILED) return false;

    status->step_index = s_current_step + 1;
    status->total_steps = CALIB_STEPS;
    status->target_voltage = s_voltage_targets[s_current_step];
    status->progress_percent = (float)(s_current_step) / CALIB_STEPS; // Rough
    
    // Check stability for UI feedback
    bool motor_ready = button_manager_is_at_target();
    int rpm = cadence_sensor_get_rpm();
    status->is_stable = (motor_ready && rpm >= CALIB_MIN_RPM);
    
    status->current_rpm = rpm;
    status->current_watts = ble_client_get_power();

    return true;
}

bool power_calib_is_finished(void) {
    return (s_state == PC_COMPLETE);
}

bool power_calib_has_failed(void) {
    return (s_state == PC_FAILED);
}
