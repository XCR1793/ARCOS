#pragma once

#include <cstdint>
#include <cstddef>

/**
 * @file platform_hal.hpp
 * @brief Platform Hardware Abstraction Layer (HAL)
 * 
 * This file defines platform-agnostic interfaces for hardware operations
 * that can be implemented differently on various platforms (ESP32, STM32, etc.)
 */

/** ============================================================================
 *  PIN ABSTRACTION
 *  ========================================================================= */

/**
 * @brief Platform-independent pin identifier
 * 
 * Each platform implementation maps this to their native pin type
 * (e.g., gpio_num_t on ESP32, GPIO_TypeDef* on STM32)
 */
typedef int32_t PinNumber;

/** Special pin value indicating "not connected" */
constexpr PinNumber PIN_NC = -1;

/**
 * @brief Pin mode configuration
 */
enum class PinMode {
  OUTPUT,           ///< Digital output
  INPUT,            ///< Digital input
  INPUT_PULLUP,     ///< Digital input with pull-up
  INPUT_PULLDOWN,   ///< Digital input with pull-down
  ANALOG            ///< Analog input/output
};

/**
 * @brief Pin drive strength
 */
enum class PinDriveStrength {
  WEAK,             ///< Minimum drive strength
  MEDIUM,           ///< Medium drive strength
  STRONG,           ///< Maximum drive strength
  DEFAULT = MEDIUM  ///< Default drive strength
};

/** ============================================================================
 *  MEMORY ABSTRACTION
 *  ========================================================================= */

/**
 * @brief Memory capabilities flags (bitfield)
 */
enum MemoryCapability : uint32_t {
  MEM_CAP_DEFAULT = 0x00,       ///< Default memory (standard RAM)
  MEM_CAP_DMA = 0x01,           ///< DMA-capable memory
  MEM_CAP_32BIT_ALIGNED = 0x02, ///< 32-bit aligned memory
  MEM_CAP_INTERNAL = 0x04,      ///< Internal RAM (faster access)
  MEM_CAP_EXTERNAL = 0x08,      ///< External RAM/PSRAM
  MEM_CAP_IRAM = 0x10,          ///< Instruction RAM
  MEM_CAP_CACHE_SAFE = 0x20     ///< Cache-safe memory
};

/** ============================================================================
 *  LOGGING ABSTRACTION
 *  ========================================================================= */

/**
 * @brief Log levels
 */
enum class LogLevel {
  NONE,     ///< No logging
  ERROR,    ///< Error messages only
  WARN,     ///< Warnings and errors
  INFO,     ///< Informational messages
  DEBUG,    ///< Debug messages
  VERBOSE   ///< All messages
};

/** ============================================================================
 *  PLATFORM HAL INTERFACE
 *  ========================================================================= */

/**
 * @brief Platform Hardware Abstraction Layer Interface
 * 
 * This interface must be implemented for each target platform.
 * The implementation provides platform-specific functionality for
 * GPIO, memory allocation, timing, and logging.
 */
class IPlatformHAL {
public:
  virtual ~IPlatformHAL() = default;
  
  /** ========================================================================
   *  GPIO OPERATIONS
   *  ====================================================================== */
  
  /**
   * @brief Configure a pin as output or input
   * @param pin Pin number
   * @param mode Pin mode (INPUT, OUTPUT, etc.)
   * @return true if successful, false otherwise
   */
  virtual bool pinMode(PinNumber pin, PinMode mode) = 0;
  
  /**
   * @brief Set pin drive strength
   * @param pin Pin number
   * @param strength Drive strength
   * @return true if successful, false otherwise
   */
  virtual bool setPinDriveStrength(PinNumber pin, PinDriveStrength strength) = 0;
  
  /**
   * @brief Write digital value to pin
   * @param pin Pin number
   * @param value true = HIGH, false = LOW
   * @return true if successful, false otherwise
   */
  virtual bool digitalWrite(PinNumber pin, bool value) = 0;
  
  /**
   * @brief Read digital value from pin
   * @param pin Pin number
   * @return true = HIGH, false = LOW
   */
  virtual bool digitalRead(PinNumber pin) = 0;
  
