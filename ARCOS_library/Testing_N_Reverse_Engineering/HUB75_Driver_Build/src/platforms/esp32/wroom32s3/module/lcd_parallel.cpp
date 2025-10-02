#include "lcd_parallel.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_rom_gpio.h"
#include "esp_private/periph_ctrl.h"
#include "soc/lcd_cam_struct.h"
#include "soc/gpio_sig_map.h"
#include "soc/soc_caps.h"
#include "hal/dma_types.h"
#include "hal/gdma_ll.h"
#include "esp_private/gdma.h"

namespace arcos::abstraction{

static const char* TAG = "LCD_PARALLEL";

LcdParallel::LcdParallel() 
  : dma_chan(nullptr)
  , dma_descriptors(nullptr)
  , desc_count(0)
  , initialized(false)
  , running(false)
  , config()
  , buffer(nullptr)
  , buffer_len(0)
{
}

LcdParallel::~LcdParallel() {
  stop();
  if (dma_descriptors) {
    heap_caps_free(reinterpret_cast<dma_descriptor_t*>(dma_descriptors));
  }
  if (dma_chan) {
    gdma_del_channel(reinterpret_cast<gdma_channel_handle_t>(dma_chan));
  }
}

LcdParallelConfig LcdParallel::getDefaultConfig(){
  return LcdParallelConfig{};  // Uses default member initializers
}

bool LcdParallel::init(const PinNumber* data_pins, const LcdParallelConfig& config){
  if(initialized){
    ESP_LOGW(TAG, "LCD parallel already initialized");
    return true;
  }
  
  if(!data_pins){
    ESP_LOGE(TAG, "Data pins cannot be nullptr");
    return false;
  }
  
  /** Store configuration */
  this->config = config;
  
  /** Also populate the hw_config for interface compliance */
  hw_config.clock_freq_hz = config.clock_freq_hz;
  hw_config.invert_clock = config.invert_clock;
  hw_config.continuous_mode = config.continuous_mode;
  hw_config.data_width = config.data_width;
  hw_config.clock_pin = config.clock_pin;
  
  ESP_LOGI(TAG, "Initializing ESP32-S3 LCD peripheral for %d-bit parallel output", this->config.data_width);
  ESP_LOGI(TAG, "Target frequency: %d Hz", this->config.clock_freq_hz);
  
  /** Enable LCD_CAM peripheral */
  periph_module_enable(PERIPH_LCD_CAM_MODULE);
  periph_module_reset(PERIPH_LCD_CAM_MODULE);
  
  /** Reset LCD peripheral */
  LCD_CAM.lcd_user.lcd_reset = 1;
  esp_rom_delay_us(1000);
  LCD_CAM.lcd_user.lcd_reset = 0;
  
  /** Calculate clock divider for target frequency */
  uint32_t base_freq = 160000000; // 160MHz PLL_F160M_CLK
  uint32_t divider = base_freq / this->config.clock_freq_hz;
  if(divider < 2) divider = 2;    // Minimum divider
  if(divider > 255) divider = 255; // Maximum divider
  
  /** Configure LCD clock */
  LCD_CAM.lcd_clock.lcd_clk_sel = 3;                    // PLL_F160M_CLK
  LCD_CAM.lcd_clock.lcd_ck_out_edge = this->config.invert_clock ? 1 : 0;  // Clock edge
  LCD_CAM.lcd_clock.lcd_ck_idle_edge = 0;               // Idle low
  LCD_CAM.lcd_clock.lcd_clkcnt_n = 1;                   // N counter
  LCD_CAM.lcd_clock.lcd_clk_equ_sysclk = 0;             // Use divider
  LCD_CAM.lcd_clock.lcd_clkm_div_num = divider;         // Calculated divider
  LCD_CAM.lcd_clock.lcd_clkm_div_a = 1;                 // A divider
  LCD_CAM.lcd_clock.lcd_clkm_div_b = 0;                 // B divider
  
  uint32_t actual_freq = base_freq / divider;
  ESP_LOGI(TAG, "Actual LCD clock frequency: %d Hz (divider: %d)", actual_freq, divider);
  
  /** Configure LCD control registers for 16-bit parallel mode */
  LCD_CAM.lcd_ctrl.lcd_rgb_mode_en = 0;     // i8080 interface mode
  LCD_CAM.lcd_rgb_yuv.lcd_conv_bypass = 0;  // Bypass colour conversion
  LCD_CAM.lcd_misc.lcd_next_frame_en = 0;   // Manual frame control
  LCD_CAM.lcd_misc.lcd_bk_en = 1;           // Enable blank
  
  /** Configure data format */
  LCD_CAM.lcd_user.lcd_8bits_order = 0;     // No byte swap
  LCD_CAM.lcd_user.lcd_bit_order = 0;       // MSB first (try normal bit order)
  LCD_CAM.lcd_user.lcd_2byte_en = 1;        // 16-bit mode
  LCD_CAM.lcd_user.lcd_dummy = 0;           // Disable dummy cycles
  LCD_CAM.lcd_user.lcd_dummy_cyclelen = 0;  // No dummy cycles
  LCD_CAM.lcd_user.lcd_cmd = 0;             // No command phase
  
  /** Enable continuous output mode */
  LCD_CAM.lcd_user.lcd_always_out_en = 1;   // Always output data
  LCD_CAM.lcd_data_dout_mode.val = 0;       // No output delay
  
  /** Connect GPIO pins to LCD data signals (only up to data_width) */
  ESP_LOGI(TAG, "Connecting %d GPIO pins to LCD data outputs", this->config.data_width);
  for(int i = 0; i < this->config.data_width && i < 16; i++){
    gpio_num_t pin = static_cast<gpio_num_t>(data_pins[i]);
    if(pin != GPIO_NUM_NC){
      /** Configure GPIO */
      gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
      };
      gpio_config(&io_conf);
      
      /** Connect to LCD data output signal */
      esp_rom_gpio_connect_out_signal(pin, LCD_DATA_OUT0_IDX + i, false, false);
      
      /** Set maximum drive strength */
      gpio_set_drive_capability(pin, GPIO_DRIVE_CAP_3);
      
      ESP_LOGD(TAG, "GPIO %d -> LCD_DATA_OUT%d", (int)pin, i);
    }
  }
  
