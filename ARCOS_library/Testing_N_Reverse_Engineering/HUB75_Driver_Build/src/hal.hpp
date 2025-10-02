/*****************************************************************
 * File:      hal.hpp
 * Category:  abstraction
 * 
 * Purpose:
 *    Main HAL header - includes all core HAL APIs and automatically
 *    selects correct platform implementation via compile-time flags
 *    
 * Usage:
 *    #include "hal.hpp"  // That's it! Everything is available
 *****************************************************************/

#ifndef HUB75_ABSTRACTION_HAL_HPP_
#define HUB75_ABSTRACTION_HAL_HPP_

// Core HAL APIs (platform-agnostic public interfaces)
#include "core/platform_hal.hpp"
#include "core/hal_protocal_parallel.hpp"
#include "core/hal_protocal_dma.hpp"
#include "core/hal_protocal_parallel_buffer.hpp"

// Platform selector - automatically includes correct implementation
#if defined(TARGET_ESP32_Wroom32S3_Module)
  #include "platforms/esp32/wroom32s3/module/hal_connector.hpp"
#elif defined(TARGET_RP2040_Pico)
  #include "platforms/rp2040/pico/hal_connector.hpp"
#elif defined(TARGET_STM32F407_Discovery)
  #include "platforms/stm32/f407/discovery/hal_connector.hpp"
#else
  // Default to ESP32-S3 if no platform specified
  #define TARGET_ESP32_Wroom32S3_Module
  #include "platforms/esp32/wroom32s3/module/hal_connector.hpp"
#endif

#endif // HUB75_ABSTRACTION_HAL_HPP_
