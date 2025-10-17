/*****************************************************************
 * File:      arcos_drivers.hpp
 * Category:  drivers
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    ARCOS hardware drivers module. Include this for hardware
 *    component drivers like HUB75, ICM20948, BME280, etc.
 *    This module includes core HAL dependencies automatically.
 *****************************************************************/

#ifndef ARCOS_DRIVERS_HPP_
#define ARCOS_DRIVERS_HPP_

// Include core HAL - drivers depend on it
#include "arcos_core.hpp"

// Display drivers
#include "abstraction/drivers/components/HUB75/driver_hub75.hpp"
#include "abstraction/drivers/components/OLED/driver_oled_sh1107.hpp"

// Sensor drivers
#include "abstraction/drivers/components/ICM20948/driver_icm20948.hpp"
#include "abstraction/drivers/components/BME280/driver_bme280.hpp"

// Storage drivers
#include "abstraction/drivers/components/SD_CARD/driver_sd_card.hpp"

/** 
 * @brief ARCOS Drivers Module
 * 
 * Provides hardware component drivers including:
 * - Display drivers (HUB75 LED matrices, OLED displays)
 * - Sensor drivers (ICM20948 IMU, BME280 environmental)
 * - Storage drivers (SD card file system)
 * - Communication protocol implementations
 * 
 * Note: This module automatically includes arcos_core.hpp as drivers
 * depend on HAL abstractions.
 * 
 * Usage:
 * ```cpp
 * #include <arcos_drivers.hpp>
 * 
 * using namespace arcos::abstraction::drivers;
 * 
 * // Use hardware drivers
 * HUB75Driver display;
 * ICM20948Driver imu_sensor;
 * ```
 */
namespace arcos::drivers {
  constexpr const char* MODULE_NAME = "ARCOS_DRIVERS";
  constexpr const char* VERSION = "1.0.0";
}

#endif // ARCOS_DRIVERS_HPP_