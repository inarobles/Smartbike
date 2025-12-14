#include "cadence_sensor.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Configuration
// Configuration
#define CADENCE_GPIO_PIN    GPIO_NUM_48
#define DEBOUNCE_TIME_US    300000 // 300ms debounce (Max 200 RPM)
#define TIMEOUT_US          3000000
#define RING_BUFFER_SIZE    3

static volatile int64_t s_last_pulse_time = 0;
static volatile uint16_t s_current_rpm = 0;

// Debug
static volatile uint32_t s_isr_count = 0;

// Smoothing Buffer
static volatile uint16_t s_rpm_buffer[RING_BUFFER_SIZE] = {0};
static volatile uint8_t s_buffer_idx = 0;

// Stats
static uint32_t s_accumulated_rpm = 0;
static uint32_t s_sample_count = 0;
static uint16_t s_max_rpm = 0;

// ISR Handler
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    s_isr_count++; // Debug count
    int64_t now = esp_timer_get_time();
    int64_t diff = now - s_last_pulse_time;

    // Use current time for visual debug even if bounced, or update on valid pulse?
    // Let's update on ANY edge for visual debug to see noise.
    // s_last_pulse_time = now; // <--- If we do this, diff logic breaks.
    
    // Better: Update separate debug timestamp? Or valid timestamp?
    // Use valid timestamp for logic.
    // But for visual debug, User wants to see "detection".
    // If we filter it, they don't see it.
    // Let's stick to valid pulses for the *main* timestamp.
    
    if (diff > DEBOUNCE_TIME_US) {
        // Valid pulse
        s_last_pulse_time = now; // Update timestamp here for visual debug + logic

        // Logic
        uint32_t inst_rpm = (60000000) / diff;
        if (inst_rpm <= 300) { 
             s_rpm_buffer[s_buffer_idx] = (uint16_t)inst_rpm;
             s_buffer_idx = (s_buffer_idx + 1) % RING_BUFFER_SIZE;
             
             uint32_t sum = 0;
             uint8_t count = 0;
             for (int i = 0; i < RING_BUFFER_SIZE; i++) {
                 if (s_rpm_buffer[i] > 0) {
                     sum += s_rpm_buffer[i];
                     count++;
                 }
             }
             if (count > 0) s_current_rpm = (uint16_t)(sum / count);
             else s_current_rpm = (uint16_t)inst_rpm;
             
             if (s_current_rpm > s_max_rpm) s_max_rpm = s_current_rpm;
             if (s_current_rpm > 0) {
                 s_accumulated_rpm += s_current_rpm;
                 s_sample_count++;
             }
        }
    }
}

void cadence_sensor_init(void)
{
    // GPIO Config
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CADENCE_GPIO_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, 
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE 
    };
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) printf("CADENCE: gpio_config failed %d\n", err);

    // Suppress "GPIO isr service already installed" error log
    esp_log_level_set("gpio", ESP_LOG_NONE);
    if (gpio_install_isr_service(0) != ESP_OK) {
        // Service already installed by another component (e.g. Touch or BSP), which is fine.
    }
    esp_log_level_set("gpio", ESP_LOG_INFO);

    err = gpio_isr_handler_add(CADENCE_GPIO_PIN, gpio_isr_handler, NULL);
    if (err != ESP_OK) printf("CADENCE: handler_add failed %d\n", err);
    else printf("CADENCE: Handler added to GPIO %d\n", CADENCE_GPIO_PIN);
}

uint16_t cadence_sensor_get_rpm(void)
{
    // Check timeout
    int64_t now = esp_timer_get_time();
    if ((now - s_last_pulse_time) > TIMEOUT_US) {
        s_current_rpm = 0;
    }
    return s_current_rpm;
}

uint16_t cadence_sensor_get_avg_rpm(void)
{
    if (s_sample_count == 0) return 0;
    return (uint16_t)(s_accumulated_rpm / s_sample_count);
}

uint16_t cadence_sensor_get_max_rpm(void)
{
    return s_max_rpm;
}

uint32_t cadence_sensor_get_isr_count(void)
{
    return s_isr_count;
}

int64_t cadence_sensor_get_last_pulse_time(void)
{
    return s_last_pulse_time;
}

void cadence_sensor_reset_stats(void)
{
    s_accumulated_rpm = 0;
    s_sample_count = 0;
    s_max_rpm = 0;
    // Clear buffer
    for(int i=0; i<RING_BUFFER_SIZE; i++) s_rpm_buffer[i] = 0;
    s_current_rpm = 0;
}
