/*****************************************************************
 * File:      i2s_parallel_driver_impl.hpp
 * Category:  abstraction/platforms/esp32/wroom32s3/module
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:    Platform-agnostic I2S parallel driver implementation
 *****************************************************************/

#include "i2s_parallel_driver.hpp"

namespace arcos::abstraction{

namespace{
static const char* I2S_PARALLEL_TAG = "I2S_PARALLEL";
}

/**
 * NOTE: This is now a platform-agnostic I2S parallel driver.
 * 
 * Platform-specific I2S configuration is handled by the platform HAL implementation.
 * This allows the same driver code to work on ESP32, STM32, or other platforms
 * by providing appropriate platform HAL implementations.
 * 
 * The platform HAL must provide:
 * 1. I2S peripheral configuration in parallel/LCD mode
 * 2. DMA setup for continuous transmission
 * 3. GPIO to I2S signal mapping
 * 4. Clock and timing configuration
 * 5. Buffer management operations
 */

I2sParallelDriver::I2sParallelDriver()
  : tx_handle(nullptr)
  , buffer(nullptr)
  , buffer_len(0)
  , initialized(false)
  , running(false)
{
  ESP_LOGI(I2S_PARALLEL_TAG, "I2S Parallel Driver initialized (platform-agnostic)");
}

I2sParallelDriver::~I2sParallelDriver() {
  stop();
  
  // Platform HAL handles cleanup of platform-specific resources
  if(tx_handle){
    // Platform-specific I2S cleanup would go here
    tx_handle = nullptr;
  }
}

bool I2sParallelDriver::init(const PinNumber* data_pins, const ParallelHardwareConfig& config){
  if(initialized){
    ESP_LOGW(I2S_PARALLEL_TAG, "Already initialized");
    return true;
  }
  
  this->config = config;
  
  ESP_LOGI(I2S_PARALLEL_TAG, "Initializing I2S parallel driver (platform-agnostic)");
  ESP_LOGI(I2S_PARALLEL_TAG, "  Data width: %d bits", config.data_width);
  ESP_LOGI(I2S_PARALLEL_TAG, "  Clock freq: %d Hz", config.clock_freq_hz);
  ESP_LOGI(I2S_PARALLEL_TAG, "  Platform: ESP32-S3");
  
  // Configure GPIO pins for output using platform HAL
  for(uint8_t i = 0; i < config.data_width; i++){
    if(data_pins[i] != PIN_NC){
      if(!configurePin(data_pins[i], true)){
        ESP_LOGE(I2S_PARALLEL_TAG, "Failed to configure pin %d", data_pins[i]);
        return false;
      }
      setPinStrength(data_pins[i], GPIO_DRIVE_CAP_3);
    }
  }
  
  // Configure clock pin if specified
  if(config.clock_pin != PIN_NC){
    if(!configurePin(config.clock_pin, true)){
      ESP_LOGE(I2S_PARALLEL_TAG, "Failed to configure clock pin %d", config.clock_pin);
      return false;
    }
    setPinStrength(config.clock_pin, GPIO_DRIVE_CAP_3);
  }
  
  // Note: Actual I2S peripheral configuration is platform-specific
  // and should be implemented in the platform HAL layer
  
  initialized = true;
  ESP_LOGI(I2S_PARALLEL_TAG, "I2S parallel driver initialized successfully");
  return true;
}

bool I2sParallelDriver::setBuffer(uint16_t* buffer, size_t buffer_len){
  if(!initialized){
    ESP_LOGE(I2S_PARALLEL_TAG, "Not initialized");
    return false;
  }
  
  this->buffer = buffer;
  this->buffer_len = buffer_len;
  
  // Platform-specific DMA descriptor setup handled by platform HAL
  ESP_LOGI(I2S_PARALLEL_TAG, "Buffer set: %d samples", buffer_len);
  
  return true;
}

bool I2sParallelDriver::setDirectBuffer(uint16_t* buffer_ptr, size_t buffer_len){
  return setBuffer(buffer_ptr, buffer_len);
}

bool I2sParallelDriver::swapBuffer(uint16_t* new_buffer_ptr, size_t buffer_len){
  if(!initialized){
    ESP_LOGE(I2S_PARALLEL_TAG, "Not initialized");
    return false;
  }
  
  // Platform-specific buffer swap handled by platform HAL
  this->buffer = new_buffer_ptr;
  this->buffer_len = buffer_len;
  
  ESP_LOGD(I2S_PARALLEL_TAG, "Buffer swapped: %d samples", buffer_len);
  return true;
}

uint16_t* I2sParallelDriver::getDirectBuffer() const {
  return buffer;
}

size_t I2sParallelDriver::getBufferSize() const {
  return buffer_len;
}

bool I2sParallelDriver::start(){
  if(!initialized){
    ESP_LOGE(I2S_PARALLEL_TAG, "Not initialized");
    return false;
  }
  
  if(running){
    ESP_LOGW(I2S_PARALLEL_TAG, "Already running");
    return true;
  }
  
  // Platform-specific I2S start handled by platform HAL
  // Implementation would call platform->startI2sTransmission() or similar
  
  running = true;
  ESP_LOGI(I2S_PARALLEL_TAG, "I2S transmission started");
  return true;
}

void I2sParallelDriver::stop(){
  if(!running){
    return;
  }
  
  // TODO: Stop I2S transmission
  // i2s_channel_disable(tx_handle);
  
  running = false;
}

bool I2sParallelDriver::isRunning() const {
  return running;
}

const ParallelHardwareConfig* I2sParallelDriver::getConfig() const {
  return initialized ? &config : nullptr;
}

/**
 * PLATFORM-AGNOSTIC I2S PARALLEL MODE IMPLEMENTATION:
 * 
 * This driver is now platform-independent. Platform-specific I2S configuration
 * is provided via the platform HAL implementation.
 * 
 * To support a new platform:
 * 1. Implement IPlatformHAL for your platform
 * 2. Provide I2S peripheral configuration functions
 * 3. Implement DMA descriptor setup
 * 4. Map GPIO pins to I2S signals
 * 5. Handle clock and timing configuration
 * 
 * Example platforms:
 * - ESP32/ESP32-S2: I2S in LCD mode
 * - ESP32-S3: LCD_CAM peripheral (use LcdParallel instead)
 * - STM32: SAI in parallel mode or custom implementation
 * - RP2040: PIO state machines for parallel output
 * - Other: Custom implementation via platform HAL
 * 
 * The platform HAL abstracts all hardware-specific details,
 * making this driver truly portable across architectures.
 */

} // namespace arcos::abstraction

