/*
 * SKETCH V22-Modificado (SIN SENSOR HALL)
 *
 * Arranca el VFD, funciona 20 segundos y se detiene.
 * No hay lectura de pulsos ni RPM.
 *
 * Versión Limpia - Sin errores de citación
 */

#include <ModbusMaster.h>
#include <HardwareSerial.h>

#define MOTOR_RUN_DURATION_MS 20000 
// #define PULSOS_POR_REVOLUCION 12.0 // Eliminado

// --- CONFIGURACIÓN DEBOUNCE ---
// const unsigned long debounceMicrosegundos = 00; // Eliminado

// Configuración de pines
#define VFD_TX_PIN 19
#define VFD_RX_PIN 18
// #define HALL_SENSOR_PIN 15 // Eliminado

// Configuración VFD
#define VFD_ADDRESS 1
#define VFD_BAUDRATE 9600
#define FREQ_REGISTER 0x2001
#define CMD_REGISTER 0x2000
#define CMD_RUN 0x0012   
#define CMD_STOP 0x0001  

ModbusMaster node;

// Variables para el sensor Hall
// volatile unsigned long pulsos = 0; // Eliminado
// volatile unsigned long ultimoTiempoPulso = 0; // Eliminado
unsigned long ultimoTiempo = 0;
// float rpm = 0; // Eliminado

// Variables para el temporizador
unsigned long startTime = 0;
bool motorRunning = false;

// --- INTERRUPCIÓN CON DEBOUNCE ---
// void IRAM_ATTR contarPulso() { ... } // Eliminado

void setup() {
  Serial.begin(115200);
  Serial.printf("\n=== VFD a 30Hz (Test %d seg) [SIN SENSOR] ===\n", 
                MOTOR_RUN_DURATION_MS / 1000);
  
  // Configurar sensor Hall
  // pinMode(HALL_SENSOR_PIN, INPUT_PULLUP); // Eliminado
  // attachInterrupt(digitalPinToInterrupt(HALL_SENSOR_PIN), contarPulso, RISING); // Eliminado
  
  // Configurar comunicación Modbus
  Serial2.begin(VFD_BAUDRATE, SERIAL_8N1, VFD_RX_PIN, VFD_TX_PIN);
  node.begin(VFD_ADDRESS, Serial2);
  
  delay(1000); 
  
  uint16_t valor_frecuencia = 7811; // Mantenido de tu sketch V22
  
  Serial.print("Estableciendo frecuencia (Valor 5000)... "); // Mantenido de tu sketch V22
  uint8_t result = node.writeSingleRegister(FREQ_REGISTER, valor_frecuencia);
  if (result == node.ku8MBSuccess) {
    Serial.println("OK");
  } else {
    Serial.print("Error: 0x");
    Serial.println(result, HEX);
  }
  
  delay(200);
  
  // Arrancar motor
  Serial.print("Arrancando motor (CMD 0x0012)... ");
  result = node.writeSingleRegister(CMD_REGISTER, CMD_RUN);
  if (result == node.ku8MBSuccess) {
    Serial.println("OK");
    Serial.printf("\nMotor funcionando durante %d segundos...\n", MOTOR_RUN_DURATION_MS / 1000);
    
    motorRunning = true; 
    startTime = millis(); 

  } else {
    Serial.print("Error: 0x");
    Serial.println(result, HEX);
    Serial.println("Motor no pudo arrancar. Test abortado.");
  }
  
  ultimoTiempo = millis();
}

void loop() {
  
  if (!motorRunning) {
    return;
  }

  unsigned long tiempoActual = millis();

  // Tarea 1 - Detener el motor después del tiempo definido
  if (tiempoActual - startTime >= MOTOR_RUN_DURATION_MS) {
      motorRunning = false;
      Serial.printf("\n--- %d segundos transcurridos. Deteniendo motor... ---\n", MOTOR_RUN_DURATION_MS / 1000);
      
      uint8_t result = node.writeSingleRegister(CMD_REGISTER, CMD_STOP);
      if (result == node.ku8MBSuccess) { 
          Serial.println("Comando STOP enviado con éxito.");
      } else {
          Serial.println("Error al enviar comando STOP.");
      }
      Serial.println("--- Test finalizado ---");
      return;
  }

  // Tarea 2: Leer VFD
  if (tiempoActual - ultimoTiempo >= 1000) {
    
    // Calcular RPM (Eliminado)
    
    // Mostrar resultados
    // Serial.print("Pulsos/seg: "); // Eliminado
    // Serial.print(pulsosTemp); // Eliminado
    // Serial.print(" | RPM: "); // Eliminado
    // Serial.print(rpm); // Eliminado

    // Leer frecuencia actual del VFD (Registro 0x2103)
    uint8_t result = node.readHoldingRegisters(0x2103, 1);
    if (result == node.ku8MBSuccess) {
      float freq = node.getResponseBuffer(0) / 100.0;
      Serial.print("VFD (Real): ");
      Serial.print(freq);
      Serial.print(" Hz");

      float kmh = freq * (6.4 / 50.0); // Mantenido de tu sketch V22
      Serial.print(" | km/h (calculado): ");
      Serial.print(kmh);
    }
    
    Serial.println();
    
    ultimoTiempo = tiempoActual;
  }
}
