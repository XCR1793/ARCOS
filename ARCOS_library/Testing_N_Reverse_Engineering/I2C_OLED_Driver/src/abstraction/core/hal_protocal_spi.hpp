/*****************************************************************
 * File:      hal_spi.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Provides a template for SPI HAL abstraction for single or 
 *    multiple SPI buses. Compile time HAL abstraction with zero 
 *    runtime overhead for configuration and data transfers.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_CORE_HAL_PROTOCAL_SPI_HPP_
#define ARCOS_ABSTRACTION_CORE_HAL_PROTOCAL_SPI_HPP_

#include <stdint.h>

namespace arcos::abstraction{
  namespace spi{
    /** SPI modes */
    enum struct SpiMode{
      Mode0 = 0, // CPOL = 0, CPHA = 0
      Mode1 = 1, // CPOL = 0, CPHA = 1
      Mode2 = 2, // CPOL = 1, CPHA = 0
      Mode3 = 3  // CPOL = 1, CPHA = 1
    };

    /** SPI clock polarity */
    enum struct SpiPolarity{
      Low  = 0,
      High = 1
    };

    /** SPI clock phase */
    enum struct SpiPhase{
      Leading  = 0,
      Trailing = 1
    };

    /** Generic SPI bus reference */
    template <typename RegisterType>
    struct SpiHalBus {
      volatile RegisterType* base;  // strongly typed, volatile for hardware
    };

    /** SPI bus + configuration mapping */
    template <typename BusType, typename RegisterType>
    struct SpiHalConfig {
      SpiHalBus<RegisterType> bus; // pointer to the hardware registers
      BusType busId;               // bus identifier
      SpiMode mode;                // SPI mode
      uint32_t speedHz;            // SPI speed in Hz
    };

  } // namespace spi

  template <typename PlatformImplementation,
            uintptr_t BusNumber,
            typename BusType,
            typename RegisterType,
            spi::SpiMode Mode,
            uint32_t SpeedHz>
  struct HalSpi{
    /**
     * @brief Initialises SPI bus with compile-time configuration
     */
    static inline void Initialise(){
      PlatformImplementation::template Initialise<BusNumber, Mode, SpeedHz>();
    }

    /**
     * @brief Transmits a single byte
     */
    template <uint8_t Data>
    static inline void Transmit(){
      PlatformImplementation::template Transmit<BusNumber, Data>();
    }

    /**
     * @brief Receives a single byte
     */
    static inline uint8_t Receive(){
      return PlatformImplementation::template Receive<BusNumber>();
    }

    /**
     * @brief Transmits and receives a single byte (full-duplex)
     */
    template <uint8_t Data>
    static inline uint8_t Transfer(){
      return PlatformImplementation::template Transfer<BusNumber, Data>();
    }

    /**
     * @brief Transmits multiple bytes (runtime)
     * @param data Pointer to data buffer
     * @param length Number of bytes to transmit
     */
    static inline void Transmit(uint8_t* data, uintptr_t length){
      PlatformImplementation::template Transmit<BusNumber>(data, length);
    }

    /**
     * @brief Receives multiple bytes (runtime)
     * @param buffer Pointer to receive buffer
     * @param length Number of bytes to receive
     */
    static inline void Receive(uint8_t* buffer, uintptr_t length){
      PlatformImplementation::template Receive<BusNumber>(buffer, length);
    }

    /**
     * @brief Transfers multiple bytes full-duplex (runtime)
     * @param txBuffer Pointer to transmit buffer
     * @param rxBuffer Pointer to receive buffer
     * @param length Number of bytes
     */
    static inline void Transfer(uint8_t* txBuffer, uint8_t* rxBuffer, uintptr_t length){
      PlatformImplementation::template Transfer<BusNumber>(txBuffer, rxBuffer, length);
    }

    /**
     * @brief Sets SPI mode at runtime
     * @param mode SPI mode
     */
    static inline void SetMode(spi::SpiMode mode){
      PlatformImplementation::template SetMode<BusNumber>(mode);
    }

    /**
     * @brief Sets SPI speed at runtime
     * @param speedHz SPI clock frequency
     */
    static inline void SetSpeed(uint32_t speedHz){
      PlatformImplementation::template SetSpeed<BusNumber>(speedHz);
    }
  };
}

#endif // ARCOS_ABSTRACTION_CORE_HAL_PROTOCAL_SPI_HPP_