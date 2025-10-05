/*****************************************************************
 * File:      hal_connector.hpp
 * Category:  abstraction/platforms/esp32/wroom32s3/module
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    HAL connector for ESP32-S3 WROOM32/Module platform.
 *    
 *    This file maps abstract HAL interfaces to concrete ESP32-S3
 *    platform implementations. It serves as the integration point
 *    between the hardware-agnostic ARCOS abstraction layer and
 *    the ESP32-S3 specific driver implementations.
 *    
 *    Applications should not include this file directly - instead
 *    include "abstraction/hal.hpp" which automatically selects
 *    the correct platform connector based on compile-time flags.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_CONNECTOR_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_CONNECTOR_HPP_

/*****************************************************************
 * SECTION 1: Basic GPIO and Communication Protocol Implementations
 *****************************************************************/

#include "hal_gpio_digital_module.hpp"      // Digital GPIO control
#include "hal_gpio_pwm_module.hpp"          // PWM output control
#include "hal_system_timer_module.hpp"      // System timer functions
#include "hal_protocal_i2c_module.hpp"      // I2C protocol implementation
#include "hal_protocal_spi_module.hpp"      // SPI protocol implementation
#include "hal_interface_i2c_module.hpp"     // I2C interface utilities

/*****************************************************************
 * SECTION 2: Parallel Data Transmission Implementations
 *****************************************************************/

#include "lcd_parallel.hpp"                 // LCD_CAM parallel interface
#include "i2s_parallel_driver.hpp"          // I2S parallel interface

/*****************************************************************
 * SECTION 3: DMA Buffer Management Implementations
 *****************************************************************/

#include "parallel_buffer_impl.hpp"         // DMA buffer manager for parallel output

/*****************************************************************
 * Platform Type Aliases
 * 
 * These type aliases map generic HAL interface names to specific
 * ESP32-S3 implementations. This allows application code to use
 * platform-independent names while the build system selects the
 * correct implementation at compile-time.
 *****************************************************************/

namespace arcos::abstraction{

/*****************************************************************
 * GPIO Implementations
 *****************************************************************/

/** Default digital GPIO implementation for ESP32-S3 */
using HAL_GPIO_DEFAULT = arcos::abstraction::HAL_GPIO_DIGITAL;

/*****************************************************************
 * Communication Protocol Implementations
 *****************************************************************/

/** Default I2C protocol implementation for ESP32-S3 */
using HAL_I2C_DEFAULT = arcos::abstraction::ESP32S3_I2C;

/*****************************************************************
 * Timer Implementations
 *****************************************************************/

/** Default system timer implementation for ESP32-S3 */
using HAL_TIMER_DEFAULT = arcos::abstraction::HAL_SYSTEM_TIMER::ESP32S3;

/*****************************************************************
 * Parallel Data Transmission Implementations
 * 
 * ESP32-S3 provides two hardware peripherals for parallel data output:
 * 
 * 1. LCD_CAM (LcdParallel):
 *    - Dedicated LCD interface peripheral
 *    - Higher performance and lower CPU overhead
 *    - Up to 16-bit parallel data width
 *    - Recommended for display applications
 * 
 * 2. I2S (I2sParallelDriver):
 *    - Repurposed I2S peripheral for parallel output
 *    - More flexible pin mapping
 *    - Lower maximum clock frequencies
 *    - Alternative if LCD_CAM pins unavailable
 *****************************************************************/

/** LCD_CAM parallel interface (high-performance display interface) */
using HAL_PARALLEL_LCD = LcdParallel;

/** I2S parallel interface (flexible alternative) */
using HAL_PARALLEL_I2S = I2sParallelDriver;

/** 
 * Default parallel implementation for ESP32-S3
 * 
 * LCD_CAM is selected as default because it provides:
 * - Higher clock frequencies (20MHz+)
 * - Lower DMA overhead
 * - Better performance for display applications
 * 
 * To use I2S instead, explicitly use HAL_PARALLEL_I2S in your code
 */
using HAL_PARALLEL_DEFAULT = HAL_PARALLEL_LCD;

} // namespace arcos::abstraction

#endif // ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_CONNECTOR_HPP_
