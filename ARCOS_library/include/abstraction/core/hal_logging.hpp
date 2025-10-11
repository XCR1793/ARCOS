/*****************************************************************
 * File:      hal_logging.hpp
 * Category:  abstraction/core
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Provides a template HAL abstraction for logging and debug
 *    output. Enables compile-time log level filtering and
 *    platform-independent logging interface with zero runtime
 *    overhead for disabled log levels.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_CORE_HAL_LOGGING_HPP_
#define ARCOS_ABSTRACTION_CORE_HAL_LOGGING_HPP_

#include <stdint.h>

namespace arcos::abstraction{
  namespace logging{

    enum struct LogLevel{
      None    = 0,
      Error   = 1,
      Warning = 2,
      Info    = 3,
      Debug   = 4,
      Verbose = 5
    };

    enum struct LogOutput{
      Serial    = 0,
      SdCard    = 1,
      Network   = 2,
      Custom    = 3
    };

    template <typename PlatformImplementation, 
              LogLevel MaxLogLevel = LogLevel::Info,
              LogOutput OutputTarget = LogOutput::Serial>
    struct HalLogging{

      static constexpr LogLevel MAX_LOG_LEVEL = MaxLogLevel;
      static constexpr LogOutput OUTPUT_TARGET = OutputTarget;
      static constexpr bool ERROR_ENABLED = (MaxLogLevel >= LogLevel::Error);
      static constexpr bool WARNING_ENABLED = (MaxLogLevel >= LogLevel::Warning);
      static constexpr bool INFO_ENABLED = (MaxLogLevel >= LogLevel::Info);
      static constexpr bool DEBUG_ENABLED = (MaxLogLevel >= LogLevel::Debug);
      static constexpr bool VERBOSE_ENABLED = (MaxLogLevel >= LogLevel::Verbose);

      static inline void Init(){
        PlatformImplementation::Init();
      }

      template <typename... Args>
      static inline void LogError(const char* tag, const char* format, Args... args){
        if constexpr(ERROR_ENABLED){
          PlatformImplementation::LogError(tag, format, args...);
        }
      }

      template <typename... Args>
      static inline void LogWarning(const char* tag, const char* format, Args... args){
        if constexpr(WARNING_ENABLED){
          PlatformImplementation::LogWarning(tag, format, args...);
        }
      }

      template <typename... Args>
      static inline void LogInfo(const char* tag, const char* format, Args... args){
        if constexpr(INFO_ENABLED){
          PlatformImplementation::LogInfo(tag, format, args...);
        }
      }

      template <typename... Args>
      static inline void LogDebug(const char* tag, const char* format, Args... args){
        if constexpr(DEBUG_ENABLED){
          PlatformImplementation::LogDebug(tag, format, args...);
        }
      }

      template <typename... Args>
      static inline void LogVerbose(const char* tag, const char* format, Args... args){
        if constexpr(VERBOSE_ENABLED){
          PlatformImplementation::LogVerbose(tag, format, args...);
        }
      }

      static inline void SetLogLevel(LogLevel level){
        PlatformImplementation::SetLogLevel(level);
      }

      static inline LogLevel GetLogLevel(){
        return PlatformImplementation::GetLogLevel();
      }

      static inline void Flush(){
        PlatformImplementation::Flush();
      }

      static inline void LogHexDump(const char* tag, const void* data, 
                                    uintptr_t length, LogLevel level = LogLevel::Debug){
        if constexpr(DEBUG_ENABLED){
          if(static_cast<uint8_t>(level) <= static_cast<uint8_t>(MAX_LOG_LEVEL)){
            PlatformImplementation::LogHexDump(tag, data, length, level);
          }
        }
      }

      template <typename... Args>
      static inline void Printf(const char* format, Args... args){
        PlatformImplementation::Printf(format, args...);
      }
    };

  } // namespace logging

  template <typename PlatformImplementation>
  using HalLoggingDefault = logging::HalLogging<PlatformImplementation, 
                                                 logging::LogLevel::Info, 
                                                 logging::LogOutput::Serial>;

} // namespace arcos::abstraction

#endif // ARCOS_ABSTRACTION_CORE_HAL_LOGGING_HPP_
