/*****************************************************************
 * File:      driver_hub75_i2s_impl.hpp
 * Category:  abstraction/drivers/communication/HUB75
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:    I2S protocol implementation for HUB75 display
 *****************************************************************/

#include "driver_hub75_i2s.hpp"
#include <cstring>

namespace arcos::abstraction::drivers{

namespace{
constexpr const char* HUB75_I2S_TAG = "HUB75_I2S";
}

HUB75_I2S_Protocol::HUB75_I2S_Protocol()
  : hwInterface(nullptr)
  , bufferManager(nullptr)
  , frontBuffer(nullptr)
  , backBuffer(nullptr)
  , buffer_size(0)
  , initialized(false)
  , running(false)
  , external_init(false)
{
}

HUB75_I2S_Protocol::~HUB75_I2S_Protocol(){
  stop();
  
  // NOTE: Protocol does NOT own hardware/buffer managers
  // Application is responsible for lifecycle management of injected dependencies
}

bool HUB75_I2S_Protocol::init(const HUB75Config& cfg, int buf_size, 
                               IParallelHardware* hardware, IDmaBufferManager* buffer_mgr){
  if(initialized){
    ESP_LOGW(HUB75_I2S_TAG, "Protocol already initialised");
    return true;
  }
  
  // Protocol REQUIRES hardware and buffer manager to be injected by application
  if(!hardware || !buffer_mgr){
    ESP_LOGE(HUB75_I2S_TAG, "Hardware interface and buffer manager must be provided (cannot be null)");
    return false;
  }
  
  config = cfg;
  buffer_size = buf_size;
  hwInterface = hardware;
  bufferManager = buffer_mgr;
  external_init = true;
  
  /** GPIO pin mapping for HUB75 protocol */
  int num_pins = (config.pins.oe_pin2 >= 0) ? 14 : 13;
  
  /** Prepare GPIO pin array for hardware interface */
  PinNumber* lcd_data_pins = new PinNumber[num_pins];
  
  lcd_data_pins[0] = config.pins.r0_pin;   // R0
  lcd_data_pins[1] = config.pins.g0_pin;   // G0
  lcd_data_pins[2] = config.pins.b0_pin;   // B0
  lcd_data_pins[3] = config.pins.r1_pin;   // R1
  lcd_data_pins[4] = config.pins.g1_pin;   // G1
  lcd_data_pins[5] = config.pins.b1_pin;   // B1
  lcd_data_pins[6] = config.pins.lat_pin;  // LAT
  lcd_data_pins[7] = config.pins.oe_pin;   // OE1
  lcd_data_pins[8] = config.pins.a_pin;    // A
  lcd_data_pins[9] = config.pins.b_pin;    // B
  lcd_data_pins[10] = config.pins.c_pin;   // C
  lcd_data_pins[11] = config.pins.d_pin;   // D
  lcd_data_pins[12] = config.pins.e_pin;   // E
  
  if(config.pins.oe_pin2 >= 0){
    lcd_data_pins[13] = config.pins.oe_pin2; // OE2
  }

  /** Allocate DMA buffers using buffer manager */
  DmaBufferConfig buffer_config;
  buffer_config.buffer_count = 2;  // Double buffering
  buffer_config.sample_count = buffer_size;
  buffer_config.mode = BufferMode::DOUBLE_BUFFER;
  buffer_config.auto_allocate = true;
  
  if(!bufferManager->init(buffer_config)){
    ESP_LOGE(HUB75_I2S_TAG, "Failed to initialize buffer manager");
    delete[] lcd_data_pins;
    return false;
  }

  /** Configure hardware interface using abstract ParallelHardwareConfig */
  ParallelHardwareConfig hw_config;
  hw_config.clock_freq_hz = config.clock_freq_hz;
  hw_config.invert_clock = false;
  hw_config.continuous_mode = true;
  hw_config.data_width = num_pins;
  hw_config.clock_pin = config.pins.clock_pin;
  hw_config.data_pins = lcd_data_pins;
  hw_config.data_pin_count = num_pins;
  
  /** Initialise hardware interface */
  if(!hwInterface->init(lcd_data_pins, hw_config)){
    ESP_LOGE(HUB75_I2S_TAG, "Failed to initialise hardware interface");
    delete[] lcd_data_pins;
    return false;
  }
  
  delete[] lcd_data_pins;

  /** Set up buffer pointers from buffer manager */
  frontBuffer = bufferManager->getFrontBuffer();
  backBuffer = bufferManager->getBackBuffer();
  
  if(!frontBuffer || !backBuffer){
    ESP_LOGE(HUB75_I2S_TAG, "Failed to get buffer pointers from buffer manager");
    return false;
  }
  
  initialized = true;
  
  ESP_LOGI(HUB75_I2S_TAG, "HUB75 I2S protocol initialised:");
  ESP_LOGI(HUB75_I2S_TAG, "  Hardware backend: %s", hwInterface->getBackendName());
  ESP_LOGI(HUB75_I2S_TAG, "  Clock: %dMHz", config.clock_freq_hz / 1000000);
  ESP_LOGI(HUB75_I2S_TAG, "  Buffer size: %d samples", buffer_size);
  ESP_LOGI(HUB75_I2S_TAG, "  Buffer mode: %s", 
           bufferManager->getMode() == BufferMode::DOUBLE_BUFFER ? "Double buffered" : "Single buffered");
  
  return true;
}

bool HUB75_I2S_Protocol::init(const HUB75Config& config, int buffer_size){
  ESP_LOGE(HUB75_I2S_TAG, "Must call init() with hardware and buffer manager dependencies");
  ESP_LOGE(HUB75_I2S_TAG, "Use init(config, buffer_size, hardware, buffer_manager) instead");
  return false;
}

bool HUB75_I2S_Protocol::start(){
  if(!initialized){
    ESP_LOGE(HUB75_I2S_TAG, "Protocol not initialised");
    return false;
  }
  
  if(running){
    ESP_LOGW(HUB75_I2S_TAG, "Protocol already running");
    return true;
  }
  
  /** Debug: Check if frontBuffer has data */
  ESP_LOGI(HUB75_I2S_TAG, "DEBUG: frontBuffer first 10 samples: %04X %04X %04X %04X %04X %04X %04X %04X %04X %04X",
                frontBuffer[0], frontBuffer[1], frontBuffer[2], frontBuffer[3], frontBuffer[4],
                frontBuffer[5], frontBuffer[6], frontBuffer[7], frontBuffer[8], frontBuffer[9]);
  
  /** Ensure hardware knows which buffer to use before starting
   *  Call setDirectBuffer() with current frontBuffer
   */
  if(!hwInterface->setDirectBuffer(frontBuffer, buffer_size)){
    ESP_LOGE(HUB75_I2S_TAG, "Failed to set front buffer before start");
    return false;
  }
  
  /** Start transmission using hardware interface */
  if(!hwInterface->start()){
    ESP_LOGE(HUB75_I2S_TAG, "Failed to start transmission");
    return false;
  }
  
  running = true;
  
  ESP_LOGI(HUB75_I2S_TAG, "HUB75 I2S transmission started");
  return true;
}

void HUB75_I2S_Protocol::stop(){
  if(running && hwInterface){
    hwInterface->stop();
    running = false;
    ESP_LOGI(HUB75_I2S_TAG, "HUB75 I2S transmission stopped");
  }
}

bool HUB75_I2S_Protocol::setBuffer(const uint16_t* buffer, int size){
  if(!initialized){
    ESP_LOGE(HUB75_I2S_TAG, "Protocol not initialised");
    return false;
  }
  
  if(size != buffer_size){
    ESP_LOGE(HUB75_I2S_TAG, "Buffer size mismatch: expected %d, got %d", buffer_size, size);
    return false;
  }
  
  // Copy buffer to front buffer
  std::memcpy(frontBuffer, buffer, size * sizeof(uint16_t));
  
  /** Update hardware interface to use front buffer */
  if(!hwInterface->setDirectBuffer(frontBuffer, buffer_size)){
    ESP_LOGE(HUB75_I2S_TAG, "Failed to set buffer in hardware interface");
    return false;
  }
  
  return true;
}

bool HUB75_I2S_Protocol::swapBuffer(const uint16_t* buffer, int size){
  if(!initialized){
    ESP_LOGE(HUB75_I2S_TAG, "Protocol not initialised");
    return false;
  }
  
  // If buffer is provided, copy it to back buffer
  if(buffer && size > 0){
    if(size != buffer_size){
      ESP_LOGE(HUB75_I2S_TAG, "Buffer size mismatch: expected %d, got %d", buffer_size, size);
      return false;
    }
    std::memcpy(backBuffer, buffer, size * sizeof(uint16_t));
  }
  // If buffer is nullptr, assume driver wrote directly to back buffer via getWritableBuffer()
  
  /** Swap buffers in the buffer manager */
  if(!bufferManager->swapBuffers()){
    ESP_LOGE(HUB75_I2S_TAG, "Failed to swap buffers in buffer manager");
    return false;
  }
  
  /** Update local pointers */
  frontBuffer = bufferManager->getFrontBuffer();
  backBuffer = bufferManager->getBackBuffer();
  
  /** Update hardware interface to use new front buffer */
  if(!hwInterface->swapBuffer(frontBuffer, buffer_size)){
    ESP_LOGE(HUB75_I2S_TAG, "Failed to swap buffer in hardware interface");
    return false;
  }
  
  return true;
}

uint16_t* HUB75_I2S_Protocol::getWritableBuffer(){
  if(!initialized){
    ESP_LOGE(HUB75_I2S_TAG, "Protocol not initialised");
    return nullptr;
  }
  
  // Return back buffer for direct writing
  return backBuffer;
}

const char* HUB75_I2S_Protocol::getBackendName() const{
  if(hwInterface){
    return hwInterface->getBackendName();
  }
  return "I2S (uninitialized)";
}

} // namespace arcos::abstraction::drivers

