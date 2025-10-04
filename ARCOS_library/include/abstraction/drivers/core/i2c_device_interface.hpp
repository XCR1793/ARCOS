/*****************************************************************
 * File:      i2c_device_interface.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Provides I2C device communication interface using ARCOS HAL
 *    abstraction. Handles register read/write operations and bus
 *    management for I2C-based device drivers.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_CORE_I2C_DEVICE_INTERFACE_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_CORE_I2C_DEVICE_INTERFACE_HPP_

#include "../../core/hal_interface_i2c.hpp"

namespace arcos::abstraction::drivers{

  /** I2C device configuration */
  struct I2cDeviceConfig{
    uint8_t device_address;     ///< 7-bit I2C device address
    uint8_t bus_id;             ///< I2C bus number (0, 1, etc.)
    uint8_t sda_pin;            ///< SDA GPIO pin number
    uint8_t scl_pin;            ///< SCL GPIO pin number
    uint32_t clock_speed_hz;    ///< I2C clock frequency in Hz
    uint32_t timeout_ms;        ///< Timeout for operations in ms
    bool pullup_enable;         ///< Enable internal pullups (deprecated, handled by HAL)
  };

  /** I2C communication interface for device drivers */
  template <typename HalI2cImplementation>
  class I2cDeviceInterface{
  public:
    /**
     * @brief Initialize I2C interface with HAL implementation
     * @param device_address 7-bit I2C device address
     * @param bus_config I2C bus configuration
     */
    I2cDeviceInterface(uint8_t device_address, const I2cDeviceConfig& bus_config)
      : device_address_(device_address), config_(bus_config), initialized_(false){
    }

    /**
     * @brief Initialize I2C bus using HAL abstraction
     * @return DriverResult indicating success or failure
     */
    DriverResult Initialize(){
      // Initialize I2C HAL with configuration
      HalResult result = HalI2cImplementation::Initialize(
        config_.bus_id, 
        config_.sda_pin, 
        config_.scl_pin, 
        config_.clock_speed_hz, 
        config_.timeout_ms
      );
      
      initialized_ = (result == HalResult::Success);
      return static_cast<DriverResult>(HalResultToDriverResult(result));
    }

    /**
     * @brief Write a single register value
     * @param register_address Register address to write
     * @param value Value to write to register
     * @return DriverResult indicating success or failure
     */
    DriverResult WriteRegister(uint8_t register_address, uint8_t value){
      if(!initialized_) return DriverResult::ErrorNotReady;
      
      HalResult result = HalI2cImplementation::WriteRegister(
        config_.bus_id, device_address_, register_address, value
      );
      return static_cast<DriverResult>(HalResultToDriverResult(result));
    }

    /**
     * @brief Write multiple bytes to consecutive registers
     * @param register_address Starting register address
     * @param data Pointer to data buffer
     * @param length Number of bytes to write
     * @return DriverResult indicating success or failure
     */
    DriverResult WriteRegisters(uint8_t register_address, const uint8_t* data, uint16_t length){
      if(!initialized_) return DriverResult::ErrorNotReady;
      if(!data || length == 0) return DriverResult::InvalidParameter;

      HalResult result = HalI2cImplementation::WriteRegisterBuffer(
        config_.bus_id, device_address_, register_address, data, length
      );
      return static_cast<DriverResult>(HalResultToDriverResult(result));
    }

    /**
     * @brief Read a single register value
     * @param register_address Register address to read
     * @param value Reference to store read value
     * @return DriverResult indicating success or failure
     */
    DriverResult ReadRegister(uint8_t register_address, uint8_t& value){
      if(!initialized_) return DriverResult::ErrorNotReady;

      HalResult result = HalI2cImplementation::ReadRegister(
        config_.bus_id, device_address_, register_address, &value
      );
      return static_cast<DriverResult>(HalResultToDriverResult(result));
    }

    /**
     * @brief Read multiple bytes from consecutive registers
     * @param register_address Starting register address
     * @param data Pointer to data buffer
     * @param length Number of bytes to read
     * @return DriverResult indicating success or failure
     */
    DriverResult ReadRegisters(uint8_t register_address, uint8_t* data, uint16_t length){
      if(!initialized_) return DriverResult::ErrorNotReady;
      if(!data || length == 0) return DriverResult::InvalidParameter;

      HalResult result = HalI2cImplementation::ReadRegisterBuffer(
        config_.bus_id, device_address_, register_address, data, length
      );
      return static_cast<DriverResult>(HalResultToDriverResult(result));
    }

    /**
     * @brief Check if device responds on I2C bus
     * @return True if device responds, false otherwise
     */
    bool IsDevicePresent(){
      if(!initialized_) return false;
      
      HalResult result = HalI2cImplementation::ProbeDevice(config_.bus_id, device_address_);
      return (result == HalResult::Success);
    }

    /**
     * @brief Get device I2C address
     * @return 7-bit I2C device address
     */
    uint8_t GetDeviceAddress() const{
      return device_address_;
    }

    /**
     * @brief Check if interface is initialized
     * @return True if initialized, false otherwise
     */
    bool IsInitialized() const{
      return initialized_;
    }

  private:
    uint8_t device_address_;
    I2cDeviceConfig config_;
    bool initialized_;
  };

}; // arcos::abstraction::drivers

#endif // ARCOS_ABSTRACTION_DRIVERS_CORE_I2C_DEVICE_INTERFACE_HPP_