#ifndef ARCOS_ABSTRACTION_HAL_HPP
#define ARCOS_ABSTRACTION_HAL_HPP

#include "core/hal_gpio.hpp"

#if defined(TARGET_ESP32_Xtensa_Wroom32S3)
  #include "platforms/esp32/xtensa/wroom32S3/hal_connector.hpp"
  using HAL_GPIO_DEFAULT = HAL_GPIO<HAL_GPIO_ESP32_Impl>;
#elif defined(TARGET_AVR_Atmega328p_Uno)
  #include "platforms/avr/atmega328p/uno/hal_connector.hpp"
#else
  #error "No platform selected!"
#endif

#endif // ARCOS_ABSTRACTION_HAL_HPP