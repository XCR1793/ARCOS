/*****************************************************************
 * File:      main.cpp
 * Category:  example
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Platform-agnostic example demonstrating the complete ARCOS
 *    library inclusion and namespace structure for sensor fusion
 *    and quaternion mathematics concepts.
 *****************************************************************/

#include <arcos.hpp>

// Use ARCOS namespaces to demonstrate modular access
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

void demonstrateNamespaceStructure(){
  logMessage("INFO", "IMU_FUSION", "=== ARCOS Namespace Structure ===");
  logMessage("INFO", "IMU_FUSION", "arcos::core - Core HAL functionality");
  logMessage("INFO", "IMU_FUSION", "arcos::algorithms - Sensor fusion & math");
  logMessage("INFO", "IMU_FUSION", "  └─ arcos::algorithms::fusion");
  logMessage("INFO", "IMU_FUSION", "  └─ arcos::algorithms::orientation");
  logMessage("INFO", "IMU_FUSION", "arcos::drivers - Hardware drivers");
  logMessage("INFO", "IMU_FUSION", "  └─ arcos::drivers::sensors");
  logMessage("INFO", "IMU_FUSION", "  └─ arcos::drivers::display");
}

void demonstrateModularInclusion(){
  logMessage("INFO", "IMU_FUSION", "=== Modular Inclusion Demo ===");
  logMessage("INFO", "IMU_FUSION", "This example uses: #include <arcos.hpp>");
  logMessage("INFO", "IMU_FUSION", "Alternative modular approaches:");
  logMessage("INFO", "IMU_FUSION", "  arcos_core.hpp - HAL only");
  logMessage("INFO", "IMU_FUSION", "  arcos_algorithms.hpp - Math/fusion only");
  logMessage("INFO", "IMU_FUSION", "  arcos_drivers_sensors.hpp - Sensor drivers only");
}

void demonstratePlatformAgnostic(){
  logMessage("INFO", "IMU_FUSION", "=== Platform Agnostic Design ===");
  
  #ifdef TARGET_ESP32_Wroom32S3_Module
    logMessage("INFO", "IMU_FUSION", "Running on: ESP32-S3 (ESP-IDF)");
  #elif defined(TARGET_ESP32_Wroom32S2_Esp32Dev)
    logMessage("INFO", "IMU_FUSION", "Running on: ESP32-S2 (ESP-IDF)");
  #elif defined(TARGET_AVR_Atmega328p_Uno)
    logMessage("INFO", "IMU_FUSION", "Running on: Arduino Uno");
  #elif defined(ARDUINO)
    logMessage("INFO", "IMU_FUSION", "Running on: Arduino Platform");
  #else
    logMessage("INFO", "IMU_FUSION", "Running on: Custom Platform");
  #endif
  
  logMessage("INFO", "IMU_FUSION", "Same code works across all platforms!");
}

void setup(){
  // Initialize serial/logging for different platforms
  #ifdef ARDUINO
    Serial.begin(115200);
    while(!Serial) delayMs(10);
  #endif
  
  logMessage("INFO", "IMU_FUSION", "Starting ARCOS IMU Fusion Demo");
  logMessage("INFO", "IMU_FUSION", "This is a concept demonstration example");
  
  // Demonstrate the modular structure
  demonstrateNamespaceStructure();
  demonstrateModularInclusion();
  demonstratePlatformAgnostic();
  
  logMessage("INFO", "IMU_FUSION", "=== Simulation Starting ===");
  logMessage("INFO", "IMU_FUSION", "This example simulates sensor fusion concepts");
}

void loop(){
  // Simulate sensor fusion demonstration
  if(loop_counter % 10 == 0){
    logMessage("INFO", "IMU_FUSION", "Simulating IMU sensor reading...");
    
    // Show that we have access to different ARCOS modules
    logMessage("INFO", "IMU_FUSION", "✓ ARCOS Core: Platform HAL available");
    logMessage("INFO", "IMU_FUSION", "✓ ARCOS Algorithms: Sensor fusion ready");
    logMessage("INFO", "IMU_FUSION", "✓ ARCOS Drivers: Hardware drivers available");
  }
  
  // Show memory efficiency info every 50 loops
  if(loop_counter % 50 == 0){
    logMessage("INFO", "IMU_FUSION", "=== Memory Efficiency ===");
    logMessage("INFO", "IMU_FUSION", "Complete library loaded via arcos.hpp");
    logMessage("INFO", "IMU_FUSION", "For smaller footprint, use modular headers:");
    logMessage("INFO", "IMU_FUSION", "  arcos_algorithms.hpp + arcos_drivers_sensors.hpp");
  }
  
  loop_counter++;
  
  // Wait 100ms (simulating 10 Hz update rate)
  delayMs(100);
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