  /**
   * @brief Connect pin to internal peripheral signal
   * @param pin Pin number
   * @param signal Platform-specific signal identifier
   * @param invert Invert signal polarity
   * @return true if successful, false otherwise
   */
  virtual bool connectPinToSignal(PinNumber pin, uint32_t signal, bool invert = false) = 0;
  
  /** ========================================================================
   *  MEMORY OPERATIONS
   *  ====================================================================== */
  
  /**
   * @brief Allocate memory with specific capabilities
   * @param size Size in bytes
   * @param caps Memory capability flags (bitwise OR of MemoryCapability)
   * @return Pointer to allocated memory, nullptr on failure
   */
  virtual void* allocateMemory(size_t size, uint32_t caps = MEM_CAP_DEFAULT) = 0;
  
  /**
   * @brief Free memory allocated by allocateMemory
   * @param ptr Pointer to memory
   */
  virtual void freeMemory(void* ptr) = 0;
  
  /**
   * @brief Get total available memory
   * @param caps Memory capability filter
   * @return Total memory in bytes
   */
  virtual size_t getTotalMemory(uint32_t caps = MEM_CAP_DEFAULT) = 0;
  
  /**
   * @brief Get free available memory
   * @param caps Memory capability filter
   * @return Free memory in bytes
   */
  virtual size_t getFreeMemory(uint32_t caps = MEM_CAP_DEFAULT) = 0;
  
  /** ========================================================================
   *  TIMING OPERATIONS
   *  ====================================================================== */
  
  /**
   * @brief Get microsecond timestamp
   * @return Microseconds since system start
   */
  virtual uint64_t getMicros() = 0;
  
  /**
   * @brief Get millisecond timestamp
   * @return Milliseconds since system start
   */
  virtual uint32_t getMillis() = 0;
  
  /**
   * @brief Delay for microseconds
   * @param us Microseconds to delay
   */
  virtual void delayMicros(uint32_t us) = 0;
  
  /**
   * @brief Delay for milliseconds
   * @param ms Milliseconds to delay
   */
  virtual void delayMillis(uint32_t ms) = 0;
  
  /** ========================================================================
   *  LOGGING OPERATIONS
   *  ====================================================================== */
  
  /**
   * @brief Set log level
   * @param level Minimum log level to display
   */
  virtual void setLogLevel(LogLevel level) = 0;
  
  /**
   * @brief Log message with level
   * @param level Log level
   * @param tag Log tag/category
   * @param format Printf-style format string
   * @param ... Variable arguments
   */
  virtual void log(LogLevel level, const char* tag, const char* format, ...) = 0;
  
  /** ========================================================================
   *  PLATFORM INFORMATION
   *  ====================================================================== */
  
  /**
   * @brief Get platform name
   * @return Platform identifier string (e.g., "ESP32-S3", "STM32F4")
   */
  virtual const char* getPlatformName() = 0;
  
  /**
   * @brief Get CPU frequency in Hz
   * @return CPU clock frequency
   */
  virtual uint32_t getCpuFrequency() = 0;
};

/** ============================================================================
 *  GLOBAL HAL INSTANCE
 *  ========================================================================= */

/**
 * @brief Get the global platform HAL instance
 * 
 * This function must be implemented by each platform to return
 * a singleton instance of their IPlatformHAL implementation.
 * 
 * @return Pointer to platform HAL instance
 */
IPlatformHAL* getPlatformHAL();

/** ============================================================================
 *  CONVENIENCE MACROS FOR LOGGING
 *  ========================================================================= */

#define PLATFORM_LOG_E(tag, format, ...) getPlatformHAL()->log(LogLevel::ERROR, tag, format, ##__VA_ARGS__)
#define PLATFORM_LOG_W(tag, format, ...) getPlatformHAL()->log(LogLevel::WARN, tag, format, ##__VA_ARGS__)
#define PLATFORM_LOG_I(tag, format, ...) getPlatformHAL()->log(LogLevel::INFO, tag, format, ##__VA_ARGS__)
#define PLATFORM_LOG_D(tag, format, ...) getPlatformHAL()->log(LogLevel::DEBUG, tag, format, ##__VA_ARGS__)
#define PLATFORM_LOG_V(tag, format, ...) getPlatformHAL()->log(LogLevel::VERBOSE, tag, format, ##__VA_ARGS__)
