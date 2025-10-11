#ifndef ARCOS_ABSTRACTION_PLATFORMS_AVR_ATMEGA328P_UNO_HAL_CONNECTOR_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_AVR_ATMEGA328P_UNO_HAL_CONNECTOR_HPP_

#include "hal_gpio_digital_uno.hpp"
#include "hal_logging_uno.hpp"
/** TODO: Add other protocol implementations for AVR ATmega328P
 * #include "hal_protocal_i2c_uno.hpp"
 * #include "hal_gpio_pwm_uno.hpp"
 * #include "hal_system_timer_uno.hpp"
 * #include "hal_protocal_spi_uno.hpp"
 */

namespace arcos::abstraction{

/** Default digital GPIO implementation for AVR ATmega328P */
using HAL_GPIO_DEFAULT = arcos::abstraction::HAL_GPIO_DIGITAL;

/** Default logging implementation for AVR ATmega328P (Info level) */
using HAL_LOG_DEFAULT = arcos::abstraction::AVR_ATmega328P_LoggingDefault;

/** Debug logging with extended output */
using HAL_LOG_DEBUG = arcos::abstraction::AVR_ATmega328P_LoggingDebug;

/** Production logging with errors only (recommended for AVR) */
using HAL_LOG_PRODUCTION = arcos::abstraction::AVR_ATmega328P_LoggingProduction;

} // namespace arcos::abstraction

#endif // ARCOS_ABSTRACTION_PLATFORMS_AVR_ATMEGA328P_UNO_HAL_CONNECTOR_HPP_