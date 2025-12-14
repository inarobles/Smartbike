#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

// Initialize the entire audio pipeline (I2C, Codec, I2S, Amp)
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

// Initialize the entire audio pipeline (I2C, Codec, I2S, Amp)
esp_err_t audio_manager_init(void);

// Play a raw buffer
void audio_manager_play(const void *data, size_t size);

typedef enum {
    AUDIO_EVENT_STARTUP,
    AUDIO_EVENT_BUTTON,
    AUDIO_EVENT_COUNTDOWN_STEP, // 3, 2, 1
    AUDIO_EVENT_COUNTDOWN_GO    // GO!
} audio_event_t;

void audio_manager_play_event(audio_event_t event);

// Play a test beep sound (blocking or non-blocking? Let's make it simple/blocking for testing or spawn a task)
// For now, a simple test function.
void audio_manager_play_beep(void); // Keeping for backward compatibility if needed

// Set volume (0-255)
void audio_manager_set_volume(uint8_t volume);

// Get current volume (0-255)
uint8_t audio_manager_get_volume(void);

// Save current volume to NVS
void audio_manager_save_volume(void);
