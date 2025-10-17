/*****************************************************************
 * File:      arcos_drivers_sensors.hpp
 * Category:  drivers/sensors
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    ARCOS sensor drivers only. Include this for sensor hardware
 *    like IMU and environmental sensors without other drivers.
 *****************************************************************/

#ifndef ARCOS_DRIVERS_SENSORS_HPP_
#define ARCOS_DRIVERS_SENSORS_HPP_

// Include core HAL - sensor drivers depend on it
#include "arcos_core.hpp"

// Sensor drivers only
#include "abstraction/drivers/components/ICM20948/driver_icm20948.hpp"
#include "abstraction/drivers/components/BME280/driver_bme280.hpp"

/** 
 * @brief ARCOS Sensor Drivers Module
 * 
 * Provides sensor hardware drivers including:
 * - ICM20948 9-DOF IMU (accelerometer, gyroscope, magnetometer)
 * - BME280 environmental sensor (temperature, humidity, pressure)
 * - I2C and SPI communication protocol implementations
 * 
 * Usage:
 * ```cpp
 * #include <arcos_drivers_sensors.hpp>
 * 
 * using namespace arcos::abstraction::drivers;
 * 
 * ICM20948Driver imu_sensor;
 * BME280Driver env_sensor;
 * ```
 */
namespace arcos::drivers::sensors {
  constexpr const char* MODULE_NAME = "ARCOS_DRIVERS_SENSORS";
  constexpr const char* VERSION = "1.0.0";
}

#endif // ARCOS_DRIVERS_SENSORS_HPP_