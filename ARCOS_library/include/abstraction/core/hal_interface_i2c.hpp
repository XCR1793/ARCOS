/*****************************************************************
 * File:      hal_i2c_interface.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Defines the standard I2C HAL interface that all platforms
 *    must implement. This provides a clean, unified interface
 *    for device drivers without needing bridge patterns.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_CORE_HAL_INTERFACE_I2C_HPP_
#define ARCOS_ABSTRACTION_CORE_HAL_INTERFACE_I2C_HPP_

#include <stdint.h>
#include <stddef.h>

namespace arcos::abstraction{

  /** Standard result codes for HAL operations */
  enum class HalResult{
    Success = 0,        ///< Operation completed successfully
    Timeout,            ///< Operation timed out
    DeviceNotFound,     ///< I2C device did not acknowledge
    BusError,           ///< Bus error (arbitration lost, etc.)
    InvalidParameter,   ///< Invalid parameter passed
    NotInitialized,     ///< HAL not initialized
    HardwareError       ///< Hardware-specific error
  };

  /**
   * @brief Standard I2C HAL Interface
   * 
   * All platform implementations must provide these exact methods.
   * This interface provides everything device drivers need for I2C
   * communication without requiring bridge patterns.
   */
  template <typename PlatformImplementation>
  struct HalI2cInterface{
    
    /**
     * @brief Initialize I2C bus with specified configuration
     * @param bus_id I2C bus number (0, 1, etc.)
     * @param sda_pin SDA GPIO pin number
     * @param scl_pin SCL GPIO pin number  
     * @param clock_speed_hz Clock frequency in Hz (100000, 400000, etc.)
     * @param timeout_ms Default timeout for operations in milliseconds
     * @return HalResult indicating success or failure
     */
    static HalResult Initialize(uint8_t bus_id, 
                               uint8_t sda_pin, 
                               uint8_t scl_pin, 
                               uint32_t clock_speed_hz, 
                               uint32_t timeout_ms = 1000){
      return PlatformImplementation::Initialize(bus_id, sda_pin, scl_pin, clock_speed_hz, timeout_ms);
    }
    
    /**
     * @brief Deinitialize I2C bus
     * @param bus_id I2C bus number
     * @return HalResult indicating success or failure
     */
    static HalResult Deinitialize(uint8_t bus_id){
      return PlatformImplementation::Deinitialize(bus_id);
    }
    
    /**
     * @brief Write a single register value to I2C device
     * @param bus_id I2C bus number
     * @param device_address 7-bit I2C device address
     * @param register_address Register address to write
     * @param value Value to write
     * @return HalResult indicating success or failure
     */
    static HalResult WriteRegister(uint8_t bus_id,
                                  uint8_t device_address,
                                  uint8_t register_address,
                                  uint8_t value){
      return PlatformImplementation::WriteRegister(bus_id, device_address, register_address, value);
    }
    
    /**
     * @brief Read a single register value from I2C device
     * @param bus_id I2C bus number
     * @param device_address 7-bit I2C device address
     * @param register_address Register address to read
     * @param value Pointer to store read value
     * @return HalResult indicating success or failure
     */
    static HalResult ReadRegister(uint8_t bus_id,
                                 uint8_t device_address,
                                 uint8_t register_address,
                                 uint8_t* value){
      return PlatformImplementation::ReadRegister(bus_id, device_address, register_address, value);
    }
    
    /**
     * @brief Write multiple bytes to consecutive registers
     * @param bus_id I2C bus number
     * @param device_address 7-bit I2C device address
     * @param register_address Starting register address
     * @param buffer Data buffer to write
     * @param length Number of bytes to write
     * @return HalResult indicating success or failure
     */
    static HalResult WriteRegisterBuffer(uint8_t bus_id,
                                        uint8_t device_address,
                                        uint8_t register_address,
                                        const uint8_t* buffer,
                                        size_t length){
      return PlatformImplementation::WriteRegisterBuffer(bus_id, device_address, register_address, buffer, length);
    }
    
    /**
     * @brief Read multiple bytes from consecutive registers
     * @param bus_id I2C bus number
     * @param device_address 7-bit I2C device address
     * @param register_address Starting register address
     * @param buffer Buffer to store read data
     * @param length Number of bytes to read
     * @return HalResult indicating success or failure
     */
    static HalResult ReadRegisterBuffer(uint8_t bus_id,
                                       uint8_t device_address,
                                       uint8_t register_address,
                                       uint8_t* buffer,
                                       size_t length){
      return PlatformImplementation::ReadRegisterBuffer(bus_id, device_address, register_address, buffer, length);
    }
    
    /**
     * @brief Check if I2C device is present on the bus
     * @param bus_id I2C bus number
     * @param device_address 7-bit I2C device address
     * @return HalResult indicating if device responded
     */
    static HalResult ProbeDevice(uint8_t bus_id, uint8_t device_address){
      return PlatformImplementation::ProbeDevice(bus_id, device_address);
    }
    
    /**
     * @brief Set timeout for subsequent operations
     * @param bus_id I2C bus number
     * @param timeout_ms Timeout in milliseconds
     * @return HalResult indicating success or failure
     */
    static HalResult SetTimeout(uint8_t bus_id, uint32_t timeout_ms){
      return PlatformImplementation::SetTimeout(bus_id, timeout_ms);
    }

    /**
     * @brief Write raw bytes to I2C device (no register address)
     * Useful for devices that don't use register addressing (like OLEDs)
     * @param bus_id I2C bus number
     * @param device_address 7-bit I2C device address
     * @param buffer Data buffer to write
     * @param length Number of bytes to write
     * @return HalResult indicating success or failure
     */
    static HalResult WriteBytes(uint8_t bus_id,
                               uint8_t device_address,
                               const uint8_t* buffer,
                               size_t length){
      return PlatformImplementation::WriteBytes(bus_id, device_address, buffer, length);
    }
  };

  /**
   * @brief Convert HalResult to DriverResult for compatibility
   * @param hal_result HAL operation result
   * @return Corresponding DriverResult
   */
  inline int HalResultToDriverResult(HalResult hal_result){
    switch(hal_result){
      case HalResult::Success:           return 0; // DriverResult::Success
      case HalResult::Timeout:           return 1; // DriverResult::Timeout
      case HalResult::DeviceNotFound:    return 2; // DriverResult::DeviceNotFound
      case HalResult::BusError:          return 3; // DriverResult::BusError
      case HalResult::InvalidParameter:  return 4; // DriverResult::InvalidParameter
      case HalResult::NotInitialized:    return 5; // DriverResult::NotInitialized
      case HalResult::HardwareError:     return 6; // DriverResult::HardwareError
      default:                           return 6; // DriverResult::HardwareError
    }
  }

} // namespace arcos::abstraction

#endif // ARCOS_ABSTRACTION_CORE_HAL_INTERFACE_I2C_HPP_