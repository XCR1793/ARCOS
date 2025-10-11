/*****************************************************************
 * File:      main.cpp
 * Category:  test
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Test the ARCOS logging system on ESP32-S3.
 *****************************************************************/

#include "abstraction/hal.hpp"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using namespace arcos::abstraction;

// Example tags for different components
static constexpr const char* TAG_MAIN = "MAIN";
static constexpr const char* TAG_TEST = "TEST";
static constexpr const char* TAG_SYSTEM = "SYSTEM";

extern "C" void app_main(void){
  // Wait 5 seconds for serial monitor to connect
  vTaskDelay(pdMS_TO_TICKS(5000));
  
  // Initialize logging system
  HAL_LOG_DEFAULT::Init();
  
  HAL_LOG_DEFAULT::LogInfo(TAG_MAIN, "========================================");
  HAL_LOG_DEFAULT::LogInfo(TAG_MAIN, "  ARCOS Logging System Test");
  HAL_LOG_DEFAULT::LogInfo(TAG_MAIN, "  Platform: ESP32-S3");
  HAL_LOG_DEFAULT::LogInfo(TAG_MAIN, "========================================");
  HAL_LOG_DEFAULT::Printf("\n");
  
  // Test 1: Basic log levels
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "Test 1: Basic Log Levels");
  HAL_LOG_DEFAULT::LogError(TAG_TEST, "This is an ERROR message");
  HAL_LOG_DEFAULT::LogWarning(TAG_TEST, "This is a WARNING message");
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "This is an INFO message");
  HAL_LOG_DEFAULT::LogDebug(TAG_TEST, "This is a DEBUG message (may not show)");
  HAL_LOG_DEFAULT::Printf("\n");
  
  vTaskDelay(pdMS_TO_TICKS(500));
  
  // Test 2: Formatted output
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "Test 2: Formatted Output");
  int value = 42;
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "Integer value: %d", value);
  
  uint32_t hex_value = 0xDEADBEEF;
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "Hex value: 0x%08X", hex_value);
  
  float temperature = 23.5f;
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "Temperature: %.1f C", temperature);
  HAL_LOG_DEFAULT::Printf("\n");
  
  vTaskDelay(pdMS_TO_TICKS(500));
  
  // Test 3: Hex dump
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "Test 3: Hex Dump");
  uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "Dumping 16 bytes:");
  HAL_LOG_DEFAULT::LogHexDump(TAG_TEST, data, sizeof(data));
  HAL_LOG_DEFAULT::Printf("\n");
  
  vTaskDelay(pdMS_TO_TICKS(500));
  
  // Test 4: Runtime log level change
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "Test 4: Runtime Log Level Control");
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "Current level: %d", 
                           static_cast<int>(HAL_LOG_DEFAULT::GetLogLevel()));
  
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "Enabling DEBUG level...");
  HAL_LOG_DEFAULT::SetLogLevel(arcos::abstraction::logging::LogLevel::Debug);
  HAL_LOG_DEFAULT::LogDebug(TAG_TEST, "Debug messages now visible!");
  HAL_LOG_DEFAULT::Printf("\n");
  
  vTaskDelay(pdMS_TO_TICKS(500));
  
  // Test 5: Multiple components
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "Test 5: Multiple Component Tags");
  HAL_LOG_DEFAULT::LogInfo(TAG_MAIN, "Message from MAIN component");
  HAL_LOG_DEFAULT::LogInfo(TAG_SYSTEM, "Message from SYSTEM component");
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "Message from TEST component");
  HAL_LOG_DEFAULT::Printf("\n");
  
  vTaskDelay(pdMS_TO_TICKS(500));
  
  // Test 6: Compile-time checks
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "Test 6: Compile-Time Features");
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "ERROR_ENABLED: %s", 
                           HAL_LOG_DEFAULT::ERROR_ENABLED ? "YES" : "NO");
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "WARNING_ENABLED: %s", 
                           HAL_LOG_DEFAULT::WARNING_ENABLED ? "YES" : "NO");
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "INFO_ENABLED: %s", 
                           HAL_LOG_DEFAULT::INFO_ENABLED ? "YES" : "NO");
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "DEBUG_ENABLED: %s", 
                           HAL_LOG_DEFAULT::DEBUG_ENABLED ? "YES" : "NO");
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "VERBOSE_ENABLED: %s", 
                           HAL_LOG_DEFAULT::VERBOSE_ENABLED ? "YES" : "NO");
  HAL_LOG_DEFAULT::Printf("\n");
  
  vTaskDelay(pdMS_TO_TICKS(500));
  
  // Test 7: Custom printf
  HAL_LOG_DEFAULT::LogInfo(TAG_TEST, "Test 7: Custom Printf (Progress Bar)");
  for(int i = 0; i <= 100; i += 10){
    HAL_LOG_DEFAULT::Printf("\rProgress: [");
    int bars = i / 10;
    for(int j = 0; j < 10; j++){
      HAL_LOG_DEFAULT::Printf(j < bars ? "=" : " ");
    }
    HAL_LOG_DEFAULT::Printf("] %3d%%", i);
    HAL_LOG_DEFAULT::Flush();
    vTaskDelay(pdMS_TO_TICKS(200));
  }
  HAL_LOG_DEFAULT::Printf("\n\n");
  
  vTaskDelay(pdMS_TO_TICKS(500));
  
  // Final message
  HAL_LOG_DEFAULT::LogInfo(TAG_MAIN, "========================================");
  HAL_LOG_DEFAULT::LogInfo(TAG_MAIN, "  All Tests Completed Successfully!");
  HAL_LOG_DEFAULT::LogInfo(TAG_MAIN, "========================================");
  HAL_LOG_DEFAULT::Printf("\n");
  
  // Continuous loop with periodic messages
  int counter = 0;
  while(true){
    vTaskDelay(pdMS_TO_TICKS(5000));
    counter++;
    HAL_LOG_DEFAULT::LogInfo(TAG_MAIN, "Heartbeat #%d - System running...", counter);
    
    if(counter % 3 == 0){
      HAL_LOG_DEFAULT::LogDebug(TAG_SYSTEM, "Memory check, free heap: ~%d bytes", 
                                esp_get_free_heap_size());
    }
  }
}
