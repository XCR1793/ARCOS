/*****************************************************************
 * File:      arcos_core.hpp
 * Category:  core
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Core ARCOS functionality including HAL abstractions and
 *    platform-specific implementations. Include this for basic
 *    hardware abstraction without drivers or algorithms.
 *****************************************************************/

#ifndef ARCOS_CORE_HPP_
#define ARCOS_CORE_HPP_

// Core abstraction layer
#include "abstraction/hal.hpp"

// Core HAL interfaces - always included
#include "abstraction/core/hal_gpio_digital.hpp"
#include "abstraction/core/hal_gpio_pwm.hpp"
#include "abstraction/core/hal_system_timer.hpp"
#include "abstraction/core/hal_logging.hpp"

// Protocol abstractions - always included
#include "abstraction/core/hal_protocal_i2c.hpp"
#include "abstraction/core/hal_protocal_spi.hpp"
#include "abstraction/core/hal_protocal_parallel.hpp"
#include "abstraction/core/hal_protocal_parallel_buffer.hpp"
#include "abstraction/core/hal_protocal_dma.hpp"
#include "abstraction/core/hal_interface_i2c.hpp"

// Platform-specific HAL implementations - auto-included based on target
#ifdef TARGET_ESP32_Wroom32S3_Module
  #include "abstraction/platforms/esp32/wroom32s3/module/hal_connector.hpp"
  #include "abstraction/platforms/esp32/wroom32s3/module/hal_gpio_digital_module.hpp"
#endif

#ifdef TARGET_ESP32_Wroom32S2_Esp32Dev
  #include "abstraction/platforms/esp32/wroom32s2/esp32dev/hal_logging_esp32dev.hpp"
#endif

// Add other platform includes as they become available
#ifdef TARGET_AVR_Atmega328p_Uno
  // Future: Arduino Uno specific implementations
#endif

#ifdef TARGET_STM32_Generic
  // Future: STM32 specific implementations
#endif

/** 
 * @brief ARCOS Core Module
 * 
 * Provides hardware abstraction layer (HAL) functionality including:
 * - GPIO operations (digital, PWM)
 * - Communication protocols (I2C, SPI, parallel)
 * - System timers and logging
 * - Platform-specific implementations (automatically included)
 * 
 * Usage:
 * ```cpp
 * #include <arcos_core.hpp>
 * 
 * using namespace arcos::abstraction;
 * 
 * // Use HAL functionality
 * core::HALGPIODigital<2>::setMode(GPIO_MODE_OUTPUT);
 * ```
 */
namespace arcos::core {
  constexpr const char* MODULE_NAME = "ARCOS_CORE";
  constexpr const char* VERSION = "1.0.0";
}

#endif // ARCOS_CORE_HPP_