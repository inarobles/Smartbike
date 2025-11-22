# Smartbike - Firmware para Ciclocomputador ESP32-S3

Este proyecto implementa el firmware para un ciclocomputador inteligente basado en el microcontrolador **ESP32-S3** y la biblioteca gráfica **LVGL**.

## Funcionalidades Actuales

*   **Interfaz Gráfica (GUI)**: Basada en LVGL 8.x.
*   **Menú Principal**: Navegación con 5 opciones:
    *   Entrenamiento libre
    *   Cargar entrenamiento
    *   WIFI
    *   BLE
    *   Calibrar
*   **Drivers de Pantalla**: Soporte para pantalla LCD RGB ST7701 (480x800) con prevención de "tearing".
*   **Pantalla Táctil**: Soporte para controlador GT911.
*   **Orientación**: Configurado para modo horizontal (0º) para máximo rendimiento.

## Hardware Requerido

*   **Placa de Desarrollo**: ESP32-S3R8 (con PSRAM Octal).
*   **Pantalla**: Panel LCD RGB de 4 pulgadas (ST7701) con interfaz RGB de 16 bits.
*   **Touch**: Panel táctil capacitivo GT911.

## Configuración de Pines (Pinout)

### Interfaz RGB (LCD)

| Señal | GPIO | Señal | GPIO |
| :--- | :--- | :--- | :--- |
| **PCLK** | 7 | **DE** | 5 |
| **VSYNC** | 3 | **HSYNC** | 46 |
| **D0** | 14 | **D1** | 38 |
| **D2** | 18 | **D3** | 17 |
| **D4** | 10 | **D5** | 39 |
| **D6** | 0 | **D7** | 45 |
| **D8** | 48 | **D9** | 47 |
| **D10** | 21 | **D11** | 1 |
| **D12** | 2 | **D13** | 42 |
| **D14** | 41 | **D15** | 40 |

### Periféricos (I2C / Control)

| Señal | GPIO | Descripción |
| :--- | :--- | :--- |
| **SDA** | 8 | I2C Data (Touch & Backlight) |
| **SCL** | 9 | I2C Clock (Touch & Backlight) |
| **RST** | -1 | Reset (No conectado/Software) |
| **INT** | -1 | Interrupción (No conectado) |

## Compilación y Flasheo

Requisitos: ESP-IDF v5.5 o superior.

1.  **Configurar el proyecto**:
    ```bash
    idf.py reconfigure
    ```
    *Asegúrate de que la rotación esté configurada correctamente en `sdkconfig`.*

2.  **Compilar y Grabar**:
    ```bash
    idf.py build flash monitor
    ```
