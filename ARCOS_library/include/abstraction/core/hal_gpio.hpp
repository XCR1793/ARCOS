/*****************************************************************
 * File:      hal_gpio.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Provides a template for GPIO HAL in both single and parallel
 *    operations of digital pins. Compile time HAL abstraction for
 *    a single port with zero runtime overhead.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_CORE_HAL_GPIO_HPP_
#define ARCOS_ABSTRACTION_CORE_HAL_GPIO_HPP_

#include <stdint.h>

namespace arcos::abstraction{
  namespace gpio{
    /** Set GPIO pins as Inputs or Outputs */
    enum struct GpioHalPinMode{
      Input = 0,
      Output = 1
    };

    /** Set GPIO pins internal pull up or downs */
    enum struct GpioHalPinPull{
      Float = 0,
      PullUp = 1,
      PullDown = 2
    };

    /** Structure for storing the GPIO pin buses */
    template <typename PinBankType>
    struct GpioHalPinAddress{
      void* port;
      PinBankType pinbank; // Can be recast if need be
    };
    /** GPIO port sizes */
    using GpioHalPinAddress8  = GpioHalPinAddress<uint8_t>;
    using GpioHalPinAddress16 = GpioHalPinAddress<uint16_t>;
    using GpioHalPinAddress32 = GpioHalPinAddress<uint32_t>;

  } // arcos hardware abstraction layer namespace specifically gpio

  template <typename  PlatformImplementation,
            uintptr_t PinNumber,
            typename  PinBusType,
            gpio::GpioHalPinMode PinMode,
            gpio::GpioHalPinPull PinPull>
  struct HalGpio{
    /**
     * @brief Initialises GPIO Pins & Bus and flattens pin registration
     *        and access instead of having to specify ports.
     */
    static inline void Initialise(){
      PlatformImplementation::Initialise();
    }
    
    /**
     * @brief Sets Pins as input or output
     */
    static inline void SetPin(){
      PlatformImplementation::template SetPin<PinNumber, PinMode>();
    }

    /**
     * @brief Sets Pull direction for pins
     */
    static inline void PullPin(){
      PlatformImplementation::template PullPin<PinNumber, PinPull>();
    }

    /**
     * @brief Writes a pin as high or low
     */
    template <bool State>
    static inline void WritePin(){
      PlatformImplementation::template WritePin<PinNumber, State>();
    }

    /**
     * @brief Writes a set of pins in parallel
     */
    static inline void WritePinParallel(uintptr_t pinValues){
      PlatformImplementation::template WritePinParallel<gpio::GpioHalPinAddress<PinBusType>>(pinValues);
    }

    /**
     * @brief Reads a pin as high or low
     */
    static inline bool ReadPin(){
      return PlatformImplementation::template ReadPin<PinNumber>();
    }

    /**
     * @brief Reads a set of pins in parallel
     */
    static inline bool ReadPinParallel(){
      return PlatformImplementation::template ReadPinParallel<gpio::GpioHalPinAddress<PinBusType>>();
    }
  };
}

#endif // ARCOS_ABSTRACTION_CORE_HAL_GPIO_HPP_