/*****************************************************************
 * File:      hal_gpio_digital_module.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Implements standard digital gpio hardware abstraction for the
 *    esp32s3 modules.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_GPIO_DIGITAL_MODULE_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_GPIO_DIGITAL_MODULE_HPP_

#include "soc/gpio_reg.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_reg.h"
#include "driver/gpio.h"

namespace arcos::abstraction{
  struct HAL_GPIO_DIGITAL{
    /** 
     * @brief Nothing to initialise as esp32 has all gpios on one port
     */
    static inline void Initialise(){}

    /**
     * @brief Sets pins as output or input using ESP-IDF API
     */
    template <uintptr_t PinNumber, gpio::GpioHalPinMode PinMode>
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

      for(uint8_t i = 0; i <= 48; i++){
        if((PinBus::pinbank >> i) & 0x1){
          gpio_set_level(static_cast<gpio_num_t>(i), (pinValues >> i) & 0x1);
        }
      }
    }

    /**
     * @brief Read an individual pin digitally using ESP-IDF API
     */
    template <uintptr_t PinNumber>
    static inline bool ReadPin(){
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
      for(uint8_t i = 0; i <= 48; i++){
        if((PinBus::pinbank >> i) & 0x1){
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
    static inline bool FastReadPin(){
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

    /**
     * @brief Sets pin as input or output (runtime)
     * @param pin   Pin number at runtime
     * @param mode  Pin mode at runtime
     * @note Runtime variant of SetPin()
     */
    static inline void SetPin(uintptr_t pin, gpio::GpioHalPinMode mode, gpio::GpioHalPinPull pull = gpio::GpioHalPinPull::Float){
      if(pin > 48){
        return;  // Pin number out of range
      }

      gpio_config_t cfg{};
      cfg.pin_bit_mask = (1ULL << pin);
      cfg.mode = (mode == gpio::GpioHalPinMode::Output) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
      cfg.intr_type = GPIO_INTR_DISABLE;

      switch(pull){
        case gpio::GpioHalPinPull::PullUp:
          cfg.pull_up_en = GPIO_PULLUP_ENABLE;
          cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
          break;
        case gpio::GpioHalPinPull::PullDown:
          cfg.pull_up_en = GPIO_PULLUP_DISABLE;
          cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;
          break;
        case gpio::GpioHalPinPull::Float:
          cfg.pull_up_en = GPIO_PULLUP_DISABLE;
          cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
          break;
      }
    
      gpio_config(&cfg);
    }


    /**
     * @brief Sets pull direction for pin (runtime)
     * @param pin   Pin number at runtime
     * @param pull  Pin pull configuration at runtime
     * @note Runtime variant of PullPin()
     */
    static inline void PullPin(uintptr_t pin, gpio::GpioHalPinPull pull){
      if(pin > 48){
        return; // Pin number out of range
      }
    
      switch(pull) {
        case gpio::GpioHalPinPull::PullUp:
          gpio_set_pull_mode(static_cast<gpio_num_t>(pin), GPIO_PULLUP_ONLY);
          break;
        case gpio::GpioHalPinPull::PullDown:
          gpio_set_pull_mode(static_cast<gpio_num_t>(pin), GPIO_PULLDOWN_ONLY);
          break;
        case gpio::GpioHalPinPull::Float:
          gpio_set_pull_mode(static_cast<gpio_num_t>(pin), GPIO_FLOATING);
          break;
      }
    }

    /**
     * @brief Writes a pin as high or low (runtime)
     * @param pin    Pin number at runtime
     * @param state  True for HIGH, false for LOW
     * @note Runtime variant of WritePin()
     */
    static inline void WritePin(uintptr_t pin, bool state){
      if(pin > 48){
        return; // Pin number out of range
      }
      gpio_set_level(static_cast<gpio_num_t>(pin), state ? 1 : 0);
    }

    /**
     * @brief Writes a set of pins in parallel (runtime)
     * @param pinMask    Mask of pins to write
     * @param pinValues  Bit values corresponding to each pin
     * @note Runtime variant of WritePinParallel()
     */
    static inline void WritePinParallel(uintptr_t pinMask, uintptr_t pinValues){
      for(uint8_t i = 0; i <= 48; i++){
        if((pinMask >> i) & 0x1){
          gpio_set_level(static_cast<gpio_num_t>(i), (pinValues >> i) & 0x1);
        }
      }
    }

    /**
     * @brief Reads a pin as high or low (runtime)
     * @param pin  Pin number at runtime
     * @return True if HIGH, false if LOW
     * @note Runtime variant of ReadPin()
     */
    static inline bool ReadPin(uintptr_t pin){
      if(pin > 48){
        return false; // Pin number out of range
      }
      return gpio_get_level(static_cast<gpio_num_t>(pin)) != 0;
    }

    /**
     * @brief Reads a set of pins in parallel (runtime)
     * @param pinMask  Mask of pins to read
     * @return Bit values of the read pins
     * @note Runtime variant of ReadPinParallel()
     */
    static inline uintptr_t ReadPinParallel(uintptr_t pinMask){
      uintptr_t result = 0;
      for(uint8_t i = 0; i <= 48; i++){
        if((pinMask >> i) & 0x1){
          result |= (static_cast<uintptr_t>(gpio_get_level(static_cast<gpio_num_t>(i))) << i);
        }
      }
      return result;
    }

    /**
     * @brief Writes a pin using a faster, lower-level implementation (runtime)
     * @param pin    Pin number at runtime
     * @param state  True for HIGH, false for LOW
     * @note Runtime variant of FastWritePin()
     */
    static inline void FastWritePin(uintptr_t pin, bool state){
      if(pin > 48){
        return;
      }
      if(state) {
        if(pin < 32){
          GPIO.out_w1ts = (1 << pin);
        }else{
          GPIO.out1_w1ts.data = (1 << (pin - 32));
        }
      }else{
        if(pin < 32){
          GPIO.out_w1tc = (1 << pin);
        }else{
          GPIO.out1_w1tc.data = (1 << (pin - 32));
        }
      }
    }


    /**
     * @brief Writes a set of pins in parallel using a faster implementation (runtime)
     * @param pinMask    Mask of pins to write
     * @param pinValues  Bit values corresponding to each pin
     * @note Runtime variant of FastWritePinParallel()
     */
    static inline void FastWritePinParallel(uintptr_t pinMask, uintptr_t pinValues){
      // Convert to 64-bit to handle both GPIO banks (0-31 and 32-48)
      uint64_t mask64 = static_cast<uint64_t>(pinMask);
      uint64_t values64 = static_cast<uint64_t>(pinValues);
      
      // Lower bank (0-31)
      uint32_t maskLow   = static_cast<uint32_t>(mask64 & 0xFFFFFFFFULL);
      uint32_t valuesLow = static_cast<uint32_t>(values64 & maskLow);

      GPIO.out_w1tc = maskLow & ~valuesLow; // Clear low pins
      GPIO.out_w1ts = valuesLow;            // Set low pins

      // Upper bank (32-48)
      uint32_t maskHigh   = static_cast<uint32_t>((mask64 >> 32) & 0xFFFFFFFFULL);
      uint32_t valuesHigh = static_cast<uint32_t>((values64 >> 32) & maskHigh);

      GPIO.out1_w1tc.data = maskHigh & ~valuesHigh; // Clear high pins
      GPIO.out1_w1ts.data = valuesHigh;             // Set high pins
    }

    /**
     * @brief Reads a pin using a faster, lower-level implementation (runtime)
     * @param pin  Pin number at runtime
     * @return True if HIGH, false if LOW
     * @note Runtime variant of FastReadPin()
     */
    static inline bool FastReadPin(uintptr_t pin){
      if(pin > 48){
        return false; // Pin number out of range
      }
      uint64_t gpio64 = (static_cast<uint64_t>(GPIO.in) & 0xFFFFFFFFULL) |
                        (static_cast<uint64_t>(GPIO.in1.data) << 32);
      return (gpio64 >> pin) & 0x1;
    }

    /**
     * @brief Reads a set of pins in parallel using a faster implementation (runtime)
     * @param pinMask  Mask of pins to read
     * @return Bit values of the read pins
     * @note Runtime variant of FastReadPinParallel()
     */
    static inline uintptr_t FastReadPinParallel(uintptr_t pinMask){
      uint64_t gpio64 = (static_cast<uint64_t>(GPIO.in) & 0xFFFFFFFFULL) |
                        (static_cast<uint64_t>(GPIO.in1.data) << 32);
      return gpio64 & pinMask;
    }
  };
}; // arcos::abstraction

#endif // ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_GPIO_DIGITAL_MODULE_HPP_