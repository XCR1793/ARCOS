/*****************************************************************
 * File:      i2s_parallel_driver.hpp
 * Category:  abstraction/platforms/esp32/wroom32s3/module
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    I2S parallel driver implementation for ESP32-S3 providing
 *    platform-agnostic parallel output using I2S peripheral.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_I2S_PARALLEL_DRIVER_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_I2S_PARALLEL_DRIVER_HPP_

#include "../../../../core/hal_protocal_parallel.hpp"
#include <cstdio>
#include <cstdarg>
#include "esp_log.h"
#include "driver/gpio.h"

namespace arcos::abstraction{

using parallel::IParallelHardware;
using parallel::ParallelHardwareConfig;

/** Forward declaration for platform-specific I2S handle */
struct PlatformI2sHandle;

/**
 * @brief I2S Parallel Driver Implementation
 * 
 * This is a platform-agnostic I2S parallel driver implementation that uses
 * the platform HAL for all hardware operations. This makes it portable across
 * different microcontrollers (ESP32, STM32, etc.)
 * 
 * NOTE: Platform-specific I2S configuration must be provided via the platform HAL.
 */
class I2sParallelDriver : public IParallelHardware {
public:
  I2sParallelDriver();
  ~I2sParallelDriver() override;
  
  /** IParallelHardware interface implementation */
  bool init(const PinNumber* data_pins, const ParallelHardwareConfig& config) override;
  bool setBuffer(uint16_t* buffer, size_t buffer_len) override;
  bool setDirectBuffer(uint16_t* buffer_ptr, size_t buffer_len) override;
  bool swapBuffer(uint16_t* new_buffer_ptr, size_t buffer_len) override;
  uint16_t* getDirectBuffer() const override;
  size_t getBufferSize() const override;
  bool start() override;
  void stop() override;
  bool isRunning() const override;
  const ParallelHardwareConfig* getConfig() const override;
  const char* getBackendName() const override { return "I2S_PARALLEL"; }
  
private:
  PlatformI2sHandle* tx_handle;        ///< Opaque platform-specific I2S handle
  ParallelHardwareConfig config;
  uint16_t* buffer;
  size_t buffer_len;
  bool initialized;
  bool running;
  
  /** Private helper functions for GPIO configuration */
  static inline bool configurePin(int pin, bool is_output){
    if(pin < 0) return false;
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = is_output ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    return gpio_config(&io_conf) == ESP_OK;
  }
  
  static inline bool setPinStrength(int pin, gpio_drive_cap_t strength){
    if(pin < 0) return false;
    return gpio_set_drive_capability((gpio_num_t)pin, strength) == ESP_OK;
  }
};

} // namespace arcos::abstraction

// Include implementation
#include "i2s_parallel_driver_impl.hpp"

#endif // ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_I2S_PARALLEL_DRIVER_HPP_
