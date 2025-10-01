#pragma once

#include "parallel_hardware_interface.hpp"
#include "driver/i2s_std.h"
#include "driver/gpio.h"

/**
 * @brief I2S Parallel Driver Implementation
 * 
 * This is an EXAMPLE/TEMPLATE implementation showing how to create an alternative
 * hardware backend using the ESP32's I2S peripheral instead of LCD_CAM.
 * 
 * NOTE: This is a skeleton implementation for reference. Full implementation
 * would require configuring I2S in parallel mode and setting up proper DMA.
 */
class I2sParallelDriver : public IParallelHardware {
public:
  I2sParallelDriver();
  ~I2sParallelDriver() override;
  
  /** IParallelHardware interface implementation */
  bool init(const gpio_num_t* data_pins, const ParallelHardwareConfig& config) override;
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
  i2s_chan_handle_t tx_handle;
  ParallelHardwareConfig config;
  uint16_t* buffer;
  size_t buffer_len;
  bool initialized;
  bool running;
};
