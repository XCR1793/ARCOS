#ifndef ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_CONNECTOR_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_CONNECTOR_HPP_

#include "hal_gpio_digital_module.hpp"
#include "hal_protocal_i2c_module.hpp"
#include "hal_gpio_pwm_module.hpp"
#include "hal_system_timer_module.hpp"
#include "hal_protocal_spi_module.hpp"
#include "hal_interface_i2c_esp32s3.hpp"

using HAL_GPIO_DEFAULT = arcos::abstraction::HAL_GPIO_DIGITAL;
using HAL_I2C_DEFAULT = arcos::abstraction::ESP32S3_I2C;

#endif // ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_CONNECTOR_HPP_