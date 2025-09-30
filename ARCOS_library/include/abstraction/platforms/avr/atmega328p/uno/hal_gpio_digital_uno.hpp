/*****************************************************************
 * File:      hal_gpio_digital_uno.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Implements standard digital gpio hardware abstraction for the
 *    AVR ATmega328P Arduino Uno.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_PLATFORMS_AVR_ATMEGA328P_UNO_HAL_GPIO_DIGITAL_UNO_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_AVR_ATMEGA328P_UNO_HAL_GPIO_DIGITAL_UNO_HPP_

#include <avr/io.h>

namespace arcos::abstraction{
  struct HAL_GPIO_DIGITAL{
    struct PinMap {
      volatile uint8_t* ddr;
      volatile uint8_t* port;
      volatile uint8_t* pin;
      uint8_t bit;
    };

    /** Flattened Arduino pin mapping: digital 0–13 + analog 14–19 */
    static constexpr PinMap pinmap[20] = {
      {&DDRD, &PORTD, &PIND, 0}, // 0
      {&DDRD, &PORTD, &PIND, 1}, // 1
      {&DDRD, &PORTD, &PIND, 2}, // 2
      {&DDRD, &PORTD, &PIND, 3}, // 3
      {&DDRD, &PORTD, &PIND, 4}, // 4
      {&DDRD, &PORTD, &PIND, 5}, // 5
      {&DDRD, &PORTD, &PIND, 6}, // 6
      {&DDRD, &PORTD, &PIND, 7}, // 7
      {&DDRB, &PORTB, &PINB, 0}, // 8
      {&DDRB, &PORTB, &PINB, 1}, // 9
      {&DDRB, &PORTB, &PINB, 2}, // 10
      {&DDRB, &PORTB, &PINB, 3}, // 11
      {&DDRB, &PORTB, &PINB, 4}, // 12
      {&DDRB, &PORTB, &PINB, 5}, // 13
      {&DDRC, &PORTC, &PINC, 0}, // A0 = 14
      {&DDRC, &PORTC, &PINC, 1}, // A1 = 15
      {&DDRC, &PORTC, &PINC, 2}, // A2 = 16
      {&DDRC, &PORTC, &PINC, 3}, // A3 = 17
      {&DDRC, &PORTC, &PINC, 4}, // A4 = 18
      {&DDRC, &PORTC, &PINC, 5}  // A5 = 19
    };

    /** 
     * @brief Nothing to initialise for AVR
     */
    static inline void Initialise(){}

    /**
     * @brief Sets pins as output or input using AVR register manipulation
     */
    template <uintptr_t PinNumber, gpio::GpioHalPinMode PinMode>
    static inline void SetPin(){
      static_assert(PinNumber < 20, "PinNumber must be between 0-19 for Arduino Uno");
      if constexpr(PinMode == gpio::GpioHalPinMode::Output){
        *pinmap[PinNumber].ddr |= (1 << pinmap[PinNumber].bit);
      }else if constexpr(PinMode == gpio::GpioHalPinMode::Input){
        *pinmap[PinNumber].ddr &= ~(1 << pinmap[PinNumber].bit);
      }
    }

    /**
     * @brief Sets a pin's internal resistor to float, pull up or pull down using AVR register manipulation
     */
    template <uintptr_t PinNumber, gpio::GpioHalPinPull PinPull>
    static inline void PullPin(){
      static_assert(PinNumber < 20, "PinNumber must be between 0-19 for Arduino Uno");
      
      if constexpr(PinPull == gpio::GpioHalPinPull::PullUp){
        *pinmap[PinNumber].port |= (1 << pinmap[PinNumber].bit);
      }else if constexpr(PinPull == gpio::GpioHalPinPull::PullDown){
        /** AVR doesn't support internal pull-down resistors */
        static_assert(PinPull != gpio::GpioHalPinPull::PullDown, "AVR ATmega328P doesn't support internal pull-down resistors");
      }else if constexpr(PinPull == gpio::GpioHalPinPull::Float){
        *pinmap[PinNumber].port &= ~(1 << pinmap[PinNumber].bit);
      }
    }

    /**
     * @brief Write to pin individually using AVR register manipulation
     */
    template <uintptr_t PinNumber, bool State>
    static inline void WritePin(){
      static_assert(PinNumber < 20, "PinNumber must be between 0-19 for Arduino Uno");
      if constexpr(State){
        *pinmap[PinNumber].port |= (1 << pinmap[PinNumber].bit);
      }else{
        *pinmap[PinNumber].port &= ~(1 << pinmap[PinNumber].bit);
      }
    }

    /**
     * @brief Write to pin in parallel using AVR register manipulation
     */
    template <typename PinBus>
    static inline void WritePinParallel(uintptr_t pinValues){
      static_assert(PinBus::pinbank != 0, "PinBus mask cannot be zero");
      
      /** AVR parallel operations are limited to 8-bit ports */
      for(uint8_t i = 0; i < 20; i++){
        if((PinBus::pinbank >> i) & 0x1){
          if((pinValues >> i) & 0x1){
            *pinmap[i].port |= (1 << pinmap[i].bit);
          }else{
            *pinmap[i].port &= ~(1 << pinmap[i].bit);
          }
        }
      }
    }

    /**
     * @brief Read an individual pin digitally using AVR register manipulation
     */
    template <uintptr_t PinNumber>
    static inline bool ReadPin(){
      static_assert(PinNumber < 20, "PinNumber must be between 0-19 for Arduino Uno");
      return (*pinmap[PinNumber].pin & (1 << pinmap[PinNumber].bit)) != 0;
    }

    /**
     * @brief Read a pin bank in parallel using AVR register manipulation
     */
    template <typename PinBus>
    static inline uint32_t ReadPinParallel(){
      static_assert(PinBus::pinbank != 0, "PinBus mask cannot be zero");
      
      uint32_t result = 0;
      for(uint8_t i = 0; i < 20; i++){
        if((PinBus::pinbank >> i) & 0x1){
          result |= (static_cast<uint32_t>((*pinmap[i].pin & (1 << pinmap[i].bit)) != 0) << i);
        }
      }
      return result;
    }

    /**
     * @brief Write to pin individually using AVR register manipulation (fast)
     * @note For AVR, this is the same as WritePin since direct register access is already used
     */
    template <uintptr_t PinNumber, bool State>
    static inline void FastWritePin(){
      WritePin<PinNumber, State>();
    }

    /**
     * @brief Write to pin in parallel using AVR register manipulation (fast)
     * @note For AVR, this is the same as WritePinParallel since direct register access is already used
     */
    template <typename PinBus>
    static inline void FastWritePinParallel(uintptr_t pinValues){
      WritePinParallel<PinBus>(pinValues);
    }

    /**
     * @brief Read an individual pin digitally using AVR register manipulation (fast)
     * @note For AVR, this is the same as ReadPin since direct register access is already used
     */
    template <uintptr_t PinNumber>
    static inline bool FastReadPin(){
      return ReadPin<PinNumber>();
    }

    /**
     * @brief Read a pin bank in parallel using AVR register manipulation (fast)
     * @note For AVR, this is the same as ReadPinParallel since direct register access is already used
     */
    template <typename PinBus>
    static inline uint32_t FastReadPinParallel(){
      return ReadPinParallel<PinBus>();
    }

    /**
     * @brief Sets pin as input or output (runtime)
     * @param pin   Pin number at runtime
     * @param mode  Pin mode at runtime
     * @note Runtime variant of SetPin()
     */
    static inline void SetPin(uintptr_t pin, gpio::GpioHalPinMode mode, gpio::GpioHalPinPull pull = gpio::GpioHalPinPull::Float){
      if(pin >= 20) return; // Pin number out of range
      
      if(mode == gpio::GpioHalPinMode::Output){
        *pinmap[pin].ddr |= (1 << pinmap[pin].bit);
      }else{
        *pinmap[pin].ddr &= ~(1 << pinmap[pin].bit);
      }
      
      /** Set pull resistor */
      PullPin(pin, pull);
    }

    /**
     * @brief Sets pull direction for pin (runtime)
     * @param pin   Pin number at runtime
     * @param pull  Pin pull configuration at runtime
     * @note Runtime variant of PullPin()
     */
    static inline void PullPin(uintptr_t pin, gpio::GpioHalPinPull pull){
      if(pin >= 20) return; // Pin number out of range
      
      switch(pull){
        case gpio::GpioHalPinPull::PullUp:
          *pinmap[pin].port |= (1 << pinmap[pin].bit);
          break;
        case gpio::GpioHalPinPull::PullDown:
          /** AVR doesn't support internal pull-down resistors */
          break;
        case gpio::GpioHalPinPull::Float:
          *pinmap[pin].port &= ~(1 << pinmap[pin].bit);
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
      if(pin >= 20) return; // Pin number out of range
      if(state){
        *pinmap[pin].port |= (1 << pinmap[pin].bit);
      }else{
        *pinmap[pin].port &= ~(1 << pinmap[pin].bit);
      }
    }

    /**
     * @brief Writes a set of pins in parallel (runtime)
     * @param pinMask    Mask of pins to write
     * @param pinValues  Bit values corresponding to each pin
     * @note Runtime variant of WritePinParallel()
     */
    static inline void WritePinParallel(uintptr_t pinMask, uintptr_t pinValues){
      for(uint8_t i = 0; i < 20; i++){
        if((pinMask >> i) & 0x1){
          if((pinValues >> i) & 0x1){
            *pinmap[i].port |= (1 << pinmap[i].bit);
          }else{
            *pinmap[i].port &= ~(1 << pinmap[i].bit);
          }
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
      if(pin >= 20) return false; // Pin number out of range
      return (*pinmap[pin].pin & (1 << pinmap[pin].bit)) != 0;
    }

    /**
     * @brief Reads a set of pins in parallel (runtime)
     * @param pinMask  Mask of pins to read
     * @return Bit values of the read pins
     * @note Runtime variant of ReadPinParallel()
     */
    static inline uintptr_t ReadPinParallel(uintptr_t pinMask){
      uintptr_t result = 0;
      for(uint8_t i = 0; i < 20; i++){
        if((pinMask >> i) & 0x1){
          result |= (static_cast<uintptr_t>((*pinmap[i].pin & (1 << pinmap[i].bit)) != 0) << i);
        }
      }
      return result;
    }

    /**
     * @brief Writes a pin using a faster, lower-level implementation (runtime)
     * @param pin    Pin number at runtime
     * @param state  True for HIGH, false for LOW
     * @note Runtime variant of FastWritePin() - same as WritePin for AVR
     */
    static inline void FastWritePin(uintptr_t pin, bool state){
      WritePin(pin, state);
    }

    /**
     * @brief Writes a set of pins in parallel using a faster implementation (runtime)
     * @param pinMask    Mask of pins to write
     * @param pinValues  Bit values corresponding to each pin
     * @note Runtime variant of FastWritePinParallel() - same as WritePinParallel for AVR
     */
    static inline void FastWritePinParallel(uintptr_t pinMask, uintptr_t pinValues){
      WritePinParallel(pinMask, pinValues);
    }

    /**
     * @brief Reads a pin using a faster, lower-level implementation (runtime)
     * @param pin  Pin number at runtime
     * @return True if HIGH, false if LOW
     * @note Runtime variant of FastReadPin() - same as ReadPin for AVR
     */
    static inline bool FastReadPin(uintptr_t pin){
      return ReadPin(pin);
    }

    /**
     * @brief Reads a set of pins in parallel using a faster implementation (runtime)
     * @param pinMask  Mask of pins to read
     * @return Bit values of the read pins
     * @note Runtime variant of FastReadPinParallel() - same as ReadPinParallel for AVR
     */
    static inline uintptr_t FastReadPinParallel(uintptr_t pinMask){
      return ReadPinParallel(pinMask);
    }
  };
}; // arcos::abstraction

#endif // ARCOS_ABSTRACTION_PLATFORMS_AVR_ATMEGA328P_UNO_HAL_GPIO_DIGITAL_UNO_HPP_