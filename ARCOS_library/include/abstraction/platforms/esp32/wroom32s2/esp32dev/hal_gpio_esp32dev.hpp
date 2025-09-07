/*****************************************************************
 * File:      hal_gpio_esp32dev.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Implements standard digital gpio hardware abstraction for the
 *    esp32s2 dev modules.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S2_ESP32DEV_HAL_GPIO_ESP32DEV_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S2_ESP32DEV_HAL_GPIO_ESP32DEV_HPP_

#include "../../../../core/hal_gpio.hpp"
#include "soc/gpio_reg.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_reg.h"
#include "driver/gpio.h"

namespace arcos::platforms::esp32::wroom32s2{
  struct HAL_GPIO_Esp32Dev_Impl{
    /** 
     * @brief Nothing to initialise as esp32 has all gpios on one port
     */
    static inline void Initialise(){}

    /**
     * @brief Sets pins as output or input
     */
    template <uintptr_t PinNumber, arcos::abstraction::gpio::GpioHalPinMode PinMode>
    static inline void SetPin(){
      static_assert(PinNumber < 32, "PinNumber must be between 0-31 for GPIO registers for ESP32-S2 Modules");

      if constexpr(PinMode == arcos::abstraction::gpio::GpioHalPinMode::Output){
        GPIO.enable_w1ts = (1 << PinNumber);
      }else if constexpr(PinMode == arcos::abstraction::gpio::GpioHalPinMode::Input){
        GPIO.enable_w1tc = (1 << PinNumber);
      }
    }

    /**
     * @brief Sets a pin's internal resistor to float, pull up or pull down
     */
    template <uintptr_t PinNumber, arcos::abstraction::gpio::GpioHalPinPull PinPull>
    static inline void PullPin(){
      static_assert(PinNumber <= 47, "PinNumber must be between 0-47 for ESP32-S2 Modules");

      /** Calculate the IO_MUX register address for the pin */
      constexpr uintptr_t pinRegister = IO_MUX_GPIO0_REG + (PinNumber * 4);

      if constexpr(PinPull == arcos::abstraction::gpio::GpioHalPinPull::PullUp){
        REG_SET_BIT(pinRegister, FUN_PU); // Enable pull-up
        REG_CLR_BIT(pinRegister, FUN_PD); // Disable pull-down
      }else if constexpr(PinPull == arcos::abstraction::gpio::GpioHalPinPull::PullDown){
        REG_SET_BIT(pinRegister, FUN_PD); // Enable pull-down
        REG_CLR_BIT(pinRegister, FUN_PU); // Disable pull-up
      }else if constexpr(PinPull == arcos::abstraction::gpio::GpioHalPinPull::Float){
        REG_CLR_BIT(pinRegister, FUN_PU); // Disable pull-up
        REG_CLR_BIT(pinRegister, FUN_PD); // Disable pull-down
      }
    }

    /**
     * @brief Write to pin individually
     */
    template <uintptr_t PinNumber, bool State>
    static inline void WritePin(){
      static_assert(PinNumber < 32, "PinNumber must be between 0-31 for GPIO registers for ESP32-S2 Modules");

      if constexpr (State){
        GPIO.out_w1ts = (1 << PinNumber); // Set pin high
      }else{
        GPIO.out_w1tc = (1 << PinNumber); // Set pin low
      }
    }

    /**
     * @brief Write to pin in parallel
     */
    template <typename PinBus>
    static inline void WritePinParallel(uintptr_t pinValues){
      static_assert(PinBus::pinbank != 0, "PinBus mask cannot be zero");
    
      constexpr uintptr_t mask = PinBus::pinbank; // Extract mask
      uintptr_t maskedValue = pinValues & mask; // Mask runtime value so only bus pins are affected

      GPIO.out_w1tc = mask & ~maskedValue; // Clear pins that should go LOW
      GPIO.out_w1ts = maskedValue; // Set pins that should go HIGH
    }

    template <uintptr_t PinNumber>
    static inline void ReadPin(){}

    template <typename PinBus>
    static inline void ReadPinParallel(){}
  };
};

#endif // ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S2_ESP32DEV_HAL_GPIO_ESP32DEV_HPP_