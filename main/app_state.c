#include "app_state.h"
#include "esp_log.h"
#include "slope_simulator.h"

static const char *TAG = "APP_STATE";
static bool s_training_active = false;

void app_state_set_training_mode(bool active) {
    ESP_LOGI(TAG, "Training Mode: %s", active ? "ACTIVE" : "INACTIVE");
    if (active && !s_training_active) {
        slope_simulator_reset();
    }
    s_training_active = active;
}

bool app_state_is_training_active(void) {
    return s_training_active;
}
