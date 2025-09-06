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

    /** Set GPIO pin buses (port) as single or parallel driven */
    enum struct GpioHalPortMode{
      Single = 0,
      Parallel = 1
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

  template <typename  PlatformImplentation,
                      gpio::GpioHalPortMode PortMode,
                      gpio::GpioHalPinMode PinMode>
  class HalGpio{
    public:
      constexpr HalGpio() noexcept = default;
      ~HalGpio() = default;

      /**
       * @brief Initialises GPIO Pins & Bus
       */
      static void Init(PlatformImplentation& impl){
        impl.hal_gpio_initialisation();
      };
      
      /**
       * @brief Sets Pins as input or output
       */

      /**
       * @brief Sets Pull direction for pins
       */

      /**
       * @brief Sets Port mode (single or parallel)
       */

      /**
       * @brief Writes a pin as high or low
       */

      /**
       * @brief Writes a set of pins in parallel
       */

      /**
       * @brief Reads a pin as high or low
       */

      /**
       * @brief Reads a set of pins in parallel
       */
  };
}

#endif // ARCOS_ABSTRACTION_CORE_HAL_GPIO_HPP_