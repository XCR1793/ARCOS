#ifndef ARCOS_ABSTRACTION_HAL_HPP_
#define ARCOS_ABSTRACTION_HAL_HPP_

#include "core/hal_gpio.hpp"

#if defined(TARGET_AVR_Atmega328p_Uno)
  #include "platforms/avr/atmega328p/uno/hal_connector.hpp"
#elif defined(TARGET_ESP32_Wroom32S2_Esp32Dev)
  #include "platforms/esp32/wroom32s2/esp32dev/hal_connector.hpp"
#else
  #error "No platform selected!"
#endif

#endif // ARCOS_ABSTRACTION_HAL_HPP_