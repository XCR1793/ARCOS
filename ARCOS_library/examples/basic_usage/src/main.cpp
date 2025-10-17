/*****************************************************************
 * File:      main.cpp
 * Category:  example
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Basic ARCOS library inclusion example demonstrating platform-
 *    agnostic design and modular functionality access.
 *****************************************************************/

#include <arcos.hpp>

// Use ARCOS namespaces for convenience
using namespace arcos;

// Global variables for demonstration
bool led_state = false;
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

void setup(){
  // Initialize serial/logging for different platforms
  #ifdef ARDUINO
    Serial.begin(115200);
    while(!Serial) delayMs(10);
  #endif
  
  logMessage("INFO", "ARCOS_BASIC", "Starting ARCOS Basic Usage Example");
  logMessage("INFO", "ARCOS_BASIC", "This demonstrates modular library inclusion");
  
  // Show ARCOS version and module availability
  logMessage("INFO", "ARCOS_BASIC", "=== ARCOS Library Info ===");
  logMessage("INFO", "ARCOS_BASIC", "✓ Complete library included (arcos.hpp)");
  logMessage("INFO", "ARCOS_BASIC", "✓ Core HAL available");
  logMessage("INFO", "ARCOS_BASIC", "✓ Algorithms module available");
  logMessage("INFO", "ARCOS_BASIC", "✓ Drivers module available");
  
  // Demonstrate namespace access
  logMessage("INFO", "ARCOS_BASIC", "Namespace structure:");
  logMessage("INFO", "ARCOS_BASIC", "  arcos::core - Core HAL functionality");
  logMessage("INFO", "ARCOS_BASIC", "  arcos::algorithms - Sensor fusion & math");
  logMessage("INFO", "ARCOS_BASIC", "  arcos::drivers - Hardware drivers");
}

void loop(){
  // Simple demonstration loop
  led_state = !led_state;
  
  // Log current state
  if(led_state){
    logMessage("INFO", "ARCOS_BASIC", "Loop: LED ON (simulated)");
  } else {
    logMessage("INFO", "ARCOS_BASIC", "Loop: LED OFF (simulated)");
  }
  
  // Show features every 10 loops
  if(loop_counter % 10 == 0){
    logMessage("INFO", "ARCOS_BASIC", "=== ARCOS Features Available ===");
    logMessage("INFO", "ARCOS_BASIC", "• Hardware Abstraction Layer");
    logMessage("INFO", "ARCOS_BASIC", "• Platform-specific implementations");
    logMessage("INFO", "ARCOS_BASIC", "• Sensor fusion algorithms");
    logMessage("INFO", "ARCOS_BASIC", "• Hardware component drivers");
    logMessage("INFO", "ARCOS_BASIC", "• Cross-platform compatibility");
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