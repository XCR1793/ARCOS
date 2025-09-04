#ifndef ARCOS_ABSTRACTION_PLATFORMS_AVR_ATMEGA328P_UNO_HAL_GPIO_UNO_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_AVR_ATMEGA328P_UNO_HAL_GPIO_UNO_HPP_

#include <Arduino.h>
#include "../../../../core/hal_gpio.hpp"

class HAL_GPIO_Arduino_Impl{
public:
  HAL_GPIO_Arduino_Impl(uint8_t pin, PinMode mode) : m_pin(pin){
    pinMode(pin, (mode == PinMode::Output) ? OUTPUT : INPUT);
  }

  void write(PinState state){
    digitalWrite(m_pin, (state == PinState::Set) ? HIGH : LOW);
  }

  void toggle(){
    write(read() == PinState::Set ? PinState::Reset : PinState::Set);
  }

  PinState read(){
    return digitalRead(m_pin) ? PinState::Set : PinState::Reset;
  }

private:
  uint8_t m_pin;
};

#endif // ARCOS_ABSTRACTION_PLATFORMS_AVR_ATMEGA328P_UNO_HAL_GPIO_UNO_HPP_