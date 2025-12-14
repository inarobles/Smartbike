#ifndef BLE_CLIENT_H
#define BLE_CLIENT_H

#include <stdint.h>
#include "host/ble_hs.h" // For ble_addr_t and other NimBLE types

// Callback function type to notify the UI about a found device
typedef void (*ble_device_found_callback_t)(const char* name, ble_addr_t addr);

// Service UUIDs
#define BLE_UUID_HEART_RATE      0x180D
#define BLE_UUID_CYCLING_POWER   0x1818

/**
 * @brief Initializes the BLE client for ESP-Hosted mode.
 *        This function sets up the NimBLE host and starts the BLE host task.
 *        It assumes that NVS and the ESP-Hosted transport layer have been initialized beforehand.
 */
void ble_client_init(void);

/**
 * @brief Starts a new BLE scan for devices advertising a specific service.
 *
 * @param cb The callback function to be invoked for each device found.
 * @param service_uuid The 16-bit UUID of the service to scan for (e.g., BLE_UUID_HEART_RATE).
 */
void ble_client_start_scan(ble_device_found_callback_t cb, uint16_t service_uuid);

/**
 * @brief Connects to a specific BLE device using its address.
 *
 * @param addr The address of the device to connect to.
 * @param service_uuid The expected service UUID (to distinguish between HR and Power).
 */
void ble_client_connect(ble_addr_t addr, uint16_t service_uuid);

/**
 * @brief Saves the address of a BLE device to Non-Volatile Storage (NVS).
 *
 * @param addr The address of the device to save.
 * @param service_uuid The service UUID to identify which device type to save.
 */
void ble_client_save_device(ble_addr_t addr, uint16_t service_uuid);

/**
 * @brief Loads a previously saved BLE device address from NVS.
 *
 * @param addr Pointer to a ble_addr_t struct to store the loaded address.
 * @param service_uuid The service UUID to identify which device type to load.
 * @return true if an address was successfully loaded, false otherwise.
 */
bool ble_client_load_saved_device(ble_addr_t *addr, uint16_t service_uuid);

// Helper to get current HR (thread safe)
uint16_t ble_client_get_heart_rate(void);
bool ble_client_is_connected(void); // Checks HR connection

// Helper to get current Power (thread safe)
int16_t ble_client_get_power(void);
bool ble_client_is_power_connected(void);

// Helper to get current Cadence (thread safe)
uint8_t ble_client_get_cadence(void);

#endif // BLE_CLIENT_H
