#ifndef ARCOS_ABSTRACTION_PLATFORMS_AVR_ATMEGA328P_UNO_HAL_CONNECTOR_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_AVR_ATMEGA328P_UNO_HAL_CONNECTOR_HPP_

#include "hal_gpio_digital_uno.hpp"
/** TODO: Add other protocol implementations for AVR ATmega328P
 * #include "hal_protocal_i2c_uno.hpp"
 * #include "hal_gpio_pwm_uno.hpp"
 * #include "hal_system_timer_uno.hpp"
 * #include "hal_protocal_spi_uno.hpp"
 */
using HAL_GPIO_DEFAULT = arcos::abstraction::HAL_GPIO_DIGITAL;

#endif // ARCOS_ABSTRACTION_PLATFORMS_AVR_ATMEGA328P_UNO_HAL_CONNECTOR_HPP_