  /** Configure external clock pin if specified */
  gpio_num_t clock_pin_gpio = static_cast<gpio_num_t>(this->config.clock_pin);
  if(clock_pin_gpio != GPIO_NUM_NC){
    ESP_LOGI(TAG, "Configuring external clock output on GPIO %d", (int)clock_pin_gpio);
    
    /** Configure clock GPIO */
    gpio_config_t clock_conf = {
      .pin_bit_mask = (1ULL << clock_pin_gpio),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&clock_conf);
    
    /** Connect to LCD clock output signal */
    esp_rom_gpio_connect_out_signal(clock_pin_gpio, LCD_PCLK_IDX, false, false);
    
    /** Set maximum drive strength for clean clock signal */
    gpio_set_drive_capability(clock_pin_gpio, GPIO_DRIVE_CAP_3);
    
    ESP_LOGI(TAG, "GPIO %d -> LCD_PCLK (External Clock Output)", (int)clock_pin_gpio);
  } else {
    ESP_LOGD(TAG, "No external clock pin configured - clock stays internal");
  }
  
  /** Allocate GDMA channel */
  gdma_channel_alloc_config_t dma_config = {
    .sibling_chan = nullptr,
    .direction = GDMA_CHANNEL_DIRECTION_TX,
    .flags = {
      .reserve_sibling = 0,
      .isr_cache_safe = 0
    }
  };
  
  gdma_channel_handle_t esp32_dma_chan = nullptr;
  esp_err_t ret = gdma_new_ahb_channel(&dma_config, &esp32_dma_chan);
  if(ret != ESP_OK){
    ESP_LOGE(TAG, "Failed to allocate GDMA channel: %s", esp_err_to_name(ret));
    return false;
  }
  dma_chan = reinterpret_cast<PlatformDmaChannel*>(esp32_dma_chan);
  
  /** Connect GDMA to LCD peripheral */
  ret = gdma_connect(esp32_dma_chan, GDMA_MAKE_TRIGGER(GDMA_TRIG_PERIPH_LCD, 0));
  if(ret != ESP_OK){
    ESP_LOGE(TAG, "Failed to connect GDMA to LCD: %s", esp_err_to_name(ret));
    return false;
  }
  
  /** Configure GDMA transfer settings */
  gdma_transfer_config_t transfer_config = {
    .max_data_burst_size = 16,
    .access_ext_mem = false
  };
  gdma_config_transfer(esp32_dma_chan, &transfer_config);
  
