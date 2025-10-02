#pragma once

#include "parallel_hardware_interface.hpp"
#include "platform_hal.hpp"

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
  IPlatformHAL* platform;              ///< Platform HAL reference
};
