/*****************************************************************
 * File:      hal_digital_gpio_module.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Implements standard digital gpio hardware abstraction for the
 *    esp32s3 modules.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_DIGITAL_GPIO_MODULE_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_DIGITAL_GPIO_MODULE_HPP_

#include "../../../../core/hal_digital_GPIO.hpp"
#include "driver/gpio.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_reg.h"

namespace arcos::platforms::esp32::wroom32s3{
  struct HAL_DIGITAL_GPIO_Module_Impl{
    /** 
     * @brief Nothing to initialise as esp32 has all gpios on one port
     */
    static inline void Initialise(){}

    /**
     * @brief Sets pins as output or input using ESP-IDF API
     */
    template <uintptr_t PinNumber, arcos::abstraction::gpio::GpioHalPinMode PinMode>
    static inline void SetPin(){
      static_assert(PinNumber <= 48, "PinNumber must be between 0-45 for ESP32-S3");
      gpio_config_t cfg{};
      cfg.pin_bit_mask = (1ULL << PinNumber);
      cfg.mode = (PinMode == arcos::abstraction::gpio::GpioHalPinMode::Output) ? 
                  GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
      cfg.pull_up_en = GPIO_PULLUP_DISABLE;
      cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
      cfg.intr_type = GPIO_INTR_DISABLE;
      gpio_config(&cfg);
    }

    /**
     * @brief Sets a pin's internal resistor to float, pull up or pull down using ESP-IDF API
     */
    template <uintptr_t PinNumber, arcos::abstraction::gpio::GpioHalPinPull PinPull>
    static inline void PullPin(){
      static_assert(PinNumber <= 48, "PinNumber must be between 0-45 for ESP32-S3");

      if constexpr(PinPull == arcos::abstraction::gpio::GpioHalPinPull::PullUp){
        gpio_set_pull_mode(static_cast<gpio_num_t>(PinNumber), GPIO_PULLUP_ONLY);
      }else if constexpr(PinPull == arcos::abstraction::gpio::GpioHalPinPull::PullDown){
        gpio_set_pull_mode(static_cast<gpio_num_t>(PinNumber), GPIO_PULLDOWN_ONLY);
      }else if constexpr(PinPull == arcos::abstraction::gpio::GpioHalPinPull::Float){
        gpio_set_pull_mode(static_cast<gpio_num_t>(PinNumber), GPIO_FLOATING);
      }
    }

    /**
     * @brief Write to pin individually using ESP-IDF API
     */
    template <uintptr_t PinNumber, bool State>
    static inline void WritePin(){
      static_assert(PinNumber <= 48, "PinNumber must be between 0-45 for ESP32-S3");
      gpio_set_level(static_cast<gpio_num_t>(PinNumber), State ? 1 : 0);
    }

    /**
     * @brief Write to pin in parallel using ESP-IDF API
     */
    template <typename PinBus>
    static inline void WritePinParallel(uintptr_t pinValues){
      static_assert(PinBus::pinbank != 0, "PinBus mask cannot be zero");

      for (uint8_t i = 0; i <= 48; i++){
        if ((PinBus::pinbank >> i) & 0x1){
          gpio_set_level(static_cast<gpio_num_t>(i), (pinValues >> i) & 0x1);
        }
      }
    }

    /**
     * @brief Read an individual pin digitally using ESP-IDF API
     */
    template <uintptr_t PinNumber>
    static inline bool ReadPin() {
      static_assert(PinNumber <= 48, "PinNumber must be between 0-45 for ESP32-S3");
      return gpio_get_level(static_cast<gpio_num_t>(PinNumber)) != 0;
    }

    /**
     * @brief Read a pin bank in parallel using ESP-IDF API
     */
    template <typename PinBus>
    static inline uint64_t ReadPinParallel(){
      static_assert(PinBus::pinbank != 0, "PinBus mask cannot be zero");

      uint64_t result = 0;
      for (uint8_t i = 0; i <= 48; i++){
        if ((PinBus::pinbank >> i) & 0x1){
          result |= (static_cast<uint64_t>(gpio_get_level(static_cast<gpio_num_t>(i))) << i);
        }
      }
      return result;
    }

    /**
     * @brief Write to pin individually using direct GPIO registers (fast)
     */
    template <uintptr_t PinNumber, bool State>
    static inline void FastWritePin(){
      static_assert(PinNumber <= 48, "PinNumber must be between 0-45 for ESP32-S3");

      if constexpr (State){
        GPIO.out_w1ts = (1 << PinNumber); // Set pin high
      }else{
        GPIO.out_w1tc = (1 << PinNumber); // Set pin low
      }
    }

    /**
     * @brief Write to pin in parallel using direct GPIO registers (fast)
     */
    template <typename PinBus>
    static inline void FastWritePinParallel(uintptr_t pinValues){
      static_assert(PinBus::pinbank != 0, "PinBus mask cannot be zero");

      GPIO.out_w1tc = ~pinValues & PinBus::pinbank; // Clear pins that should go LOW
      GPIO.out_w1ts = pinValues & PinBus::pinbank;  // Set pins that should go HIGH
    }

    /**
     * @brief Read an individual pin digitally using direct GPIO registers (fast)
     */
    template <uintptr_t PinNumber>
    static inline bool FastReadPin() {
      static_assert(PinNumber <= 48, "PinNumber must be between 0-45 for ESP32-S3");

      uint64_t gpio64 = (static_cast<uint64_t>(GPIO.in) & 0xFFFFFFFFULL) | 
                        (static_cast<uint64_t>(GPIO.in1.data) << 32);
      return (gpio64 >> PinNumber) & 0x1;
    }

    /**
     * @brief Read a pin bank in parallel using direct GPIO registers (fast)
     */
    template <typename PinBus>
    static inline uint64_t FastReadPinParallel(){
      static_assert(PinBus::pinbank != 0, "PinBus mask cannot be zero");

      uint64_t gpio64 = (static_cast<uint64_t>(GPIO.in) & 0xFFFFFFFFULL) | 
                        (static_cast<uint64_t>(GPIO.in1.data) << 32);
      return gpio64 & PinBus::pinbank;
    }
  };
};

#endif // ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_DIGITAL_GPIO_MODULE_HPP_