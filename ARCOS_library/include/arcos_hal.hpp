/*****************************************************************
 * File:      arcos_hal.hpp
 * Category:  hal
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    ARCOS HAL only - minimal hardware abstraction without drivers
 *    or algorithms. Use this for projects that only need basic
 *    hardware interfaces without higher-level functionality.
 *****************************************************************/

#ifndef ARCOS_HAL_HPP_
#define ARCOS_HAL_HPP_

// Core abstraction layer
#include "abstraction/hal.hpp"

// Core HAL interfaces only - no platform-specific code
#include "abstraction/core/hal_gpio_digital.hpp"
#include "abstraction/core/hal_gpio_pwm.hpp"
#include "abstraction/core/hal_system_timer.hpp"
#include "abstraction/core/hal_logging.hpp"

// Protocol abstractions
#include "abstraction/core/hal_protocal_i2c.hpp"
#include "abstraction/core/hal_protocal_spi.hpp"
#include "abstraction/core/hal_protocal_parallel.hpp"
#include "abstraction/core/hal_protocal_parallel_buffer.hpp"
#include "abstraction/core/hal_protocal_dma.hpp"
#include "abstraction/core/hal_interface_i2c.hpp"

/** 
 * @brief ARCOS HAL-Only Module
 * 
 * Provides basic hardware abstraction interfaces without platform-specific
 * implementations, drivers, or algorithms. Use this for:
 * - Custom platform implementations
 * - Minimal footprint applications
 * - HAL interface definitions only
 * 
 * Note: This module does NOT include platform-specific implementations.
 * Use arcos_core.hpp if you need platform support.
 * 
 * Usage:
 * ```cpp
 * #include <arcos_hal.hpp>
 * 
 * using namespace arcos::abstraction::core;
 * 
 * // Use HAL interfaces (requires your own platform implementation)
 * HALGPIODigital<2>::setMode(GPIO_MODE_OUTPUT);
 * ```
 */
namespace arcos::hal {
  constexpr const char* MODULE_NAME = "ARCOS_HAL";
  constexpr const char* VERSION = "1.0.0";
}

#endif // ARCOS_HAL_HPP_