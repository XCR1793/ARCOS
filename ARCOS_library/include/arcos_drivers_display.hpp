/*****************************************************************
 * File:      arcos_drivers_display.hpp
 * Category:  drivers/display
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    ARCOS display drivers only. Include this for display hardware
 *    like HUB75 LED matrices and OLED displays without other drivers.
 *****************************************************************/

#ifndef ARCOS_DRIVERS_DISPLAY_HPP_
#define ARCOS_DRIVERS_DISPLAY_HPP_

// Include core HAL - display drivers depend on it
#include "arcos_core.hpp"

// Display drivers only
#include "abstraction/drivers/components/HUB75/driver_hub75.hpp"
#include "abstraction/drivers/components/OLED/driver_oled_sh1107.hpp"

/** 
 * @brief ARCOS Display Drivers Module
 * 
 * Provides display hardware drivers including:
 * - HUB75 RGB LED matrix drivers with I2S DMA support
 * - OLED display drivers (SH1107 and compatible)
 * - Display buffer management and pixel operations
 * 
 * Usage:
 * ```cpp
 * #include <arcos_drivers_display.hpp>
 * 
 * using namespace arcos::abstraction::drivers;
 * 
 * HUB75Driver led_matrix;
 * OLEDDriver oled_display;
 * ```
 */
namespace arcos::drivers::display {
  constexpr const char* MODULE_NAME = "ARCOS_DRIVERS_DISPLAY";
  constexpr const char* VERSION = "1.0.0";
}

#endif // ARCOS_DRIVERS_DISPLAY_HPP_