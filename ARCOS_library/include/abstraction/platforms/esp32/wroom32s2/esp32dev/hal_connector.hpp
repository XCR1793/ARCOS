#ifndef ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S2_ESP32DEV_HAL_CONNECTOR_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S2_ESP32DEV_HAL_CONNECTOR_HPP_

#include "hal_gpio_digital_esp32dev.hpp"
/** TODO: Add other protocol implementations for ESP32-S2
 * #include "hal_protocal_i2c_esp32dev.hpp"
 * #include "hal_gpio_pwm_esp32dev.hpp"
 * #include "hal_system_timer_esp32dev.hpp"
 * #include "hal_protocal_spi_esp32dev.hpp"
 */
using HAL_GPIO_DEFAULT = arcos::abstraction::HAL_GPIO_DIGITAL;

#endif // ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S2_ESP32DEV_HAL_CONNECTOR_HPP_