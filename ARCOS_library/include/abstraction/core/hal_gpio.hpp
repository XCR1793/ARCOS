/**
 * File:      hal_gpio.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Provides a template for GPIO HAL in both single and parallel
 *    operations of digital pins.
 */

#ifndef ARCOS_ABSTRACTION_CORE_HAL_GPIO_HPP_
#define ARCOS_ABSTRACTION_CORE_HAL_GPIO_HPP_

namespace arcos::hal::gpio{
  enum struct GpioHalPinMode{
    Input = 0,
    Output = 1
  };

  enum struct GpioHalPinPull{
    Float = 0,
    PullUp = 1,
    PullDown = 2
  };

  enum struct GpioHalPortMode{
    Single = 0,
    Parallel = 1
  };

  struct GpioHalPinAddress{
    void* port;
    uint32_t mask; // Can be recast if need be
  };
} // arcos hardware abstraction layer namespace specifically gpio

#endif // ARCOS_ABSTRACTION_CORE_HAL_GPIO_HPP_