/*****************************************************************
 * File:      hal_connector.hpp
 * Category:  abstraction/platforms/esp32/wroom32s3/module
 * 
 * Purpose:
 *    HAL connector for ESP32-S3 WROOM32/Module
 *    Maps HAL protocol implementations to platform-specific drivers
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_CONNECTOR_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_CONNECTOR_HPP_

// Platform-specific HAL implementations (headers include their implementations)
#include "lcd_parallel.hpp"
#include "i2s_parallel_driver.hpp"
#include "esp32_platform_impl.hpp"

// Platform implementation functions
#include "esp32_platform_functions_impl.hpp"

// DMA buffer manager implementation
#include "parallel_buffer_impl.hpp"

namespace arcos::abstraction{

/**
 * Default HAL implementations for ESP32-S3 WROOM32/Module
 * 
 * These type aliases allow the abstraction layer to use
 * platform-specific implementations without knowing the details
 */

// Parallel protocol implementations
using HAL_PARALLEL_LCD = LcdParallel;
using HAL_PARALLEL_I2S = I2sParallelDriver;

// Default parallel implementation (LCD_CAM is faster on ESP32-S3)
using HAL_PARALLEL_DEFAULT = HAL_PARALLEL_LCD;

// Platform HAL implementation
using HAL_PLATFORM_DEFAULT = ESP32PlatformHAL;

} // namespace arcos::abstraction

#endif // ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_CONNECTOR_HPP_ // HUB75_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_CONNECTOR_HPP_
