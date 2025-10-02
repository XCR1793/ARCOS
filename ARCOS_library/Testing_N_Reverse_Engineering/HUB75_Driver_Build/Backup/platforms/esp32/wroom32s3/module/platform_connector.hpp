/*****************************************************************
 * File:      platform_connector.hpp
 * Category:  platform/esp32_s3
 * 
 * Purpose:
 *    Platform connector for ESP32-S3 (WROOM/Module variants)
 *    Maps HAL implementations to platform-specific drivers
 *    
 * Note:
 *    This file makes porting to ARCOS architecture easier by
 *    centralizing platform-specific type definitions.
 *****************************************************************/

#ifndef PLATFORM_ESP32_S3_CONNECTOR_HPP_
#define PLATFORM_ESP32_S3_CONNECTOR_HPP_

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

#endif // PLATFORM_ESP32_S3_CONNECTOR_HPP_
