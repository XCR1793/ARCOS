/*****************************************************************
 * File:      hal_logging_uno.hpp
 * Category:  abstraction/platforms/avr/atmega328p/uno
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    AVR ATmega328P (Arduino Uno) platform implementation of the 
 *    logging HAL.
 *    
 *    Uses Arduino Serial for output with simple formatting.
 *    Optimized for minimal memory footprint on resource-constrained
 *    AVR microcontrollers.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_PLATFORMS_AVR_ATMEGA328P_UNO_HAL_LOGGING_UNO_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_AVR_ATMEGA328P_UNO_HAL_LOGGING_UNO_HPP_

#include <stdint.h>
#include <Arduino.h>

namespace arcos::abstraction{

  /**
   * @brief AVR ATmega328P implementation of the logging HAL.
   * 
   * Uses Arduino Serial for output with lightweight formatting
   * suitable for constrained AVR environments.
   */
  class AVR_ATmega328P_Logging{
  private:
    // Runtime log level
    static inline logging::LogLevel runtime_level_ = logging::LogLevel::Info;

    // Log level prefixes
    static constexpr const char* PREFIX_ERROR = "[ERROR] ";
    static constexpr const char* PREFIX_WARNING = "[WARN ] ";
    static constexpr const char* PREFIX_INFO = "[INFO ] ";
    static constexpr const char* PREFIX_DEBUG = "[DEBUG] ";
    static constexpr const char* PREFIX_VERBOSE = "[VERB ] ";

    // Helper to print prefix and tag
    static inline void PrintPrefix(const char* prefix, const char* tag){
      Serial.print(prefix);
      Serial.print(tag);
      Serial.print(": ");
    }

  public:
    /**
     * @brief Initialize the AVR logging system.
     * 
     * Ensures Serial is initialized. User should call Serial.begin()
     * with appropriate baud rate before calling Init().
     */
    static void Init(){
      // Serial should already be initialized by user with Serial.begin()
      // Just ensure it's available
      if(!Serial){
        Serial.begin(115200);
      }
    }

    /**
     * @brief Log an error message.
     */
    static void LogError(const char* tag, const char* format, ...){
      if(runtime_level_ >= logging::LogLevel::Error){
        PrintPrefix(PREFIX_ERROR, tag);
        
        char buffer[128];  // Reduced buffer for AVR
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        Serial.println(buffer);
      }
    }

    /**
     * @brief Log a warning message.
     */
    static void LogWarning(const char* tag, const char* format, ...){
      if(runtime_level_ >= logging::LogLevel::Warning){
        PrintPrefix(PREFIX_WARNING, tag);
        
        char buffer[128];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        Serial.println(buffer);
      }
    }

    /**
     * @brief Log an informational message.
     */
    static void LogInfo(const char* tag, const char* format, ...){
      if(runtime_level_ >= logging::LogLevel::Info){
        PrintPrefix(PREFIX_INFO, tag);
        
        char buffer[128];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        Serial.println(buffer);
      }
    }

    /**
     * @brief Log a debug message.
     */
    static void LogDebug(const char* tag, const char* format, ...){
      if(runtime_level_ >= logging::LogLevel::Debug){
        PrintPrefix(PREFIX_DEBUG, tag);
        
        char buffer[128];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        Serial.println(buffer);
      }
    }

    /**
     * @brief Log a verbose debug message.
     */
    static void LogVerbose(const char* tag, const char* format, ...){
      if(runtime_level_ >= logging::LogLevel::Verbose){
        PrintPrefix(PREFIX_VERBOSE, tag);
        
        char buffer[128];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        Serial.println(buffer);
      }
    }

    /**
     * @brief Set the runtime log level.
     */
    static void SetLogLevel(logging::LogLevel level){
      runtime_level_ = level;
    }

    /**
     * @brief Get the current runtime log level.
     */
    static logging::LogLevel GetLogLevel(){
      return runtime_level_;
    }

    /**
     * @brief Flush buffered log output.
     */
    static void Flush(){
      Serial.flush();
    }

    /**
     * @brief Log raw data as hexadecimal dump.
     */
    static void LogHexDump(const char* tag, const void* data, 
                           uintptr_t length, logging::LogLevel level){
      if(static_cast<uint8_t>(runtime_level_) >= static_cast<uint8_t>(level)){
        PrintPrefix(PREFIX_DEBUG, tag);
        Serial.print("Hex dump (");
        Serial.print((unsigned int)length);
        Serial.println(" bytes):");
        
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        for(uintptr_t i = 0; i < length; i++){
          if(i > 0 && i % 16 == 0){
            Serial.println();
          }
          if(bytes[i] < 0x10){
            Serial.print('0');
          }
          Serial.print(bytes[i], HEX);
          Serial.print(' ');
        }
        Serial.println();
      }
    }

    /**
     * @brief Direct printf-style output without formatting.
     */
    static void Printf(const char* format, ...){
      char buffer[128];
      va_list args;
      va_start(args, format);
      vsnprintf(buffer, sizeof(buffer), format, args);
      va_end(args);
      
      Serial.print(buffer);
    }
  };

  // Type alias for convenient usage
  using AVR_ATmega328P_LoggingDefault = logging::HalLogging<AVR_ATmega328P_Logging, 
                                                              logging::LogLevel::Info, 
                                                              logging::LogOutput::Serial>;

  // Debug build with verbose logging (not recommended for AVR due to memory constraints)
  using AVR_ATmega328P_LoggingDebug = logging::HalLogging<AVR_ATmega328P_Logging, 
                                                            logging::LogLevel::Debug, 
                                                            logging::LogOutput::Serial>;

  // Production build with errors only (recommended for AVR)
  using AVR_ATmega328P_LoggingProduction = logging::HalLogging<AVR_ATmega328P_Logging, 
                                                                 logging::LogLevel::Error, 
                                                                 logging::LogOutput::Serial>;

} // namespace arcos::abstraction

#endif // ARCOS_ABSTRACTION_PLATFORMS_AVR_ATMEGA328P_UNO_HAL_LOGGING_UNO_HPP_
