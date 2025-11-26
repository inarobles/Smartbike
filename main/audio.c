#include "audio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "Audio";

// TODO: Implementar driver de audio completo sin dependencia del BSP
// Por ahora, el audio está deshabilitado

void audio_play_beep(void)
{
    // Audio deshabilitado temporalmente - pendiente de implementar driver ES8311
    ESP_LOGD(TAG, "Beep solicitado (audio deshabilitado)");
}

esp_err_t audio_init(void)
{
    ESP_LOGW(TAG, "Audio deshabilitado temporalmente - pendiente de implementar driver ES8311 sin BSP");
    // TODO: Implementar inicialización de I2S y ES8311 sin dependencias del BSP
    return ESP_OK;
}
