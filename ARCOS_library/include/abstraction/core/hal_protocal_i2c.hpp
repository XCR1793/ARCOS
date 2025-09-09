/*****************************************************************
 * File:      hal_protocal_i2c.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Provides a template for I2C HAL for both single-byte and
 *    multi-byte transfers. Compile-time HAL abstraction for a
 *    single bus with zero runtime overhead where possible.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_CORE_HAL_PROTOCAL_I2C_HPP_
#define ARCOS_ABSTRACTION_CORE_HAL_PROTOCAL_I2C_HPP_

#include <stdint.h>

namespace arcos::abstraction{
  namespace i2c{
    /** I2C speed modes */
    enum struct I2CHalSpeed{
      Standard  = 100000,  // 100 kHz
      Fast      = 400000,  // 400 kHz
      FastPlus  = 1000000, // 1 MHz (if supported)
      Custom    = 0        // user-defined
    };

    /** I2C bus reference template */
    template <typename RegisterType>
    struct I2CHalBus {
      volatile RegisterType* base;  // strongly typed, volatile for hardware
    };

    /** I2C device address structure */
    template <typename BusType, typename RegisterType>
    struct I2CHalAddress {
      I2CHalBus<RegisterType> bus; // pointer to the I2C peripheral
      uint8_t addr;                 // 7-bit or 10-bit address
    };

    /** Type aliases for common widths */
    using I2CHalAddress8  = I2CHalAddress<uint8_t,  uint8_t>;
    using I2CHalAddress16 = I2CHalAddress<uint16_t, uint32_t>; // STM32 32-bit regs
    using I2CHalAddress32 = I2CHalAddress<uint32_t, uint32_t>;
  } // namespace i2c

  template <typename PlatformImplementation,
            uintptr_t BusNumber,
            typename BusType,
            typename RegisterType,
            i2c::I2CHalSpeed Speed>
  struct HalI2C{
    /**
     * @brief Initialise the I2C bus
     */
    static inline void Initialise(){
      PlatformImplementation::Initialise();
    }

    /**
     * @brief Write a single byte to a device
     */
    static inline bool WriteByte(uint8_t deviceAddr, uint8_t data){
      return PlatformImplementation::template WriteByte<BusNumber>(deviceAddr, data);
    }

    /**
     * @brief Write multiple bytes to a device
     */
    static inline bool WriteBytes(uint8_t deviceAddr, const uint8_t* data, uintptr_t length){
      return PlatformImplementation::template WriteBytes<BusNumber>(deviceAddr, data, length);
    }

    /**
     * @brief Read a single byte from a device
     */
    static inline bool ReadByte(uint8_t deviceAddr, uint8_t& data){
      return PlatformImplementation::template ReadByte<BusNumber>(deviceAddr, data);
    }

    /**
     * @brief Read multiple bytes from a device
     */
    static inline bool ReadBytes(uint8_t deviceAddr, uint8_t* buffer, uintptr_t length){
      return PlatformImplementation::template ReadBytes<BusNumber>(deviceAddr, buffer, length);
    }

    /**
     * @brief Non-blocking write with runtime configuration
     * @param busNumber   I2C bus number
     * @param deviceAddr  Device address
     * @param data        Pointer to data
     * @param length      Number of bytes
     * @return true if success, false if fail or bus busy
     */
    static inline bool WriteBytes(uintptr_t busNumber, uint8_t deviceAddr, const uint8_t* data, uintptr_t length){
      return PlatformImplementation::WriteBytes(busNumber, deviceAddr, data, length);
    }

    /**
     * @brief Non-blocking read with runtime configuration
     * @param busNumber   I2C bus number
     * @param deviceAddr  Device address
     * @param buffer      Pointer to receive buffer
     * @param length      Number of bytes
     * @return true if success, false if fail or bus busy
     */
    static inline bool ReadBytes(uintptr_t busNumber, uint8_t deviceAddr, uint8_t* buffer, uintptr_t length){
      return PlatformImplementation::ReadBytes(busNumber, deviceAddr, buffer, length);
    }

    /**
     * @brief Set the I2C speed at runtime
     */
    static inline void SetSpeed(i2c::I2CHalSpeed speed){
      PlatformImplementation::template SetSpeed<BusNumber>(speed);
    }

    /**
     * @brief Reset the bus if stuck
     */
    static inline void ResetBus(){
      PlatformImplementation::template ResetBus<BusNumber>();
    }
  };
}

#endif // ARCOS_ABSTRACTION_CORE_HAL_PROTOCAL_I2C_HPP_