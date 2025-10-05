/*****************************************************************
 * File:      hal_gpio_digital.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Provides a template for GPIO HAL in both single and parallel
 *    operations of digital pins. Compile time HAL abstraction for
 *    a single port with zero runtime overhead.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_CORE_HAL_GPIO_DIGITAL_HPP_
#define ARCOS_ABSTRACTION_CORE_HAL_GPIO_DIGITAL_HPP_

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

    /** Generic GPIO port reference */
    template <typename RegisterType>
    struct GpioHalPort {
      volatile RegisterType* base;  // strongly typed, volatile for hardware
    };

    /** Pin + port mapping */
    template <typename PinBankType, typename RegisterType>
    struct GpioHalPinAddress {
      GpioHalPort<RegisterType> port; // pointer to the hardware registers
      PinBankType pinbank;            // mask for pins in this port
    };

    /** Type aliases for common widths */
    using GpioHalPinAddress8  = GpioHalPinAddress<uint8_t,  uint8_t>;
    using GpioHalPinAddress16 = GpioHalPinAddress<uint16_t, uint32_t>; // STM32 uses 32-bit GPIO regs
    using GpioHalPinAddress32 = GpioHalPinAddress<uint32_t, uint32_t>;

  } // arcos hardware abstraction layer namespace specifically gpio

  template <typename  PlatformImplementation,
            uintptr_t PinNumber,
            typename  PinBusType,
            typename  RegisterType,
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
      PlatformImplementation::template WritePinParallel<gpio::GpioHalPinAddress<PinBusType, RegisterType>>(pinValues);
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
    static inline uintptr_t ReadPinParallel(){
      return PlatformImplementation::template ReadPinParallel<gpio::GpioHalPinAddress<PinBusType, RegisterType>>();
    }

    /**
     * @brief Writes a pin as high or low using lower level faster implementation
     */
    template <bool State>
    static inline void FastWritePin(){
      PlatformImplementation::template FastWritePin<PinNumber, State>();
    }

    /**
     * @brief Writes a set of pins in parallel using lower level faster implementation
     */
    static inline void FastWritePinParallel(uintptr_t pinValues){
      PlatformImplementation::template WritePinParallel<gpio::GpioHalPinAddress<PinBusType, RegisterType>>(pinValues);
    }


    /**
     * @brief Reads a pin as high or low using lower level faster implementation
     */
    static inline bool FastReadPin(){
      return PlatformImplementation::template ReadPin<PinNumber>();
    }

    /**
     * @brief Reads a set of pins in parallel using lower level faster implementation
     */
    static inline uintptr_t FastReadPinParallel(){
      return PlatformImplementation::template ReadPinParallel<gpio::GpioHalPinAddress<PinBusType, RegisterType>>();
    }

    /**
     * @brief Sets pin as input or output (runtime).
     * @param pin   Pin number at runtime
     * @param mode  Pin mode at runtime
     * @note Runtime variant of SetPin().
     */
    static inline void SetPin(uintptr_t pin, gpio::GpioHalPinMode mode, gpio::GpioHalPinPull pull);

    /**
     * @brief Sets pull direction for pin (runtime).
     * @param pin   Pin number at runtime
     * @param pull  Pin pull configuration at runtime
     * @note Runtime variant of PullPin().
     */
    static inline void PullPin(uintptr_t pin, gpio::GpioHalPinPull pull);

    /**
     * @brief Writes a pin as high or low (runtime).
     * @param pin    Pin number at runtime
     * @param state  True for HIGH, false for LOW
     * @note Runtime variant of WritePin().
     */
    static inline void WritePin(uintptr_t pin, bool state);

    /**
     * @brief Writes a set of pins in parallel (runtime).
     * @param pinMask    Mask of pins to write
     * @param pinValues  Bit values corresponding to each pin
     * @note Runtime variant of WritePinParallel().
     */
    static inline void WritePinParallel(uintptr_t pinMask, uintptr_t pinValues);

    /**
     * @brief Reads a pin as high or low (runtime).
     * @param pin  Pin number at runtime
     * @return True if HIGH, false if LOW
     * @note Runtime variant of ReadPin().
     */
    static inline bool ReadPin(uintptr_t pin);

    /**
     * @brief Reads a set of pins in parallel (runtime).
     * @param pinMask  Mask of pins to read
     * @return Bit values of the read pins
     * @note Runtime variant of ReadPinParallel().
     */
    static inline uintptr_t ReadPinParallel(uintptr_t pinMask);

    /**
     * @brief Writes a pin using a faster, lower-level implementation (runtime).
     * @param pin    Pin number at runtime
     * @param state  True for HIGH, false for LOW
     * @note Runtime variant of FastWritePin().
     */
    static inline void FastWritePin(uintptr_t pin, bool state);

    /**
     * @brief Writes a set of pins in parallel using a faster implementation (runtime).
     * @param pinMask    Mask of pins to write
     * @param pinValues  Bit values corresponding to each pin
     * @note Runtime variant of FastWritePinParallel().
     */
    static inline void FastWritePinParallel(uintptr_t pinMask, uintptr_t pinValues);

    /**
     * @brief Reads a pin using a faster, lower-level implementation (runtime).
     * @param pin  Pin number at runtime
     * @return True if HIGH, false if LOW
     * @note Runtime variant of FastReadPin().
     */
    static inline bool FastReadPin(uintptr_t pin);

    /**
     * @brief Reads a set of pins in parallel using a faster implementation (runtime).
     * @param pinMask  Mask of pins to read
     * @return Bit values of the read pins
     * @note Runtime variant of FastReadPinParallel().
     */
    static inline uintptr_t FastReadPinParallel(uintptr_t pinMask);
  };
}

#endif // ARCOS_ABSTRACTION_CORE_HAL_GPIO_DIGITAL_HPP_