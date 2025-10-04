/*****************************************************************
 * File:      platform_common.hpp
 * Category:  platform/esp32_s3
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Platform common definitions for ESP32-S3 (WROOM/Module variants)
 *    Provides type aliases and capabilities for platform-specific drivers
 *    
 * Note:
 *    This file makes porting to ARCOS architecture easier by
 *    centralizing platform-specific type definitions.
 *****************************************************************/

#ifndef ARCOS_PLATFORMS_ESP32_WROOM32S3_MODULE_PLATFORM_COMMON_HPP_
#define ARCOS_PLATFORMS_ESP32_WROOM32S3_MODULE_PLATFORM_COMMON_HPP_

// Platform-specific HAL implementations
#include "lcd_parallel.hpp"
#include "i2s_parallel_driver.hpp"
#include "esp32_platform_impl.hpp"

/**
 * Default HAL implementations for ESP32-S3
 * 
 * These type aliases allow easy switching between LCD_CAM and I2S
 * parallel implementations. Change HAL_PARALLEL_DEFAULT to switch.
 */

// Primary parallel interface (LCD_CAM - recommended for ESP32-S3)
using HAL_PARALLEL_LCD = LcdParallel;

// Alternative parallel interface (I2S - legacy/compatibility)
using HAL_PARALLEL_I2S = I2SParallelDriver;

// Default selection (use LCD_CAM for ESP32-S3)
using HAL_PARALLEL_DEFAULT = HAL_PARALLEL_LCD;

// Platform HAL implementation
using HAL_PLATFORM = ESP32PlatformHAL;

/**
 * Platform capabilities
 */
namespace PlatformCapabilities{
  constexpr bool HAS_LCD_CAM = true;
  constexpr bool HAS_I2S_PARALLEL = true;
  constexpr bool HAS_DMA = true;
  constexpr int MAX_GPIO_PINS = 48;
  constexpr int MAX_DMA_CHANNELS = 5;
}

#endif // ARCOS_PLATFORMS_ESP32_WROOM32S3_MODULE_PLATFORM_COMMON_HPP_