  initialized = true;
  ESP_LOGI(TAG, "LCD parallel interface initialized successfully");
  return true;
}

bool LcdParallel::setBuffer(uint16_t* buffer, size_t buffer_len){
  if(!initialized){
    ESP_LOGE(TAG, "LCD parallel not initialized");
    return false;
  }
  
  if(!buffer || buffer_len == 0){
    ESP_LOGE(TAG, "Invalid buffer parameters");
    return false;
  }
  
  /** Stop any ongoing transfer */
  if(running){
    stop();
  }
  
  /** Free previous descriptors if they exist */
  if(dma_descriptors){
    heap_caps_free(dma_descriptors);
    dma_descriptors = nullptr;
  }
  
  /** Store buffer information */
  this->buffer = buffer;
  this->buffer_len = buffer_len;
  
  /** Calculate number of DMA descriptors needed */
  size_t buffer_bytes = buffer_len * sizeof(uint16_t);
  const size_t max_desc_size = 4092; // Maximum DMA descriptor size
  desc_count = (buffer_bytes + max_desc_size - 1) / max_desc_size;
  
  ESP_LOGI(TAG, "Setting buffer: %d samples (%d bytes), DMA descriptors: %d", 
           buffer_len, buffer_bytes, desc_count);
  
  /** Allocate DMA descriptors */
  dma_descriptor_t* esp32_descriptors = static_cast<dma_descriptor_t*>(
    heap_caps_malloc(desc_count * sizeof(dma_descriptor_t), MALLOC_CAP_DMA));
  if(!esp32_descriptors){
    ESP_LOGE(TAG, "Failed to allocate DMA descriptors");
    return false;
  }
  dma_descriptors = reinterpret_cast<PlatformDmaDescriptor*>(esp32_descriptors);
  
  /** Setup DMA descriptor chain */
  uint8_t* buf_ptr = reinterpret_cast<uint8_t*>(buffer);
  size_t remaining = buffer_bytes;
  
  for(size_t i = 0; i < desc_count; i++){
    size_t chunk_size = (remaining > max_desc_size) ? max_desc_size : remaining;
    
    esp32_descriptors[i].dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
    esp32_descriptors[i].dw0.suc_eof = (i == desc_count - 1) ? 1 : 0;
    esp32_descriptors[i].dw0.length = chunk_size;
    esp32_descriptors[i].buffer = buf_ptr;
    
    /** Create circular linked list for continuous operation if enabled */
    if(config.continuous_mode){
      if(i == desc_count - 1){
        esp32_descriptors[i].next = &esp32_descriptors[0]; // Loop back
      } else {
        esp32_descriptors[i].next = &esp32_descriptors[i + 1];
      }
    } else {
      esp32_descriptors[i].next = (i == desc_count - 1) ? nullptr : &esp32_descriptors[i + 1];
    }
    
    buf_ptr += chunk_size;
    remaining -= chunk_size;
    
    ESP_LOGD(TAG, "DMA desc[%d]: %d bytes at %p", i, chunk_size, esp32_descriptors[i].buffer);
  }
  
  ESP_LOGI(TAG, "Buffer set successfully");
  return true;
}

bool LcdParallel::setDirectBuffer(uint16_t* buffer_ptr, size_t buffer_len){
  return setBuffer(buffer_ptr, buffer_len);  // Simply call setBuffer - it already works with external pointers
}

bool LcdParallel::swapBuffer(uint16_t* new_buffer_ptr, size_t buffer_len){
  if(!initialized){
    ESP_LOGE(TAG, "LCD parallel not initialized");
    return false;
  }
  
  if(!new_buffer_ptr || buffer_len == 0){
    ESP_LOGE(TAG, "Invalid buffer parameters");
    return false;
  }
  
  if(buffer_len != this->buffer_len){
    ESP_LOGE(TAG, "Buffer size mismatch: expected %d, got %d", this->buffer_len, buffer_len);
    return false;
  }
  
  if(!dma_descriptors || desc_count == 0){
    ESP_LOGE(TAG, "No DMA descriptors available");
    return false;
  }
  
  ESP_LOGD(TAG, "Swapping buffer seamlessly (no transmission stop)");
  
  this->buffer = new_buffer_ptr;  // Update buffer pointer
  
  /* Update DMA descriptor chain to point to new buffer */
  dma_descriptor_t* esp32_descriptors = reinterpret_cast<dma_descriptor_t*>(dma_descriptors);
  uint8_t* buf_ptr = reinterpret_cast<uint8_t*>(new_buffer_ptr);
  size_t buffer_bytes = buffer_len * sizeof(uint16_t);
  size_t remaining = buffer_bytes;
  const size_t max_desc_size = 4092;
  
  for(size_t i = 0; i < desc_count; i++){
    size_t chunk_size = (remaining > max_desc_size) ? max_desc_size : remaining;
    
    esp32_descriptors[i].buffer = buf_ptr;    // Update buffer pointer in descriptor
    /* Keep other descriptor settings (owner, eof, length, next) unchanged */
    
    buf_ptr += chunk_size;
    remaining -= chunk_size;
  }
  
  ESP_LOGD(TAG, "Buffer swapped successfully - new buffer at %p", new_buffer_ptr);
  return true;
}

