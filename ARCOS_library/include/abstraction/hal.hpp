/*****************************************************************
 * File:      hal.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Main Hardware Abstraction Layer (HAL) header for ARCOS.
 *    
 *    This single header provides access to all hardware abstraction
 *    interfaces and automatically selects the correct platform
 *    implementation based on compile-time target flags.
 *    
 * Usage:
 *    #include "abstraction/hal.hpp"
 *    
 *    Then use platform-independent HAL interfaces:
 *    - HAL_GPIO_DEFAULT for GPIO operations
 *    - HAL_I2C_DEFAULT for I2C communication
 *    - HAL_PARALLEL_DEFAULT for parallel data output
 *    - etc.
 *    
 * Platform Selection:
 *    Define one of these compiler flags:
 *    - TARGET_AVR_Atmega328p_Uno
 *    - TARGET_ESP32_Wroom32S2_Esp32Dev
 *    - TARGET_ESP32_Wroom32S3_Module
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_HAL_HPP_
#define ARCOS_ABSTRACTION_HAL_HPP_

/*****************************************************************
 * Core HAL Protocol Interfaces
 * 
 * These headers define platform-independent interfaces that all
 * platform implementations must provide.
 *****************************************************************/

// Basic GPIO interfaces
#include "core/hal_gpio_digital.hpp"        // Digital GPIO control
#include "core/hal_gpio_pwm.hpp"            // PWM output control

// Communication protocol interfaces
#include "core/hal_protocal_i2c.hpp"        // I2C protocol
#include "core/hal_protocal_spi.hpp"        // SPI protocol
#include "core/hal_protocal_parallel.hpp"   // Parallel data output

// DMA and buffer management interfaces
#include "core/hal_protocal_dma.hpp"        // DMA buffer management
#include "core/hal_protocal_parallel_buffer.hpp"  // Parallel buffer utilities

// System utilities
#include "core/hal_system_timer.hpp"        // System timer functions

/*****************************************************************
 * Platform-Specific Implementation Selection
 * 
 * Based on the TARGET_* compile-time flag, include the appropriate
 * platform connector which maps generic interfaces to concrete
 * hardware implementations.
 *****************************************************************/

#if defined(TARGET_AVR_Atmega328p_Uno)
  #include "platforms/avr/atmega328p/uno/hal_connector.hpp"
#elif defined(TARGET_ESP32_Wroom32S2_Esp32Dev)
  #include "platforms/esp32/wroom32s2/esp32dev/hal_connector.hpp"
#elif defined(TARGET_ESP32_Wroom32S3_Module)
  #include "platforms/esp32/wroom32s3/module/hal_connector.hpp"
#else
  #error "No platform selected! Define one of: TARGET_AVR_Atmega328p_Uno, TARGET_ESP32_Wroom32S2_Esp32Dev, TARGET_ESP32_Wroom32S3_Module"
#endif

#endif // ARCOS_ABSTRACTION_HAL_HPP_