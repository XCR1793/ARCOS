/*****************************************************************
 * File:      spi_device_interface.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Provides SPI device communication interface using ARCOS HAL
 *    abstraction. Handles SPI transactions and chip select
 *    management for SPI-based device drivers.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_CORE_SPI_DEVICE_INTERFACE_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_CORE_SPI_DEVICE_INTERFACE_HPP_

#include "../../core/hal_protocal_spi.hpp"

namespace arcos::abstraction::drivers{

  /** SPI communication interface for device drivers */
  template <typename HalSpiImplementation>
  class SpiDeviceInterface{
  public:
    /**
     * @brief Initialize SPI interface with HAL implementation
     * @param cs_pin Chip select pin number
     * @param bus_config SPI bus configuration
     */
    SpiDeviceInterface(uint8_t cs_pin, const SpiDeviceConfig& bus_config)
      : cs_pin_(cs_pin), config_(bus_config), initialized_(false){
    }

    /**
     * @brief Initialize SPI bus using HAL abstraction
     * @return DriverResult indicating success or failure
     */
    DriverResult Initialize(){
      // Initialize SPI HAL with configuration
      auto result = HalSpiImplementation::Initialize(config_.clock_speed_hz, config_.mode, config_.bit_order);
      if(result != DriverResult::Success) return result;

      // Configure chip select pin as output
      // TODO: Use HAL GPIO abstraction for CS pin configuration
      
      initialized_ = true;
      return DriverResult::Success;
    }

    /**
     * @brief Perform SPI transaction with chip select management
     * @param tx_data Pointer to transmit data buffer (can be nullptr for read-only)
     * @param rx_data Pointer to receive data buffer (can be nullptr for write-only)
     * @param length Number of bytes to transfer
     * @return DriverResult indicating success or failure
     */
    DriverResult Transaction(const uint8_t* tx_data, uint8_t* rx_data, uint16_t length){
      if(!initialized_) return DriverResult::ErrorNotReady;
      if(length == 0) return DriverResult::InvalidParameter;

      // Assert chip select (active low)
      SetChipSelect(false);

      // Perform SPI transfer
      DriverResult result = HalSpiImplementation::TransferFullDuplex(tx_data, rx_data, length, config_.timeout_ms);

      // Deassert chip select
      SetChipSelect(true);

      return result;
    }

    /**
     * @brief Write data to SPI device
     * @param data Pointer to data buffer
     * @param length Number of bytes to write
     * @return DriverResult indicating success or failure
     */
    DriverResult Write(const uint8_t* data, uint16_t length){
      return Transaction(data, nullptr, length);
    }

    /**
     * @brief Read data from SPI device
     * @param data Pointer to data buffer
     * @param length Number of bytes to read
     * @return DriverResult indicating success or failure
     */
    DriverResult Read(uint8_t* data, uint16_t length){
      return Transaction(nullptr, data, length);
    }

    /**
     * @brief Write command and read response
     * @param command Command byte to write
     * @param response_data Pointer to response buffer
     * @param response_length Number of response bytes to read
     * @return DriverResult indicating success or failure
     */
    DriverResult WriteCommand(uint8_t command, uint8_t* response_data, uint16_t response_length){
      if(!initialized_) return DriverResult::ErrorNotReady;

      // Assert chip select
      SetChipSelect(false);

      // Write command
      DriverResult result = HalSpiImplementation::TransferFullDuplex(&command, nullptr, 1, config_.timeout_ms);
      if(result != DriverResult::Success){
        SetChipSelect(true);
        return result;
      }

      // Read response if requested
      if(response_data && response_length > 0){
        result = HalSpiImplementation::TransferFullDuplex(nullptr, response_data, response_length, config_.timeout_ms);
      }

      // Deassert chip select
      SetChipSelect(true);
      return result;
    }

    /**
     * @brief Get chip select pin number
     * @return Chip select pin number
     */
    uint8_t GetChipSelectPin() const{
      return cs_pin_;
    }

    /**
     * @brief Check if interface is initialized
     * @return True if initialized, false otherwise
     */
    bool IsInitialized() const{
      return initialized_;
    }

  private:
    uint8_t cs_pin_;
    SpiDeviceConfig config_;
    bool initialized_;

    /**
     * @brief Control chip select pin state
     * @param state True for high (deasserted), false for low (asserted)
     */
    void SetChipSelect(bool state){
      // TODO: Use HAL GPIO abstraction to control CS pin
      // HalGpioImplementation::WritePin(cs_pin_, state);
    }
  };

}; // arcos::abstraction::drivers

#endif // ARCOS_ABSTRACTION_DRIVERS_CORE_SPI_DEVICE_INTERFACE_HPP_