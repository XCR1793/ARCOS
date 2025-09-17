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

/** Static variables for LCD parallel interface */
static gdma_channel_handle_t s_dma_chan = nullptr;
static dma_descriptor_t* s_dma_descriptors = nullptr;
static size_t s_desc_count = 0;
static bool s_initialized = false;
static bool s_running = false;
static LcdParallelConfig s_config;
static uint16_t* s_buffer = nullptr;
static size_t s_buffer_len = 0;

LcdParallelConfig LcdParallel::getDefaultConfig(){
  return LcdParallelConfig{};  // Uses default member initializers
}

bool LcdParallel::init(const gpio_num_t* data_pins, const LcdParallelConfig& config){
  if(s_initialized){
    ESP_LOGW(TAG, "LCD parallel already initialized");
    return true;
  }
  
  if(!data_pins){
    ESP_LOGE(TAG, "Data pins cannot be nullptr");
    return false;
  }
  
  /** Store configuration */
  s_config = config;
  
  ESP_LOGI(TAG, "Initializing ESP32-S3 LCD peripheral for %d-bit parallel output", s_config.data_width);
  ESP_LOGI(TAG, "Target frequency: %d Hz", s_config.clock_freq_hz);
  
  /** Enable LCD_CAM peripheral */
  periph_module_enable(PERIPH_LCD_CAM_MODULE);
  periph_module_reset(PERIPH_LCD_CAM_MODULE);
  
  /** Reset LCD peripheral */
  LCD_CAM.lcd_user.lcd_reset = 1;
  esp_rom_delay_us(1000);
  LCD_CAM.lcd_user.lcd_reset = 0;
  
  /** Calculate clock divider for target frequency */
  uint32_t base_freq = 160000000; // 160MHz PLL_F160M_CLK
  uint32_t divider = base_freq / s_config.clock_freq_hz;
  if(divider < 2) divider = 2;    // Minimum divider
  if(divider > 255) divider = 255; // Maximum divider
  
  /** Configure LCD clock */
  LCD_CAM.lcd_clock.lcd_clk_sel = 3;                    // PLL_F160M_CLK
  LCD_CAM.lcd_clock.lcd_ck_out_edge = s_config.invert_clock ? 1 : 0;  // Clock edge
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
  LCD_CAM.lcd_user.lcd_bit_order = 0;       // MSB first
  LCD_CAM.lcd_user.lcd_2byte_en = 1;        // 16-bit mode
  LCD_CAM.lcd_user.lcd_dummy = 1;           // Enable dummy cycles
  LCD_CAM.lcd_user.lcd_dummy_cyclelen = 1;  // 2 dummy cycles
  LCD_CAM.lcd_user.lcd_cmd = 0;             // No command phase
  
  /** Enable continuous output mode */
  LCD_CAM.lcd_user.lcd_always_out_en = 1;   // Always output data
  LCD_CAM.lcd_data_dout_mode.val = 0;       // No output delay
  
  /** Connect GPIO pins to LCD data signals (only up to data_width) */
  ESP_LOGI(TAG, "Connecting %d GPIO pins to LCD data outputs", s_config.data_width);
  for(int i = 0; i < s_config.data_width && i < 16; i++){
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
  
  /** Allocate GDMA channel */
  gdma_channel_alloc_config_t dma_config = {
    .sibling_chan = nullptr,
    .direction = GDMA_CHANNEL_DIRECTION_TX,
    .flags = {
      .reserve_sibling = 0
    }
  };
  
  esp_err_t ret = gdma_new_channel(&dma_config, &s_dma_chan);
  if(ret != ESP_OK){
    ESP_LOGE(TAG, "Failed to allocate GDMA channel: %s", esp_err_to_name(ret));
    return false;
  }
  
  /** Connect GDMA to LCD peripheral */
  ret = gdma_connect(s_dma_chan, GDMA_MAKE_TRIGGER(GDMA_TRIG_PERIPH_LCD, 0));
  if(ret != ESP_OK){
    ESP_LOGE(TAG, "Failed to connect GDMA to LCD: %s", esp_err_to_name(ret));
    return false;
  }
  
  /** Configure GDMA transfer settings */
  gdma_transfer_ability_t ability = {
    .sram_trans_align = 32,
    .psram_trans_align = 64,
  };
  gdma_set_transfer_ability(s_dma_chan, &ability);
  
  s_initialized = true;
  ESP_LOGI(TAG, "LCD parallel interface initialized successfully");
  return true;
}

bool LcdParallel::setBuffer(uint16_t* buffer, size_t buffer_len){
  if(!s_initialized){
    ESP_LOGE(TAG, "LCD parallel not initialized");
    return false;
  }
  
  if(!buffer || buffer_len == 0){
    ESP_LOGE(TAG, "Invalid buffer parameters");
    return false;
  }
  
  /** Stop any ongoing transfer */
  if(s_running){
    stop();
  }
  
  /** Free previous descriptors if they exist */
  if(s_dma_descriptors){
    heap_caps_free(s_dma_descriptors);
    s_dma_descriptors = nullptr;
  }
  
  /** Store buffer information */
  s_buffer = buffer;
  s_buffer_len = buffer_len;
  
  /** Calculate number of DMA descriptors needed */
  size_t buffer_bytes = buffer_len * sizeof(uint16_t);
  const size_t max_desc_size = 4092; // Maximum DMA descriptor size
  s_desc_count = (buffer_bytes + max_desc_size - 1) / max_desc_size;
  
  ESP_LOGI(TAG, "Setting buffer: %d samples (%d bytes), DMA descriptors: %d", 
           buffer_len, buffer_bytes, s_desc_count);
  
  /** Allocate DMA descriptors */
  s_dma_descriptors = static_cast<dma_descriptor_t*>(
    heap_caps_malloc(s_desc_count * sizeof(dma_descriptor_t), MALLOC_CAP_DMA));
  if(!s_dma_descriptors){
    ESP_LOGE(TAG, "Failed to allocate DMA descriptors");
    return false;
  }
  
  /** Setup DMA descriptor chain */
  uint8_t* buf_ptr = reinterpret_cast<uint8_t*>(buffer);
  size_t remaining = buffer_bytes;
  
  for(size_t i = 0; i < s_desc_count; i++){
    size_t chunk_size = (remaining > max_desc_size) ? max_desc_size : remaining;
    
    s_dma_descriptors[i].dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
    s_dma_descriptors[i].dw0.suc_eof = (i == s_desc_count - 1) ? 1 : 0;
    s_dma_descriptors[i].dw0.length = chunk_size;
    s_dma_descriptors[i].buffer = buf_ptr;
    
    /** Create circular linked list for continuous operation if enabled */
    if(s_config.continuous_mode){
      if(i == s_desc_count - 1){
        s_dma_descriptors[i].next = &s_dma_descriptors[0]; // Loop back
      } else {
        s_dma_descriptors[i].next = &s_dma_descriptors[i + 1];
      }
    } else {
      s_dma_descriptors[i].next = (i == s_desc_count - 1) ? nullptr : &s_dma_descriptors[i + 1];
    }
    
    buf_ptr += chunk_size;
    remaining -= chunk_size;
    
    ESP_LOGD(TAG, "DMA desc[%d]: %d bytes at %p", i, chunk_size, s_dma_descriptors[i].buffer);
  }
  
  ESP_LOGI(TAG, "Buffer set successfully");
  return true;
}

bool LcdParallel::start(){
  if(!s_initialized){
    ESP_LOGE(TAG, "LCD parallel not initialized");
    return false;
  }
  
  if(!s_dma_descriptors){
    ESP_LOGE(TAG, "No buffer set - call setBuffer() first");
    return false;
  }
  
  if(s_running){
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
  esp_err_t ret = gdma_start(s_dma_chan, reinterpret_cast<intptr_t>(&s_dma_descriptors[0]));
  if(ret != ESP_OK){
    ESP_LOGE(TAG, "Failed to start GDMA: %s", esp_err_to_name(ret));
    return false;
  }
  
  /** Small delay before triggering LCD */
  esp_rom_delay_us(100);
  
  /** Start LCD transmission */
  LCD_CAM.lcd_user.lcd_start = 1;
  
  s_running = true;
  ESP_LOGI(TAG, "LCD parallel DMA transfer started");
  return true;
}

void LcdParallel::stop(){
  if(!s_initialized || !s_running){
    return;
  }
  
  ESP_LOGI(TAG, "Stopping LCD parallel DMA transfer");
  
  /** Stop LCD transmission */
  LCD_CAM.lcd_user.lcd_start = 0;
  LCD_CAM.lcd_user.lcd_dout = 0;
  
  /** Stop GDMA */
  if(s_dma_chan){
    gdma_stop(s_dma_chan);
  }
  
  s_running = false;
  ESP_LOGI(TAG, "LCD parallel DMA transfer stopped");
}

bool LcdParallel::isRunning(){
  return s_running;
}

const LcdParallelConfig* LcdParallel::getConfig(){
  return s_initialized ? &s_config : nullptr;
}