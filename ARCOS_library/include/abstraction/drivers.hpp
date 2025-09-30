/*****************************************************************
 * File:      drivers.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Main header file for ARCOS driver abstraction system.
 *    Includes all driver interfaces and provides unified access
 *    to device drivers using HAL abstraction.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_HPP_

// Core driver abstractions
#include "drivers/core/driver_base.hpp"
#include "drivers/core/i2c_device_interface.hpp"
#include "drivers/core/spi_device_interface.hpp"
#include "drivers/core/driver_manager.hpp"

// Device drivers
#include "drivers/ICM20948/icm20948_driver.hpp"
#include "drivers/BME280/bme280_driver.hpp"

// HAL abstraction (ensure HAL is included)
#include "hal.hpp"

namespace arcos::abstraction::drivers{

  /** 
   * @brief Type aliases for common driver configurations
   * 
   * These aliases provide convenient access to driver types with
   * the default HAL implementations selected in hal.hpp
   */
  
  // I2C device drivers using default HAL I2C implementation
  using ICM20948 = ICM20948Driver<HAL_I2C_DEFAULT>;
  using BME280 = BME280Driver<HAL_I2C_DEFAULT>;
  
  // Driver manager using default HAL implementations  
  using SensorManager = DriverManager<HAL_I2C_DEFAULT>;

  /**
   * @brief Quick initialization helper function
   * @return Pointer to configured driver manager
   */
  inline SensorManager* CreateSensorManager(){
    return new SensorManager();
  }

  /**
   * @brief Quick ICM20948 creation helper
   * @param manager Driver manager instance
   * @param address I2C device address (default: 0x68)
   * @return Pointer to ICM20948 driver or nullptr on failure
   */
  inline ICM20948* CreateICM20948(SensorManager* manager, uint8_t address = 0x68){
    if(!manager) return nullptr;
    return manager->CreateICM20948(address);
  }

  /**
   * @brief Quick BME280 creation helper
   * @param manager Driver manager instance  
   * @param address I2C device address (default: 0x76)
   * @return Pointer to BME280 driver or nullptr on failure
   */
  inline BME280* CreateBME280(SensorManager* manager, uint8_t address = 0x76){
    if(!manager) return nullptr;
    return manager->CreateBME280(address);
  }

  /**
   * @brief Initialize all sensors in manager
   * @param manager Driver manager instance
   * @return DriverResult indicating success or failure
   */
  inline DriverResult InitializeAllSensors(SensorManager* manager){
    if(!manager) return DriverResult::InvalidParameter;
    return manager->InitializeAllDrivers();
  }

}; // arcos::abstraction::drivers

/** 
 * @brief Convenience macros for quick sensor setup
 * 
 * Example usage:
 * 
 * void main(){
 *   ARCOS_INIT_SENSOR_MANAGER();
 *   ARCOS_CREATE_ICM20948(imu, 0x68);
 *   ARCOS_CREATE_BME280(env, 0x76);
 *   ARCOS_INIT_ALL_SENSORS();
 *   
 *   while(true){
 *     ImuData imu_data;
 *     EnvironmentalData env_data;
 *     imu->ReadSensorData(imu_data);
 *     env->ReadSensorData(env_data);
 *   }
 * }
 */

#define ARCOS_INIT_SENSOR_MANAGER() \
  auto* sensor_manager = arcos::abstraction::drivers::CreateSensorManager()

#define ARCOS_CREATE_ICM20948(var_name, address) \
  auto* var_name = arcos::abstraction::drivers::CreateICM20948(sensor_manager, address)

#define ARCOS_CREATE_BME280(var_name, address) \
  auto* var_name = arcos::abstraction::drivers::CreateBME280(sensor_manager, address)

#define ARCOS_INIT_ALL_SENSORS() \
  arcos::abstraction::drivers::InitializeAllSensors(sensor_manager)

#endif // ARCOS_ABSTRACTION_DRIVERS_HPP_