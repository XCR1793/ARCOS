/*****************************************************************
 * File:      driver_base.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Provides base interfaces and common structures for all device
 *    drivers in the ARCOS abstraction layer. Defines protocol
 *    interfaces and device lifecycle management.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_CORE_DRIVER_BASE_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_CORE_DRIVER_BASE_HPP_

#include <stdint.h>

namespace arcos::abstraction::drivers{

  // Forward declarations
  struct I2cDeviceConfig;
  struct SpiDeviceConfig;

  /** Driver operation result codes */
  enum struct DriverResult{
    Success = 0,            ///< Operation completed successfully
    Timeout = 1,            ///< Operation timed out
    DeviceNotFound = 2,     ///< Device did not respond
    BusError = 3,           ///< Communication bus error
    InvalidParameter = 4,   ///< Invalid parameter provided
    NotInitialized = 5,     ///< Driver not initialized
    HardwareError = 6,      ///< Hardware-specific error
    ErrorInit = 7,          ///< Legacy: Initialization error (deprecated)
    ErrorCommunication = 8, ///< Legacy: Communication error (deprecated)
    ErrorNotReady = 9,      ///< Device not ready for operation
    ErrorUnknown = 10       ///< Unknown error
  };

  /** Base interface for all device drivers */
  template <typename ConcreteDriver>
  class DriverBase{
  public:
    /**
     * @brief Initialize the device driver
     * @return DriverResult indicating success or failure
     */
    DriverResult Initialize(){
      return static_cast<ConcreteDriver*>(this)->InitializeImpl();
    }

    /**
     * @brief Check if device is ready for operations
     * @return True if device is ready, false otherwise
     */
    bool IsReady() const{
      return static_cast<const ConcreteDriver*>(this)->IsReadyImpl();
    }

    /**
     * @brief Reset the device to default state
     * @return DriverResult indicating success or failure
     */
    DriverResult Reset(){
      return static_cast<ConcreteDriver*>(this)->ResetImpl();
    }

  protected:
    bool initialized_ = false;
  };

  /** SPI device configuration structure */
  struct SpiDeviceConfig{
    uint8_t cs_pin;             // Chip select pin number
    uint32_t clock_speed_hz;    // SPI clock frequency
    uint8_t mode;               // SPI mode (0-3)
    uint8_t bit_order;          // MSB/LSB first
    uint32_t timeout_ms;        // Communication timeout
  };

  /** Base interface for I2C device drivers */
  template <typename ConcreteDriver>
  class I2cDriverBase : public DriverBase<ConcreteDriver>{
  public:
    /**
     * @brief Get I2C device address from concrete driver
     * @return 7-bit I2C device address
     */
    uint8_t GetAddress() const{
      return static_cast<const ConcreteDriver*>(this)->GetDeviceAddress();
    }
  };

  /** Base interface for SPI device drivers */
  template <typename ConcreteDriver>
  class SpiDriverBase : public DriverBase<ConcreteDriver>{
  public:
    /**
     * @brief Configure SPI device parameters
     * @param config SPI device configuration
     */
    void Configure(const SpiDeviceConfig& config){
      config_ = config;
    }

    /**
     * @brief Get device chip select pin
     * @return Chip select pin number
     */
    uint8_t GetChipSelectPin() const{
      return config_.cs_pin;
    }

  protected:
    SpiDeviceConfig config_;
  };

}; // arcos::abstraction::drivers

#endif // ARCOS_ABSTRACTION_DRIVERS_CORE_DRIVER_BASE_HPP_