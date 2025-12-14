/*
 * BLE Client for Heart Rate Monitor and Power Meter using NimBLE on ESP-Hosted Architecture
 * Adapted for SmartbikeP4
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdbool.h>

// Core NimBLE includes
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

// Key include for the hosted architecture
#include "esp_hosted.h"

#include "ble_client.h"

static const char *TAG = "BLE_CLIENT";

// NVS constants for storing the BLE device address
#define NVS_NAMESPACE "ble_client"
#define NVS_KEY_SAVED_ADDR_HR "saved_addr_hr"
#define NVS_KEY_SAVED_ADDR_PWR "saved_addr_pwr"

// Heart Rate Service and Characteristic UUIDs
static const ble_uuid16_t g_svc_heart_rate_uuid = BLE_UUID16_INIT(BLE_UUID_HEART_RATE);
static const ble_uuid16_t g_chr_heart_rate_meas_uuid = BLE_UUID16_INIT(0x2A37);

// Cycling Power Service and Characteristic UUIDs
static const ble_uuid16_t g_svc_cycling_power_uuid = BLE_UUID16_INIT(BLE_UUID_CYCLING_POWER);
static const ble_uuid16_t g_chr_cycling_power_meas_uuid = BLE_UUID16_INIT(0x2A63);

// Connection Handles
static uint16_t g_hr_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t g_power_conn_handle = BLE_HS_CONN_HANDLE_NONE;

// Characteristic Value Handles
static uint16_t g_hr_chr_val_handle = 0;
static uint16_t g_power_chr_val_handle = 0;

static uint8_t g_own_addr_type;

// --- Globals for scanning and device list ---
#define MAX_DISCOVERED_DEVICES 20
static ble_addr_t g_discovered_devices[MAX_DISCOVERED_DEVICES];
static int g_discovered_device_count = 0;
static ble_device_found_callback_t g_device_found_cb = NULL;
static bool g_is_scanning = false;
static uint16_t g_scan_service_uuid = 0; // Service UUID to filter scan results

static bool g_user_initiated_disconnect = false;  // Flag to prevent auto-reconnect after manual scan
static TaskHandle_t g_reconnect_task_handle = NULL;
static SemaphoreHandle_t g_ble_state_mutex = NULL;  // Protects state and data

// --- Local State ---
static uint16_t s_heart_rate = 0;
static bool s_hr_connected = false;

static int16_t s_power_watts = 0;
static bool s_power_connected = false;

// Cadence state
static uint8_t s_cadence = 0;
static uint16_t s_prev_crank_revs = 0;
static uint16_t s_prev_crank_time = 0;
static bool s_first_crank_meas = true;

// Forward declarations
static void ble_reconnect_task(void *pvParameters);
static void ble_client_scan_internal(void);
static int ble_client_gap_event(struct ble_gap_event *event, void *arg);
static void ble_client_on_sync(void);
static void ble_client_on_reset(int reason);
static void ble_host_task(void *param);
static int ble_client_on_dsc_disc(uint16_t conn_handle, const struct ble_gatt_error *error, uint16_t chr_val_handle, const struct ble_gatt_dsc *dsc, void *arg);
static int ble_client_on_char_disc(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_chr *chr, void *arg);
static int ble_client_on_service_disc(uint16_t conn_handle, const struct ble_gatt_error *error, const struct ble_gatt_svc *service, void *arg);

// --- PUBLIC FUNCTIONS ---

uint16_t ble_client_get_heart_rate(void) {
    uint16_t hr = 0;
    if (g_ble_state_mutex && xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        hr = s_heart_rate;
        xSemaphoreGive(g_ble_state_mutex);
    }
    return hr;
}

bool ble_client_is_connected(void) {
    bool connected = false;
    if (g_ble_state_mutex && xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        connected = s_hr_connected;
        xSemaphoreGive(g_ble_state_mutex);
    }
    return connected;
}

int16_t ble_client_get_power(void) {
    int16_t pwr = 0;
    if (g_ble_state_mutex && xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        pwr = s_power_watts;
        xSemaphoreGive(g_ble_state_mutex);
    }
    return pwr;
}

bool ble_client_is_power_connected(void) {
    bool connected = false;
    if (g_ble_state_mutex && xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        connected = s_power_connected;
        xSemaphoreGive(g_ble_state_mutex);
    }
    return connected;
}

uint8_t ble_client_get_cadence(void) {
    uint8_t cad = 0;
    if (g_ble_state_mutex && xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        cad = s_cadence;
        xSemaphoreGive(g_ble_state_mutex);
    }
    return cad;
}

void ble_client_start_scan(ble_device_found_callback_t cb, uint16_t service_uuid) {
    bool is_scanning = false;
    if (g_ble_state_mutex && xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        is_scanning = g_is_scanning;
        xSemaphoreGive(g_ble_state_mutex);
    }

    if (is_scanning) {
        ESP_LOGW(TAG, "Scan already in progress.");
        return;
    }

    // If scanning for HR, disconnect existing HR. If scanning for Power, disconnect existing Power.
    uint16_t conn_to_terminate = BLE_HS_CONN_HANDLE_NONE;
    if (service_uuid == BLE_UUID_HEART_RATE && g_hr_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        conn_to_terminate = g_hr_conn_handle;
    } else if (service_uuid == BLE_UUID_CYCLING_POWER && g_power_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        conn_to_terminate = g_power_conn_handle;
    }

    if (conn_to_terminate != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "Disconnecting from current device (Handle %d) to start scan...", conn_to_terminate);
        if (g_ble_state_mutex && xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            g_user_initiated_disconnect = true;  // Mark as user-initiated
            xSemaphoreGive(g_ble_state_mutex);
        }
        ble_gap_terminate(conn_to_terminate, BLE_ERR_REM_USER_CONN_TERM);
        
        // Reset local state immediately
        if (xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (conn_to_terminate == g_hr_conn_handle) {
                g_hr_conn_handle = BLE_HS_CONN_HANDLE_NONE;
                s_hr_connected = false;
                s_heart_rate = 0;
            } else {
                g_power_conn_handle = BLE_HS_CONN_HANDLE_NONE;
                s_power_connected = false;
                s_power_watts = 0;
            }
            xSemaphoreGive(g_ble_state_mutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI(TAG, "Starting new scan for service UUID: 0x%04x", service_uuid);
    g_device_found_cb = cb;
    g_scan_service_uuid = service_uuid;
    g_discovered_device_count = 0;
    memset(g_discovered_devices, 0, sizeof(g_discovered_devices));
    ble_client_scan_internal();
}

void ble_client_connect(ble_addr_t addr, uint16_t service_uuid) {
    // Check if already connected to a device of this type
    if (service_uuid == BLE_UUID_HEART_RATE && g_hr_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGW(TAG, "Already connected to HR device.");
        return;
    }
    if (service_uuid == BLE_UUID_CYCLING_POWER && g_power_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGW(TAG, "Already connected to Power device.");
        return;
    }

    // Stop scanning before connecting
    bool was_scanning = false;
    if (g_ble_state_mutex && xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        was_scanning = g_is_scanning;
        xSemaphoreGive(g_ble_state_mutex);
    }

    if (was_scanning) {
        ble_gap_disc_cancel();
        if (g_ble_state_mutex && xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            g_is_scanning = false;
            xSemaphoreGive(g_ble_state_mutex);
        }
    }

    ESP_LOGI(TAG, "Attempting to connect to device with address: %02x:%02x:%02x:%02x:%02x:%02x",
             addr.val[5], addr.val[4], addr.val[3], addr.val[2], addr.val[1], addr.val[0]);

    // Reset user disconnect flag
    if (g_ble_state_mutex && xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_user_initiated_disconnect = false;
        xSemaphoreGive(g_ble_state_mutex);
    }

    // We pass the service UUID as the argument to the callback context so we know what we are connecting to
    // However, ble_gap_connect takes a void* arg which is passed to the event handler.
    // BUT, the event handler is global. We need a way to know which service we expect.
    // For simplicity, we'll rely on service discovery to assign the handle to the correct global variable.
    // OR, we can temporarily store the "connecting service" in a global if we assume sequential user actions.
    // Let's use service discovery to be robust.

    int rc = ble_gap_connect(g_own_addr_type, &addr, 30000, NULL, ble_client_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to initiate connect; rc=%d. Restarting scan.", rc);
        ble_client_scan_internal();
    }
}

void ble_client_save_device(ble_addr_t addr, uint16_t service_uuid) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return;
    }

    const char* key = (service_uuid == BLE_UUID_HEART_RATE) ? NVS_KEY_SAVED_ADDR_HR : NVS_KEY_SAVED_ADDR_PWR;
    err = nvs_set_blob(nvs_handle, key, &addr, sizeof(ble_addr_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write address to NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Saved device address to NVS (Key: %s).", key);
        nvs_commit(nvs_handle);
    }
    nvs_close(nvs_handle);
}

bool ble_client_load_saved_device(ble_addr_t *addr, uint16_t service_uuid) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        return false;
    }

    const char* key = (service_uuid == BLE_UUID_HEART_RATE) ? NVS_KEY_SAVED_ADDR_HR : NVS_KEY_SAVED_ADDR_PWR;
    size_t required_size = sizeof(ble_addr_t);
    err = nvs_get_blob(nvs_handle, key, addr, &required_size);
    nvs_close(nvs_handle);

    if (err != ESP_OK || required_size != sizeof(ble_addr_t)) {
        return false;
    }
    
    ESP_LOGI(TAG, "Successfully loaded saved device address from NVS (Key: %s).", key);
    return true;
}


// --- INTERNAL FUNCTIONS ---

static int ble_client_gap_event(struct ble_gap_event *event, void *arg) {
    struct ble_hs_adv_fields fields;
    int rc;

    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
        if (rc != 0) return 0;

        bool service_found = false;
        ble_uuid16_t target_uuid;
        target_uuid.u.type = BLE_UUID_TYPE_16;
        target_uuid.value = g_scan_service_uuid;

        for (int i = 0; i < fields.num_uuids16; i++) {
            if (ble_uuid_cmp(&fields.uuids16[i].u, &target_uuid.u) == 0) {
                service_found = true;
                break;
            }
        }

        if (service_found) {
            for (int i = 0; i < g_discovered_device_count; i++) {
                if (ble_addr_cmp(&event->disc.addr, &g_discovered_devices[i]) == 0) {
                    return 0; 
                }
            }

            if (g_discovered_device_count < MAX_DISCOVERED_DEVICES) {
                g_discovered_devices[g_discovered_device_count++] = event->disc.addr;

                char dev_name[30] = {0};
                if (fields.name != NULL && fields.name_len > 0) {
                    int name_len = fields.name_len > sizeof(dev_name) - 1 ? sizeof(dev_name) - 1 : fields.name_len;
                    memcpy(dev_name, fields.name, name_len);
                } else {
                    snprintf(dev_name, sizeof(dev_name), "DEV-%02x%02x", event->disc.addr.val[1], event->disc.addr.val[0]);
                }

                ESP_LOGI(TAG, "Found device: %s", dev_name);

                if (g_device_found_cb) {
                    g_device_found_cb(dev_name, event->disc.addr);
                }
            }
        }
        return 0;

    case BLE_GAP_EVENT_CONNECT:
        g_is_scanning = false;
        if (event->connect.status == 0) {
            ESP_LOGI(TAG, "Connection established; conn_handle=%d", event->connect.conn_handle);
            
            // We don't know if it's HR or Power yet. Service discovery will tell us.
            rc = ble_gattc_disc_all_svcs(event->connect.conn_handle, ble_client_on_service_disc, NULL);
            if (rc != 0) {
                ESP_LOGE(TAG, "Failed to discover services; rc=%d", rc);
            }
        } else {
            ESP_LOGE(TAG, "Connection attempt failed; status=%d.", event->connect.status);
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "Disconnected; reason=%d, handle=%d", event->disconnect.reason, event->disconnect.conn.conn_handle);
        
        if (xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (event->disconnect.conn.conn_handle == g_hr_conn_handle) {
                g_hr_conn_handle = BLE_HS_CONN_HANDLE_NONE;
                s_hr_connected = false;
                s_heart_rate = 0;
            } else if (event->disconnect.conn.conn_handle == g_power_conn_handle) {
                g_power_conn_handle = BLE_HS_CONN_HANDLE_NONE;
                s_power_connected = false;
                s_power_watts = 0;
            }
            xSemaphoreGive(g_ble_state_mutex);
        }

        // Auto-reconnect logic could be complex with two devices. 
        // For now, let the reconnect task handle it periodically.
        return 0;

    case BLE_GAP_EVENT_DISC_COMPLETE:
        g_is_scanning = false;
        ESP_LOGI(TAG, "Scan complete; reason=%d", event->disc_complete.reason);
        return 0;

    case BLE_GAP_EVENT_NOTIFY_RX:
        if (event->notify_rx.attr_handle == g_hr_chr_val_handle) {
            uint8_t *data = event->notify_rx.om->om_data;
            uint16_t len = event->notify_rx.om->om_len;
            if (len >= 2) {
                uint8_t flags = data[0];
                uint16_t bpm = (flags & 0x01) ? ((data[2] << 8) | data[1]) : data[1];
                if (bpm > 30 && bpm < 250) { 
                    if (xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                        s_heart_rate = bpm;
                        xSemaphoreGive(g_ble_state_mutex);
                    }
                }
            }
        } else if (event->notify_rx.attr_handle == g_power_chr_val_handle) {
            uint8_t *data = event->notify_rx.om->om_data;
            uint16_t len = event->notify_rx.om->om_len;
            if (len >= 4) {
                // Cycling Power Measurement Characteristic (0x2A63)
                // Flags (16 bits) + Instantaneous Power (16 bits)
                int16_t watts = (data[3] << 8) | data[2];
                uint16_t flags = (data[1] << 8) | data[0];
                int offset = 4;
                
                if (flags & 0x01) { // Pedal Power Balance
                    offset += 1;
                }
                if (flags & 0x04) { // Accumulated Torque
                    offset += 2;
                }
                if (flags & 0x10) { // Wheel Revolution Data
                    offset += 6;
                }
                
                uint8_t current_cadence = 0;
                
                if (flags & 0x20) { // Crank Revolution Data
                    if (len >= offset + 4) {
                        uint16_t crank_revs = (data[offset+1] << 8) | data[offset];
                        uint16_t crank_time = (data[offset+3] << 8) | data[offset+2];
                        
                        if (!s_first_crank_meas) {
                            uint16_t rev_diff = crank_revs - s_prev_crank_revs;
                            uint16_t time_diff = crank_time - s_prev_crank_time;
                            
                            if (time_diff > 0) {
                                // time unit is 1/1024 seconds
                                // RPM = (rev_diff * 1024 * 60) / time_diff
                                uint32_t val = (uint32_t)rev_diff * 1024 * 60;
                                current_cadence = (uint8_t)(val / time_diff);
                            }
                        } else {
                            s_first_crank_meas = false;
                        }
                        
                        s_prev_crank_revs = crank_revs;
                        s_prev_crank_time = crank_time;
                    }
                }
                
                if (xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    s_power_watts = watts;
                    if (current_cadence > 0) s_cadence = current_cadence;
                    xSemaphoreGive(g_ble_state_mutex);
                }
                ESP_LOGD(TAG, "Power: %d W, Cadence: %d", watts, s_cadence);
            }
        }
        return 0;

    default:
        return 0;
    }
}

static int ble_client_on_service_disc(uint16_t conn_handle, const struct ble_gatt_error *error,
                                    const struct ble_gatt_svc *service, void *arg) {
    if (error->status == 0 && service != NULL) {
        if (ble_uuid_cmp(&service->uuid.u, &g_svc_heart_rate_uuid.u) == 0) {
            ESP_LOGI(TAG, "Found Heart Rate Service. Handle: %d", conn_handle);
            g_hr_conn_handle = conn_handle;
            
            if (xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                s_hr_connected = true;
                xSemaphoreGive(g_ble_state_mutex);
            }
            
            struct ble_gap_conn_desc desc;
            if (ble_gap_conn_find(conn_handle, &desc) == 0) {
                ble_client_save_device(desc.peer_id_addr, BLE_UUID_HEART_RATE);
            }

            ble_gattc_disc_all_chrs(conn_handle, service->start_handle, service->end_handle, ble_client_on_char_disc, NULL);
        } 
        else if (ble_uuid_cmp(&service->uuid.u, &g_svc_cycling_power_uuid.u) == 0) {
            ESP_LOGI(TAG, "Found Cycling Power Service. Handle: %d", conn_handle);
            g_power_conn_handle = conn_handle;

            if (xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                s_power_connected = true;
                xSemaphoreGive(g_ble_state_mutex);
            }

            struct ble_gap_conn_desc desc;
            if (ble_gap_conn_find(conn_handle, &desc) == 0) {
                ble_client_save_device(desc.peer_id_addr, BLE_UUID_CYCLING_POWER);
            }

            ble_gattc_disc_all_chrs(conn_handle, service->start_handle, service->end_handle, ble_client_on_char_disc, NULL);
        }
    }
    return 0;
}

static int ble_client_on_char_disc(uint16_t conn_handle, const struct ble_gatt_error *error,
                                 const struct ble_gatt_chr *chr, void *arg) {
    if (error->status == 0 && chr != NULL) {
        if (ble_uuid_cmp(&chr->uuid.u, &g_chr_heart_rate_meas_uuid.u) == 0) {
            ESP_LOGI(TAG, "Found Heart Rate Measurement characteristic.");
            g_hr_chr_val_handle = chr->val_handle;
            ble_gattc_disc_all_dscs(conn_handle, chr->val_handle, 0xffff, ble_client_on_dsc_disc, NULL);
        }
        else if (ble_uuid_cmp(&chr->uuid.u, &g_chr_cycling_power_meas_uuid.u) == 0) {
            ESP_LOGI(TAG, "Found Cycling Power Measurement characteristic.");
            g_power_chr_val_handle = chr->val_handle;
            ble_gattc_disc_all_dscs(conn_handle, chr->val_handle, 0xffff, ble_client_on_dsc_disc, NULL);
        }
    }
    return 0;
}

static int ble_client_on_dsc_disc(uint16_t conn_handle, const struct ble_gatt_error *error,
                                uint16_t chr_val_handle, const struct ble_gatt_dsc *dsc, void *arg) {
    if (error->status == 0 && dsc != NULL) {
        if (dsc->uuid.u.type == BLE_UUID_TYPE_16 && ble_uuid_u16(&dsc->uuid.u) == BLE_GATT_DSC_CLT_CFG_UUID16) {
            ESP_LOGI(TAG, "Found CCCD. Enabling notifications.");
            uint8_t value[2] = {0x01, 0x00};
            ble_gattc_write_flat(conn_handle, dsc->handle, value, sizeof(value), NULL, NULL);
        }
    }
    return 0;
}

static void ble_reconnect_task(void *pvParameters) {
    ble_addr_t saved_addr_hr;
    ble_addr_t saved_addr_pwr;
    bool has_saved_hr = false;
    bool has_saved_pwr = false;

    ESP_LOGI(TAG, "Reconnect task started");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));

        bool should_skip = false;
        if (g_ble_state_mutex && xSemaphoreTake(g_ble_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            should_skip = g_is_scanning || g_user_initiated_disconnect;
            xSemaphoreGive(g_ble_state_mutex);
        }
        if (should_skip) continue;

        // Try to reconnect HR
        if (g_hr_conn_handle == BLE_HS_CONN_HANDLE_NONE) {
            if (!has_saved_hr) {
                has_saved_hr = ble_client_load_saved_device(&saved_addr_hr, BLE_UUID_HEART_RATE);
            }
            if (has_saved_hr) {
                ESP_LOGI(TAG, "Attempting reconnect to saved HR device...");
                ble_client_connect(saved_addr_hr, BLE_UUID_HEART_RATE);
                vTaskDelay(pdMS_TO_TICKS(2000)); // Wait a bit between attempts
            }
        }

        // Try to reconnect Power
        if (g_power_conn_handle == BLE_HS_CONN_HANDLE_NONE) {
            if (!has_saved_pwr) {
                has_saved_pwr = ble_client_load_saved_device(&saved_addr_pwr, BLE_UUID_CYCLING_POWER);
            }
            if (has_saved_pwr) {
                ESP_LOGI(TAG, "Attempting reconnect to saved Power device...");
                ble_client_connect(saved_addr_pwr, BLE_UUID_CYCLING_POWER);
            }
        }
    }
}

static void ble_client_on_sync(void) {
    int rc;
    ESP_LOGI(TAG, "BLE Host synced.");

    rc = ble_hs_id_infer_auto(0, &g_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error determining address type; rc=%d", rc);
        return;
    }

    if (g_reconnect_task_handle == NULL) {
        xTaskCreate(ble_reconnect_task, "ble_reconnect", 4096, NULL, 5, &g_reconnect_task_handle);
        ESP_LOGI(TAG, "BLE reconnect task started");
    }
}

static void ble_client_on_reset(int reason)
{
    ESP_LOGE(TAG, "Resetting state; reason=%d", reason);
}

static void ble_client_scan_internal(void) {
    struct ble_gap_disc_params disc_params;
    disc_params.filter_duplicates = 1;
    disc_params.passive = 0;
    disc_params.itvl = 0;
    disc_params.window = 0;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    int rc = ble_gap_disc(g_own_addr_type, 10000, &disc_params, ble_client_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error starting scan; rc=%d", rc);
        g_is_scanning = false;
    } else {
        g_is_scanning = true;
        ESP_LOGI(TAG, "Starting BLE scan...");
    }
}

void ble_host_task(void *param) {
    ESP_LOGI(TAG, "BLE Host Task Started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void ble_client_init(void) {
    ESP_LOGI(TAG, "Initializing BLE Client for ESP-Hosted...");

    if (g_ble_state_mutex == NULL) {
        g_ble_state_mutex = xSemaphoreCreateMutex();
        if (g_ble_state_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create BLE state mutex");
            return;
        }
    }

    nimble_port_init();

    ble_hs_cfg.sync_cb = ble_client_on_sync;
    ble_hs_cfg.reset_cb = ble_client_on_reset;
    
    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
    ble_hs_cfg.sm_sc = 0;

    const char *device_name = "smartbike-p4";
    ble_svc_gap_device_name_set(device_name);

    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(TAG, "BLE Client initialization complete.");
}
