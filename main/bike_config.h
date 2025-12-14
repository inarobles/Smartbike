#ifndef BIKE_CONFIG_H
#define BIKE_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_CASSETTE_COGS 15
#define MAX_CHAINRINGS 4

typedef enum {
    TIRE_700_23C = 0,
    TIRE_700_25C,
    TIRE_700_28C,
    TIRE_700_30C,
    TIRE_700_32C,
    TIRE_TYPE_MAX
} tire_type_t;

typedef struct {
    uint8_t cassette_teeth[MAX_CASSETTE_COGS]; // 0 means unused
    uint8_t chainring_teeth[MAX_CHAINRINGS];   // 0 means unused
    
    uint8_t current_cassette_index; // 0-indexed, relates to active gear
    uint8_t current_chainring_index; // 0-indexed, relates to active gear
    
    uint8_t tire_selection_index; // maps to tire_type_t
    uint16_t wheel_circumference_mm; // Calculated from tire selection
    
    uint8_t rim_profile_front; // Just storage for now
    uint8_t rim_profile_rear;  // Just storage for now
    
    float bike_weight_kg;
    
    float brake_min_voltage;
    float brake_max_voltage;
} bike_config_t;

/**
 * @brief Initialize bike config and load from NVS
 */
void bike_config_init(void);

/**
 * @brief Save current configuration to NVS
 */
void bike_config_save(void);

/**
 * @brief Get a pointer to the current configuration (read/write used by UI)
 * NOTE: Call bike_config_save() after modifying if you want persistence.
 */
bike_config_t* bike_config_get(void);

/**
 * @brief Calculate speed in KPH based on current gear and cadence
 */
float bike_config_calculate_speed(float cadence_rpm);

/**
 * @brief Shift cassette up (smaller cog / harder gear)
 * @return true if changed
 */
bool bike_config_shift_cassette_up(void);

/**
 * @brief Shift cassette down (larger cog / easier gear)
 * @return true if changed
 */
bool bike_config_shift_cassette_down(void);

/**
 * @brief Shift chainring up (larger ring / harder gear)
 * @return true if changed
 */
bool bike_config_shift_chainring_up(void);

/**
 * @brief Shift chainring down (smaller ring / easier gear)
 * @return true if changed
 */
bool bike_config_shift_chainring_down(void);

/**
 * @brief Get total number of active cogs in cassette
 */
uint8_t bike_config_get_cassette_count(void);

/**
 * @brief Get total number of active chainrings
 */
uint8_t bike_config_get_chainring_count(void);

/**
 * @brief Set cassette index directly (safe).
 * @return true if changed
 */
bool bike_config_set_cassette_index(uint8_t index);

/**
 * @brief Set chainring index directly (safe).
 * @return true if changed
 */
bool bike_config_set_chainring_index(uint8_t index);


/**
 * @brief Helper to get tire name string
 */
const char* bike_config_get_tire_name(uint8_t index);

#endif // BIKE_CONFIG_H