uint16_t* LcdParallel::getDirectBuffer() const{
  return buffer;
}

size_t LcdParallel::getBufferSize() const{
  return buffer_len;
}

bool LcdParallel::start(){
  if(!initialized){
    ESP_LOGE(TAG, "LCD parallel not initialized");
    return false;
  }
  
  if(!dma_descriptors){
    ESP_LOGE(TAG, "No buffer set - call setBuffer() first");
    return false;
  }
  
  if(running){
    ESP_LOGW(TAG, "LCD parallel already running");
    return true;
  }
  
  ESP_LOGI(TAG, "Starting LCD parallel DMA transfer");
  
  /** Reset LCD FIFO */
  LCD_CAM.lcd_misc.lcd_afifo_reset = 1;
  LCD_CAM.lcd_misc.lcd_afifo_reset = 0;
  
  /** Enable LCD output */
  LCD_CAM.lcd_user.lcd_dout = 1;
  LCD_CAM.lcd_user.lcd_update = 1;
  
  /** Start GDMA with descriptor chain */
  dma_descriptor_t* esp32_descriptors = reinterpret_cast<dma_descriptor_t*>(dma_descriptors);
  gdma_channel_handle_t esp32_dma_chan = reinterpret_cast<gdma_channel_handle_t>(dma_chan);
  esp_err_t ret = gdma_start(esp32_dma_chan, reinterpret_cast<intptr_t>(&esp32_descriptors[0]));
  if(ret != ESP_OK){
    ESP_LOGE(TAG, "Failed to start GDMA: %s", esp_err_to_name(ret));
    return false;
  }
  
  /** Small delay before triggering LCD */
  esp_rom_delay_us(100);
  
  /** Start LCD transmission */
  LCD_CAM.lcd_user.lcd_start = 1;
  
  running = true;
  ESP_LOGI(TAG, "LCD parallel DMA transfer started");
  return true;
}

void LcdParallel::stop(){
  if(!initialized || !running){
    return;
  }
  
  ESP_LOGI(TAG, "Stopping LCD parallel DMA transfer");
  
  /** Stop LCD transmission */
  LCD_CAM.lcd_user.lcd_start = 0;
  LCD_CAM.lcd_user.lcd_dout = 0;
  
  /** Stop GDMA */
  if(dma_chan){
    gdma_channel_handle_t esp32_dma_chan = reinterpret_cast<gdma_channel_handle_t>(dma_chan);
    gdma_stop(esp32_dma_chan);
  }
  
  running = false;
  ESP_LOGI(TAG, "LCD parallel DMA transfer stopped");
}

bool LcdParallel::isRunning() const {
  return running;
}

const LcdParallelConfig* LcdParallel::getLegacyConfig() const {
  return initialized ? &config : nullptr;
}

const ParallelHardwareConfig* LcdParallel::getConfig() const {
  return initialized ? &hw_config : nullptr;
}

bool LcdParallel::init(const PinNumber* data_pins, const ParallelHardwareConfig& config){
  /** Convert to LcdParallelConfig and call legacy init */
  LcdParallelConfig legacy_config;
  legacy_config.clock_freq_hz = config.clock_freq_hz;
  legacy_config.invert_clock = config.invert_clock;
  legacy_config.continuous_mode = config.continuous_mode;
  legacy_config.data_width = config.data_width;
  legacy_config.clock_pin = config.clock_pin;
  
  return init(data_pins, legacy_config);
}

} // namespace arcos::abstraction