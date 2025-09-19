#include "lcd_parallel.hpp"
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
    heap_caps_free(dma_descriptors);
  }
  if (dma_chan) {
    gdma_del_channel(dma_chan);
  }
}

LcdParallelConfig LcdParallel::getDefaultConfig(){
  return LcdParallelConfig{};  // Uses default member initializers
}

bool LcdParallel::init(const gpio_num_t* data_pins, const LcdParallelConfig& config){
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
  LCD_CAM.lcd_rgb_yuv.lcd_conv_bypass = 0;  // Bypass color conversion
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
    if(data_pins[i] != GPIO_NUM_NC){
      /** Configure GPIO */
      gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << data_pins[i]),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
      };
      gpio_config(&io_conf);
      
      /** Connect to LCD data output signal */
      esp_rom_gpio_connect_out_signal(data_pins[i], LCD_DATA_OUT0_IDX + i, false, false);
      
      /** Set maximum drive strength */
      gpio_set_drive_capability(data_pins[i], GPIO_DRIVE_CAP_3);
      
      ESP_LOGD(TAG, "GPIO %d -> LCD_DATA_OUT%d", data_pins[i], i);
    }
  }
  
  /** Configure external clock pin if specified */
  if(this->config.clock_pin != GPIO_NUM_NC){
    ESP_LOGI(TAG, "Configuring external clock output on GPIO %d", this->config.clock_pin);
    
    /** Configure clock GPIO */
    gpio_config_t clock_conf = {
      .pin_bit_mask = (1ULL << this->config.clock_pin),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&clock_conf);
    
    /** Connect to LCD clock output signal */
    esp_rom_gpio_connect_out_signal(this->config.clock_pin, LCD_PCLK_IDX, false, false);
    
    /** Set maximum drive strength for clean clock signal */
    gpio_set_drive_capability(this->config.clock_pin, GPIO_DRIVE_CAP_3);
    
    ESP_LOGI(TAG, "GPIO %d -> LCD_PCLK (External Clock Output)", this->config.clock_pin);
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
  
  esp_err_t ret = gdma_new_ahb_channel(&dma_config, &dma_chan);
  if(ret != ESP_OK){
    ESP_LOGE(TAG, "Failed to allocate GDMA channel: %s", esp_err_to_name(ret));
    return false;
  }
  
  /** Connect GDMA to LCD peripheral */
  ret = gdma_connect(dma_chan, GDMA_MAKE_TRIGGER(GDMA_TRIG_PERIPH_LCD, 0));
  if(ret != ESP_OK){
    ESP_LOGE(TAG, "Failed to connect GDMA to LCD: %s", esp_err_to_name(ret));
    return false;
  }
  
  /** Configure GDMA transfer settings */
  gdma_transfer_config_t transfer_config = {
    .max_data_burst_size = 16,
    .access_ext_mem = false
  };
  gdma_config_transfer(dma_chan, &transfer_config);
  
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
  dma_descriptors = static_cast<dma_descriptor_t*>(
    heap_caps_malloc(desc_count * sizeof(dma_descriptor_t), MALLOC_CAP_DMA));
  if(!dma_descriptors){
    ESP_LOGE(TAG, "Failed to allocate DMA descriptors");
    return false;
  }
  
  /** Setup DMA descriptor chain */
  uint8_t* buf_ptr = reinterpret_cast<uint8_t*>(buffer);
  size_t remaining = buffer_bytes;
  
  for(size_t i = 0; i < desc_count; i++){
    size_t chunk_size = (remaining > max_desc_size) ? max_desc_size : remaining;
    
    dma_descriptors[i].dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
    dma_descriptors[i].dw0.suc_eof = (i == desc_count - 1) ? 1 : 0;
    dma_descriptors[i].dw0.length = chunk_size;
    dma_descriptors[i].buffer = buf_ptr;
    
    /** Create circular linked list for continuous operation if enabled */
    if(config.continuous_mode){
      if(i == desc_count - 1){
        dma_descriptors[i].next = &dma_descriptors[0]; // Loop back
      } else {
        dma_descriptors[i].next = &dma_descriptors[i + 1];
      }
    } else {
      dma_descriptors[i].next = (i == desc_count - 1) ? nullptr : &dma_descriptors[i + 1];
    }
    
    buf_ptr += chunk_size;
    remaining -= chunk_size;
    
    ESP_LOGD(TAG, "DMA desc[%d]: %d bytes at %p", i, chunk_size, dma_descriptors[i].buffer);
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
  uint8_t* buf_ptr = reinterpret_cast<uint8_t*>(new_buffer_ptr);
  size_t buffer_bytes = buffer_len * sizeof(uint16_t);
  size_t remaining = buffer_bytes;
  const size_t max_desc_size = 4092;
  
  for(size_t i = 0; i < desc_count; i++){
    size_t chunk_size = (remaining > max_desc_size) ? max_desc_size : remaining;
    
    dma_descriptors[i].buffer = buf_ptr;    // Update buffer pointer in descriptor
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
  esp_err_t ret = gdma_start(dma_chan, reinterpret_cast<intptr_t>(&dma_descriptors[0]));
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
    gdma_stop(dma_chan);
  }
  
  running = false;
  ESP_LOGI(TAG, "LCD parallel DMA transfer stopped");
}

bool LcdParallel::isRunning() const {
  return running;
}

const LcdParallelConfig* LcdParallel::getConfig() const {
  return initialized ? &config : nullptr;
}