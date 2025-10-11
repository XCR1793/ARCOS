/*****************************************************************
 * File:      hal_logging_module.hpp
 * Category:  abstraction/platforms/esp32/wroom32s3/module
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    ESP32-S3 platform implementation of the logging HAL.
 *    
 *    Uses ESP-IDF's esp_log system for consistent integration
 *    with other ESP32 components. Supports color-coded output,
 *    timestamps, and runtime log level filtering.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_LOGGING_MODULE_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_LOGGING_MODULE_HPP_

#include <stdint.h>
#include <cstdarg>
#include "esp_log.h"

namespace arcos::abstraction{

  /**
   * @brief ESP32-S3 implementation of the logging HAL.
   * 
   * Integrates with ESP-IDF's esp_log system for consistent
   * logging across platform components.
   */
  class ESP32S3_Logging{
  private:
    // Runtime log level (can be adjusted without recompiling)
    static inline logging::LogLevel runtime_level_ = logging::LogLevel::Info;

    // Convert ARCOS LogLevel to ESP-IDF esp_log_level_t
    static inline esp_log_level_t ToEspLogLevel(logging::LogLevel level){
      switch(level){
        case logging::LogLevel::None:    return ESP_LOG_NONE;
        case logging::LogLevel::Error:   return ESP_LOG_ERROR;
        case logging::LogLevel::Warning: return ESP_LOG_WARN;
        case logging::LogLevel::Info:    return ESP_LOG_INFO;
        case logging::LogLevel::Debug:   return ESP_LOG_DEBUG;
        case logging::LogLevel::Verbose: return ESP_LOG_VERBOSE;
        default:                         return ESP_LOG_INFO;
      }
    }

  public:
    /**
     * @brief Initialize the ESP32-S3 logging system.
     * 
     * Configures ESP-IDF logging defaults. Serial output is
     * already initialized by ESP-IDF before main().
     */
    static void Init(){
      // ESP-IDF logging is initialized by default
      // Set default log level for all tags
      esp_log_level_set("*", ToEspLogLevel(runtime_level_));
    }

    /**
     * @brief Log an error message.
     */
    static void LogError(const char* tag, const char* format, ...){
      if(runtime_level_ >= logging::LogLevel::Error){
        va_list args;
        va_start(args, format);
        esp_log_writev(ESP_LOG_ERROR, tag, format, args);
        va_end(args);
      }
    }

    /**
     * @brief Log a warning message.
     */
    static void LogWarning(const char* tag, const char* format, ...){
      if(runtime_level_ >= logging::LogLevel::Warning){
        va_list args;
        va_start(args, format);
        esp_log_writev(ESP_LOG_WARN, tag, format, args);
        va_end(args);
      }
    }

    /**
     * @brief Log an informational message.
     */
    static void LogInfo(const char* tag, const char* format, ...){
      if(runtime_level_ >= logging::LogLevel::Info){
        va_list args;
        va_start(args, format);
        esp_log_writev(ESP_LOG_INFO, tag, format, args);
        va_end(args);
      }
    }

    /**
     * @brief Log a debug message.
     */
    static void LogDebug(const char* tag, const char* format, ...){
      if(runtime_level_ >= logging::LogLevel::Debug){
        va_list args;
        va_start(args, format);
        esp_log_writev(ESP_LOG_DEBUG, tag, format, args);
        va_end(args);
      }
    }

    /**
     * @brief Log a verbose debug message.
     */
    static void LogVerbose(const char* tag, const char* format, ...){
      if(runtime_level_ >= logging::LogLevel::Verbose){
        va_list args;
        va_start(args, format);
        esp_log_writev(ESP_LOG_VERBOSE, tag, format, args);
        va_end(args);
      }
    }

    /**
     * @brief Set the runtime log level.
     */
    static void SetLogLevel(logging::LogLevel level){
      runtime_level_ = level;
      esp_log_level_set("*", ToEspLogLevel(level));
    }

    /**
     * @brief Get the current runtime log level.
     */
    static logging::LogLevel GetLogLevel(){
      return runtime_level_;
    }

    /**
     * @brief Flush buffered log output.
     * 
     * ESP32 UART output is typically unbuffered, but this
     * ensures any pending writes are completed.
     */
    static void Flush(){
      // Force flush UART output
      fflush(stdout);
    }

    /**
     * @brief Log raw data as hexadecimal dump.
     */
    static void LogHexDump(const char* tag, const void* data, 
                           uintptr_t length, logging::LogLevel level){
      if(static_cast<uint8_t>(runtime_level_) >= static_cast<uint8_t>(level)){
        // Use ESP-IDF's hex dump function
        ESP_LOG_BUFFER_HEX_LEVEL(tag, data, length, ToEspLogLevel(level));
      }
    }

    /**
     * @brief Direct printf-style output without formatting.
     */
    static void Printf(const char* format, ...){
      va_list args;
      va_start(args, format);
      vprintf(format, args);
      va_end(args);
    }
  };

  // Type alias for convenient usage
  using ESP32S3_LoggingDefault = logging::HalLogging<ESP32S3_Logging, 
                                                      logging::LogLevel::Info, 
                                                      logging::LogOutput::Serial>;

  // Debug build with verbose logging
  using ESP32S3_LoggingDebug = logging::HalLogging<ESP32S3_Logging, 
                                                    logging::LogLevel::Debug, 
                                                    logging::LogOutput::Serial>;

  // Verbose build with all logging
  using ESP32S3_LoggingVerbose = logging::HalLogging<ESP32S3_Logging, 
                                                      logging::LogLevel::Verbose, 
                                                      logging::LogOutput::Serial>;

  // Production build with errors only
  using ESP32S3_LoggingProduction = logging::HalLogging<ESP32S3_Logging, 
                                                         logging::LogLevel::Error, 
                                                         logging::LogOutput::Serial>;

} // namespace arcos::abstraction

#endif // ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_LOGGING_MODULE_HPP_
