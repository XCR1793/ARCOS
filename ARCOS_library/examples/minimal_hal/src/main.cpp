/*****************************************************************
 * File:      main.cpp
 * Category:  example
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Minimal HAL example demonstrating core hardware abstraction
 *    functionality without drivers or algorithms. Shows smallest
 *    possible ARCOS footprint using modular inclusion.
 *****************************************************************/

// Include only core HAL + platform implementations
#include <arcos_core.hpp>

using namespace arcos;

// Simple state tracking
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
  
  logMessage("INFO", "MINIMAL_HAL", "Starting ARCOS Minimal HAL Example");
  logMessage("INFO", "MINIMAL_HAL", "Using modular inclusion: arcos_core.hpp");
  
  // Show what's included and what's not
  logMessage("INFO", "MINIMAL_HAL", "=== Included Modules ===");
  logMessage("INFO", "MINIMAL_HAL", "✓ Core HAL interfaces");
  logMessage("INFO", "MINIMAL_HAL", "✓ Platform-specific implementations");
  logMessage("INFO", "MINIMAL_HAL", "✓ GPIO, Timer, Protocol abstractions");
  
  logMessage("INFO", "MINIMAL_HAL", "=== Excluded Modules ===");
  logMessage("INFO", "MINIMAL_HAL", "✗ Sensor fusion algorithms");
  logMessage("INFO", "MINIMAL_HAL", "✗ Hardware drivers");
  logMessage("INFO", "MINIMAL_HAL", "✗ Display/sensor components");
  
  logMessage("INFO", "MINIMAL_HAL", "=== Benefits ===");
  logMessage("INFO", "MINIMAL_HAL", "• Smallest memory footprint");
  logMessage("INFO", "MINIMAL_HAL", "• Fastest compilation");
  logMessage("INFO", "MINIMAL_HAL", "• Platform abstraction only");
  
  // Show platform detection
  #ifdef TARGET_ESP32_Wroom32S3_Module
    logMessage("INFO", "MINIMAL_HAL", "Platform: ESP32-S3 detected");
  #elif defined(TARGET_ESP32_Wroom32S2_Esp32Dev)
    logMessage("INFO", "MINIMAL_HAL", "Platform: ESP32-S2 detected");
  #elif defined(TARGET_AVR_Atmega328p_Uno)
    logMessage("INFO", "MINIMAL_HAL", "Platform: Arduino Uno detected");
  #elif defined(ARDUINO)
    logMessage("INFO", "MINIMAL_HAL", "Platform: Arduino detected");
  #else
    logMessage("INFO", "MINIMAL_HAL", "Platform: Custom/Generic");
  #endif
}

void loop(){
  // Toggle LED state (simulated)
  led_state = !led_state;
  
  // Log current state
  if(led_state){
    logMessage("INFO", "MINIMAL_HAL", "HAL GPIO: LED ON (simulated)");
  } else {
    logMessage("INFO", "MINIMAL_HAL", "HAL GPIO: LED OFF (simulated)");
  }
  
  // Show minimal features every 10 loops
  if(loop_counter % 10 == 0){
    logMessage("INFO", "MINIMAL_HAL", "=== Minimal HAL Features ===");
    logMessage("INFO", "MINIMAL_HAL", "• Platform-agnostic GPIO interfaces");
    logMessage("INFO", "MINIMAL_HAL", "• System timer abstractions");
    logMessage("INFO", "MINIMAL_HAL", "• Communication protocol interfaces");
    logMessage("INFO", "MINIMAL_HAL", "• Logging system");
    logMessage("INFO", "MINIMAL_HAL", "• Cross-platform compatibility");
  }
  
  // Show memory efficiency every 20 loops
  if(loop_counter % 20 == 0){
    logMessage("INFO", "MINIMAL_HAL", "=== Memory Efficiency ===");
    logMessage("INFO", "MINIMAL_HAL", "Only core HAL loaded - minimal footprint");
    logMessage("INFO", "MINIMAL_HAL", "To add features, include additional modules:");
    logMessage("INFO", "MINIMAL_HAL", "  + arcos_algorithms.hpp (sensor fusion)");
    logMessage("INFO", "MINIMAL_HAL", "  + arcos_drivers.hpp (hardware drivers)");
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

/*
 * This example demonstrates:
 * 
 * 1. Minimal ARCOS footprint using only arcos_core.hpp
 * 2. Platform-specific implementations automatically included
 * 3. Basic GPIO, timing, and logging functionality
 * 4. No drivers or algorithms loaded - smallest memory usage
 * 5. Foundation for custom implementations
 * 
 * Use cases for minimal HAL:
 * - Custom platform bring-up
 * - Memory-constrained applications  
 * - HAL interface validation
 * - Building custom driver implementations
 * - Educational purposes
 */