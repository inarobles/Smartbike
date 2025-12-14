# ES8311 Audio Driver Solution (ESP32-P4)

## Overview
This document details the successful implementation of the audio driver for the **ES8311 Codec** on the **ESP32-P4 Function EV Board**. 
The final solution achieves clean, high-quality audio output (48kHz) by bypassing standard libraries and implementing a manual, hardware-specific driver.

## The Problems Solved
1.  **Driver Conflict**: The `espressif/es8311` component relies on the legacy I2C driver, which conflicts with the BSP's use of the "New" `i2c_master` driver.
2.  **"Open Fail" NACKs**: The `esp_codec_dev` abstraction layer failed to initialize the chip reliably, likely due to strict timing or register check failures.
3.  **Silence / No Tone**: The codec was initially configured to listen to the MCLK pin (which is disconnected), rather than deriving its clock from BCLK.
4.  **Crackling / Noise**: 
    - **Protocol Mismatch**: ESP32 was sending MSB-Justified (Left) data, while the Codec expected I2S (Philips) format. This caused a 1-bit data shift.
    - **Timing Skew**: Digital noise was resolved by **inverting the BCLK phase** on the ESP32 side.

---

## "The Golden Profile" Configuration

### 1. I2S Configuration (ESP32 Side)
Located in `audio_manager.c` -> `init_i2s`.

*   **Sample Rate**: `48000` Hz (High Quality / Standard).
*   **Format**: `I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG` (Philips I2S).
    *   *Why*: Matches the ES8311 default `0x0C` I2S mode.
*   **Clocking**: 
    *   `auto_clear = true`: Ensures BCLK starts generating before data is sent.
    *   **CRITICAL FIX**: `bclk_inv = true`. Inverts the bit clock to correct phase mismatch and eliminate static/crackling.

### 2. Manual Codec Initialization (ES8311 Side)
Located in `audio_manager.c` -> `manual_es8311_init`.
We bypass all libraries and write directly to I2C Address `0x18`.

#### Critical Register Sequence
| Step | Register | Value | Description |
| :--- | :--- | :--- | :--- |
| **0. Reset** | `0x00` | `0x80` | **CSM ON**, Slave Mode. (Crucial: Default `0x1F` disables Clock Manager!) |
| **1. Clock** | `0x01` | `0xBF` | **MCLK Source = BCLK**. (Bit 7=1). |
| | `0x02` | `0x18` | PLL Multiplier. (Input 1.536MHz -> System 12.288MHz). |
| | `0x03` | `0x10` | ADC OSR (16x for 48kHz). |
| | `0x04` | `0x10` | DAC OSR (16x for 48kHz). |
| **2. Format**| `0x09` | `0x0C` | I2S Format, 16-bit. |
| | `0x0A` | `0x0C` | I2S Format, 16-bit. |
| **3. Power** | `0x0D` | `0x01` | Power Up Analog. |
| | `0x12` | `0x00` | Enable DAC. |
| **4. Gain** | `0x17` | `0x80` | ADC Volume (~0dB). Reduced from Max (`0xBF`) to prevent clipping. |
| | `0x32` | `0x80` | DAC Volume (~0dB). Reduced from Max (`0xBF`) to prevent clipping. |

---

## Implementation Details
The driver is self-contained in `main/audio_manager.c`.
- **Dependencies**: Uses `bsp_i2c_get_handle()` to share the I2C bus with the Touch Panel and other peripherals without conflict.
- **Hardware Pins**:
    - **BCLK**: GPIO 12
    - **WS**: GPIO 10
    - **DOUT**: GPIO 9
    - **Amp Enable**: GPIO 20

## Usage
call `audio_manager_init()` at startup.
call `audio_manager_play_beep()` to test.
