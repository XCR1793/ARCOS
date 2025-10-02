#pragma once

#include "platform_hal.hpp"
#include <stdarg.h>

/**
 * @file esp32_platform_impl.hpp
 * @brief ESP32/ESP-IDF Platform HAL Implementation
 * 
 * This file provides the ESP32-specific implementation of the IPlatformHAL interface.
 * It maps the platform-agnostic API to ESP-IDF functions.
 */

/**
 * @brief ESP32 Platform HAL Implementation
 */
class ESP32PlatformHAL : public IPlatformHAL {
public:
  ESP32PlatformHAL();
  ~ESP32PlatformHAL() override = default;
  
  /** GPIO Operations */
  bool pinMode(PinNumber pin, PinMode mode) override;
  bool setPinDriveStrength(PinNumber pin, PinDriveStrength strength) override;
  bool digitalWrite(PinNumber pin, bool value) override;
  bool digitalRead(PinNumber pin) override;
  bool connectPinToSignal(PinNumber pin, uint32_t signal, bool invert = false) override;
  
  /** Memory Operations */
  void* allocateMemory(size_t size, uint32_t caps = MEM_CAP_DEFAULT) override;
  void freeMemory(void* ptr) override;
  size_t getTotalMemory(uint32_t caps = MEM_CAP_DEFAULT) override;
  size_t getFreeMemory(uint32_t caps = MEM_CAP_DEFAULT) override;
  
  /** Timing Operations */
  uint64_t getMicros() override;
  uint32_t getMillis() override;
  void delayMicros(uint32_t us) override;
  void delayMillis(uint32_t ms) override;
  
  /** Logging Operations */
  void setLogLevel(LogLevel level) override;
  void log(LogLevel level, const char* tag, const char* format, ...) override;
  
  /** Platform Information */
  const char* getPlatformName() override;
  uint32_t getCpuFrequency() override;
  
private:
  LogLevel current_log_level;
  char log_buffer[256];  // Buffer for formatted log messages
};

/**
 * @brief Get the ESP32 platform HAL singleton instance
 * @return Pointer to ESP32 platform HAL
 */
ESP32PlatformHAL* getESP32PlatformHAL();
