#ifndef ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S2_ESP32DEV_HAL_CONNECTOR_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S2_ESP32DEV_HAL_CONNECTOR_HPP_

#include "hal_gpio_digital_esp32dev.hpp"
#include "hal_logging_esp32dev.hpp"
/** TODO: Add other protocol implementations for ESP32-S2
 * #include "hal_protocal_i2c_esp32dev.hpp"
 * #include "hal_gpio_pwm_esp32dev.hpp"
 * #include "hal_system_timer_esp32dev.hpp"
 * #include "hal_protocal_spi_esp32dev.hpp"
 */

namespace arcos::abstraction{

/** Default digital GPIO implementation for ESP32-S2 */
using HAL_GPIO_DEFAULT = arcos::abstraction::HAL_GPIO_DIGITAL;

/** Default logging implementation for ESP32-S2 (Info level) */
using HAL_LOG_DEFAULT = arcos::abstraction::ESP32_Wroom32S2_LoggingDefault;

/** Debug logging with extended output */
using HAL_LOG_DEBUG = arcos::abstraction::ESP32_Wroom32S2_LoggingDebug;

/** Verbose logging with all messages */
using HAL_LOG_VERBOSE = arcos::abstraction::ESP32_Wroom32S2_LoggingVerbose;

/** Production logging with errors only */
using HAL_LOG_PRODUCTION = arcos::abstraction::ESP32_Wroom32S2_LoggingProduction;

} // namespace arcos::abstraction

#endif // ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S2_ESP32DEV_HAL_CONNECTOR_HPP_