/*****************************************************************
 * File:      main.cpp
 * Category:  example
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Modular example demonstrating sensor-only functionality.
 *    Includes HAL, algorithms, and sensor drivers but excludes
 *    display drivers for optimized footprint.
 *****************************************************************/

// Modular includes - only what we need
#include <arcos_algorithms.hpp>        // Sensor fusion algorithms
#include <arcos_drivers_sensors.hpp>   // Sensor drivers (includes core HAL)

using namespace arcos;

// Demonstration variables
unsigned long loop_counter = 0;

// Platform-agnostic delay function
void delayMs(unsigned long ms){
  #ifdef TARGET_ESP32_Wroom32S3_Module
    vTaskDelay(ms / portTICK_PERIOD_MS);
  #elif defined(ARDUINO)
    delay(ms);
  #else
    // Simple delay simulation for other platforms
    for(volatile unsigned long i = 0; i < ms * 1000; i++);
  #endif
}

// Platform-agnostic logging (simplified)
void logMessage(const char* level, const char* tag, const char* message){
  #ifdef TARGET_ESP32_Wroom32S3_Module
    printf("[%s] %s: %s\n", level, tag, message);
  #elif defined(ARDUINO)
    Serial.print("["); Serial.print(level); Serial.print("] ");
    Serial.print(tag); Serial.print(": "); Serial.println(message);
  #else
    printf("[%s] %s: %s\n", level, tag, message);
  #endif
}

void demonstrateModularInclusion(){
  logMessage("INFO", "SENSORS_ONLY", "=== Modular Inclusion Demo ===");
  logMessage("INFO", "SENSORS_ONLY", "Included modules:");
  logMessage("INFO", "SENSORS_ONLY", "✓ arcos_algorithms.hpp - Sensor fusion & math");
  logMessage("INFO", "SENSORS_ONLY", "✓ arcos_drivers_sensors.hpp - Sensor drivers");
  logMessage("INFO", "SENSORS_ONLY", "  └─ Auto-includes arcos_core.hpp (HAL)");
  
  logMessage("INFO", "SENSORS_ONLY", "Excluded modules:");
  logMessage("INFO", "SENSORS_ONLY", "✗ arcos_drivers_display.hpp - Display drivers");
  logMessage("INFO", "SENSORS_ONLY", "✗ Storage/communication drivers");
  
  logMessage("INFO", "SENSORS_ONLY", "Benefits:");
  logMessage("INFO", "SENSORS_ONLY", "• Optimized for sensor applications");
  logMessage("INFO", "SENSORS_ONLY", "• Smaller footprint than complete library");
  logMessage("INFO", "SENSORS_ONLY", "• All sensor fusion capabilities available");
}

void demonstrateNamespaceAccess(){
  logMessage("INFO", "SENSORS_ONLY", "=== Available Namespaces ===");
  logMessage("INFO", "SENSORS_ONLY", "arcos::core - Core HAL functionality");
  logMessage("INFO", "SENSORS_ONLY", "arcos::algorithms::fusion - IMU & sensor fusion");
  logMessage("INFO", "SENSORS_ONLY", "arcos::algorithms::orientation - Quaternions, Euler");
  logMessage("INFO", "SENSORS_ONLY", "arcos::drivers::sensors - ICM20948, BME280, etc.");
  
  logMessage("INFO", "SENSORS_ONLY", "Not available (not included):");
  logMessage("INFO", "SENSORS_ONLY", "arcos::drivers::display - HUB75, OLED drivers");
}

void simulateSensorProcessing(){
  // Simulate sensor data processing
  logMessage("INFO", "SENSORS_ONLY", "Simulating sensor data processing...");
  
  // Show algorithm capabilities
  logMessage("INFO", "SENSORS_ONLY", "✓ IMU fusion algorithms available");
  logMessage("INFO", "SENSORS_ONLY", "✓ Quaternion mathematics available");
  logMessage("INFO", "SENSORS_ONLY", "✓ Euler angle conversions available");
  logMessage("INFO", "SENSORS_ONLY", "✓ Gravity vector calculations available");
  
  // Show sensor driver capabilities
  logMessage("INFO", "SENSORS_ONLY", "✓ ICM20948 IMU driver available");
  logMessage("INFO", "SENSORS_ONLY", "✓ BME280 environmental driver available");
  logMessage("INFO", "SENSORS_ONLY", "✓ I2C/SPI communication protocols available");
}

void setup(){
  // Initialize serial/logging for different platforms
  #ifdef ARDUINO
    Serial.begin(115200);
    while(!Serial) delayMs(10);
  #endif
  
  logMessage("INFO", "SENSORS_ONLY", "Starting ARCOS Sensors-Only Example");
  logMessage("INFO", "SENSORS_ONLY", "Demonstrating modular library inclusion");
  
  // Show what's included
  demonstrateModularInclusion();
  demonstrateNamespaceAccess();
  
  logMessage("INFO", "SENSORS_ONLY", "=== Platform Detection ===");
  #ifdef TARGET_ESP32_Wroom32S3_Module
    logMessage("INFO", "SENSORS_ONLY", "Platform: ESP32-S3");
  #elif defined(TARGET_ESP32_Wroom32S2_Esp32Dev)
    logMessage("INFO", "SENSORS_ONLY", "Platform: ESP32-S2");
  #elif defined(TARGET_AVR_Atmega328p_Uno)
    logMessage("INFO", "SENSORS_ONLY", "Platform: Arduino Uno");
  #elif defined(ARDUINO)
    logMessage("INFO", "SENSORS_ONLY", "Platform: Arduino");
  #else
    logMessage("INFO", "SENSORS_ONLY", "Platform: Custom/Generic");
  #endif
  
  logMessage("INFO", "SENSORS_ONLY", "=== Simulation Starting ===");
}

void loop(){
  // Simulate sensor operations
  simulateSensorProcessing();
  
  // Show optimization benefits every 10 loops
  if(loop_counter % 10 == 0){
    logMessage("INFO", "SENSORS_ONLY", "=== Optimization Benefits ===");
    logMessage("INFO", "SENSORS_ONLY", "Memory saved by excluding display drivers");
    logMessage("INFO", "SENSORS_ONLY", "Faster compilation with modular inclusion");
    logMessage("INFO", "SENSORS_ONLY", "Perfect for IoT sensor nodes");
  }
  
  // Show alternative inclusion methods every 20 loops
  if(loop_counter % 20 == 0){
    logMessage("INFO", "SENSORS_ONLY", "=== Alternative Approaches ===");
    logMessage("INFO", "SENSORS_ONLY", "For complete library: #include <arcos.hpp>");
    logMessage("INFO", "SENSORS_ONLY", "For HAL only: #include <arcos_core.hpp>");
    logMessage("INFO", "SENSORS_ONLY", "For algorithms only: #include <arcos_algorithms.hpp>");
    logMessage("INFO", "SENSORS_ONLY", "Current: algorithms + sensor drivers");
  }
  
  loop_counter++;
  
  // Wait 1 second
  delayMs(1000);
}

/*
 * Platform-agnostic main entry point
 */
int main(){
  setup();
  
  while(true){
    loop();
  }
  
  return 0;
}

// ESP-IDF specific entry point
#ifdef TARGET_ESP32_Wroom32S3_Module
extern "C" void app_main(){
  main();
}
#endif