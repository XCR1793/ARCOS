#include "hub75_driver.hpp"
#include "parallel_hardware_interface.hpp"
#include "dma_buffer_manager.hpp"
#include "lcd_parallel.hpp"          // Concrete implementation
#include "parallel_buffer.hpp"       // Concrete implementation
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include <cstring>
#include <cmath>

static const char* TAG = "HUB75_DRIVER";

/** HUB75 protocol bit positions */
#define R0_BIT  0
#define G0_BIT  1
#define B0_BIT  2
#define R1_BIT  3
#define G1_BIT  4
#define B1_BIT  5
#define LAT_BIT 6
#define OE_BIT  7
#define A_BIT   8
#define B_BIT   9
#define C_BIT   10
#define D_BIT   11
#define E_BIT   12
#define OE2_BIT 13  // Second OE pin

HUB75Driver::HUB75Driver()
  : hwInterface(nullptr)
  , bufferManager(nullptr)
  , owns_hardware(false)
  , owns_buffer_manager(false)
  , default_hw_impl(nullptr)
  , default_buffer_impl(nullptr)
  , frontBuffer(nullptr)
  , backBuffer(nullptr)
  , oe_pin2(GPIO_NUM_NC)
  , initialized(false)
  , running(false)
  , framebuffer(nullptr)
  , buffer_size(0)
  , base_buffer_size(0)
{
  // Initialise gamma table with compile-time values for optimal cache performance
  memcpy(gamma_table, GAMMA_TABLE_22, sizeof(gamma_table));
}

HUB75Driver::~HUB75Driver(){
  stop();
  if(framebuffer){
    heap_caps_free(framebuffer);
  }
  
  // Clean up owned resources (cast opaque pointers to concrete types)
  if(owns_hardware && default_hw_impl){
    delete static_cast<LcdParallel*>(default_hw_impl);
  }
  if(owns_buffer_manager && default_buffer_impl){
    delete static_cast<ParallelBuffer*>(default_buffer_impl);
  }
}

bool HUB75Driver::init(const HUB75Config& cfg, IParallelHardware* hardware, IDmaBufferManager* buffer_mgr){
  if(initialized){
    ESP_LOGW(TAG, "Driver already initialised");
    return true;
  }
  
  config = cfg;
  
  // Use provided interfaces or create defaults
  if(hardware){
    hwInterface = hardware;
    owns_hardware = false;
  } else {
    // Create default hardware implementation (concrete type only known here)
    LcdParallel* lcd_impl = new LcdParallel();
    default_hw_impl = lcd_impl;  // Store as opaque pointer
    hwInterface = lcd_impl;       // Store as interface pointer
    owns_hardware = true;
  }
  
  if(buffer_mgr){
    bufferManager = buffer_mgr;
    owns_buffer_manager = false;
  } else {
    // Create default buffer implementation (concrete type only known here)
    ParallelBuffer* buffer_impl = new ParallelBuffer();
    default_buffer_impl = buffer_impl;  // Store as opaque pointer
    bufferManager = buffer_impl;         // Store as interface pointer
    owns_buffer_manager = true;
  }
  
  /** Calculate buffer sizes for new BCM protocol
   *  For each row (height/2):
   *    For each color buffer (colour_depth):
   *      - pixels per row (depends on expansion mode)
   *      - 1 delay bit
   *    Total per row = colour_depth * (pixels_per_row + 1)
   *  Total buffer = rows * colour_depth * (pixels_per_row + 1)
   *
   *  Expansion modes:
   *  - SINGLE: 64 pixels per row
   *  - PARALLEL_OE: 64 * panel_count pixels (data clocked to each panel)
   *  - SERIES_CHAIN: 64 * panel_count pixels (data flows through panels)
   */
  const int hub75_rows = config.matrix_height / 2;
  
  // Determine pixels per row based on expansion mode
  int pixels_per_row = config.matrix_width;
  if(config.expansion_mode == HUB75Config::ExpansionMode::PARALLEL_OE){
    pixels_per_row = config.matrix_width * config.panel_count;
    config.effective_width = pixels_per_row;
  } else if(config.expansion_mode == HUB75Config::ExpansionMode::SERIES_CHAIN){
    pixels_per_row = config.matrix_width * config.panel_count;
    config.effective_width = pixels_per_row;
  } else if(config.dual_display_mode){
    // Legacy dual display mode (backward compatibility)
    pixels_per_row = 128;
    config.expansion_mode = HUB75Config::ExpansionMode::PARALLEL_OE;
    config.panel_count = 2;
  }
  
  // Buffer size calculation for BCM timing:
  // PARALLEL_OE mode: BCM timing happens per-panel (after each 64-pixel latch)
  // SERIES_CHAIN/SINGLE: BCM timing happens once at end
  // BCM timing varies: 1,2,4,8,16 samples for planes 0-4 = 31 total
  // Delay samples: 3 per plane × 5 planes = 15
  int total_bcm_samples;
  if(config.expansion_mode == HUB75Config::ExpansionMode::PARALLEL_OE){
    // BCM per panel: 31 samples × panel_count
    total_bcm_samples = 31 * config.panel_count;
  } else {
    // BCM once at end: 31 samples total
    total_bcm_samples = 31;
  }
  int total_delay_samples = config.colour_depth * 3;  // 3 delay bits per plane
  buffer_size = hub75_rows * (config.colour_depth * pixels_per_row + total_bcm_samples + total_delay_samples);
  
  /** Allocate framebuffer */
  int fb_width = config.dual_display_mode ? config.effective_width : config.matrix_width;
  size_t framebuffer_bytes = fb_width * config.matrix_height * sizeof(RGBPixel);
  framebuffer = static_cast<RGBPixel*>(heap_caps_malloc(framebuffer_bytes, MALLOC_CAP_8BIT));
  if(!framebuffer){
    ESP_LOGE(TAG, "Failed to allocate framebuffer (%d bytes)", framebuffer_bytes);
    return false;
  }
  
  /** Clear framebuffer */
  std::memset(framebuffer, 0, framebuffer_bytes);
  
  /** GPIO pin mapping for HUB75 protocol */
  int num_pins = (config.pins.oe_pin2 >= 0) ? 14 : 13;
  
  /** Prepare GPIO pin array for hardware interface */
  gpio_num_t* lcd_data_pins = new gpio_num_t[num_pins];
  
  lcd_data_pins[0] = static_cast<gpio_num_t>(config.pins.r0_pin);   // R0
  lcd_data_pins[1] = static_cast<gpio_num_t>(config.pins.g0_pin);   // G0
  lcd_data_pins[2] = static_cast<gpio_num_t>(config.pins.b0_pin);   // B0
  lcd_data_pins[3] = static_cast<gpio_num_t>(config.pins.r1_pin);   // R1
  lcd_data_pins[4] = static_cast<gpio_num_t>(config.pins.g1_pin);   // G1
  lcd_data_pins[5] = static_cast<gpio_num_t>(config.pins.b1_pin);   // B1
  lcd_data_pins[6] = static_cast<gpio_num_t>(config.pins.lat_pin);  // LAT
  lcd_data_pins[7] = static_cast<gpio_num_t>(config.pins.oe_pin);   // OE1
  lcd_data_pins[8] = static_cast<gpio_num_t>(config.pins.a_pin);    // A
  lcd_data_pins[9] = static_cast<gpio_num_t>(config.pins.b_pin);    // B
  lcd_data_pins[10] = static_cast<gpio_num_t>(config.pins.c_pin);   // C
  lcd_data_pins[11] = static_cast<gpio_num_t>(config.pins.d_pin);   // D
  lcd_data_pins[12] = static_cast<gpio_num_t>(config.pins.e_pin);   // E
  
  if(config.pins.oe_pin2 >= 0){
    lcd_data_pins[13] = static_cast<gpio_num_t>(config.pins.oe_pin2); // OE2
  }

  /** Allocate DMA buffers using buffer manager */
  DmaBufferConfig buffer_config;
  buffer_config.buffer_count = 2;  // Double buffering
  buffer_config.sample_count = buffer_size;
  buffer_config.mode = BufferMode::DOUBLE_BUFFER;
  buffer_config.auto_allocate = true;
  
  if(!bufferManager->init(buffer_config)){
    ESP_LOGE(TAG, "Failed to initialize buffer manager");
    return false;
  }

  /** Configure hardware interface using abstract ParallelHardwareConfig */
  ParallelHardwareConfig hw_config;
  hw_config.clock_freq_hz = config.clock_freq_hz;
  hw_config.invert_clock = false;
  hw_config.continuous_mode = true;
  hw_config.data_width = num_pins;
  hw_config.clock_pin = static_cast<gpio_num_t>(config.pins.clock_pin);
  hw_config.data_pins = lcd_data_pins;
  hw_config.data_pin_count = num_pins;
  
  /** Initialise hardware interface */
  if(!hwInterface->init(lcd_data_pins, hw_config)){
    ESP_LOGE(TAG, "Failed to initialise hardware interface");
    delete[] lcd_data_pins;
    return false;
  }
  
  delete[] lcd_data_pins;

  /** Set up buffer pointers from buffer manager */
  frontBuffer = bufferManager->getFrontBuffer();
  backBuffer = bufferManager->getBackBuffer();
  
  if(!frontBuffer || !backBuffer){
    ESP_LOGE(TAG, "Failed to get buffer pointers from buffer manager");
    return false;
  }
  
  /** Initialize lookup tables and configurations */
  initializeLUT();
  
  /** Store second OE pin for reference */
  if(config.pins.oe_pin2 >= 0){
    oe_pin2 = static_cast<gpio_num_t>(config.pins.oe_pin2);
    ESP_LOGI(TAG, "Dual OE mode: Primary=%d, Secondary=%d (controlled via DMA buffer)", 
             config.pins.oe_pin, config.pins.oe_pin2);
  }
  
  initialized = true;
  
  ESP_LOGI(TAG, "HUB75 driver initialised:");
  ESP_LOGI(TAG, "  Hardware backend: %s", hwInterface->getBackendName());
  ESP_LOGI(TAG, "  Matrix: %dx%d pixels", config.matrix_width, config.matrix_height);
  ESP_LOGI(TAG, "  Colour depth: %d-bit (%d planes)", config.colour_depth, config.colour_depth);
  ESP_LOGI(TAG, "  Clock: %dMHz", config.clock_freq_hz / 1000000);
  ESP_LOGI(TAG, "  Buffer size: %d samples", buffer_size);
  ESP_LOGI(TAG, "  Buffer mode: %s", 
           bufferManager->getMode() == BufferMode::DOUBLE_BUFFER ? "Double buffered" : "Single buffered");
  
  return true;
}

bool HUB75Driver::init(const HUB75Config& cfg){
  // Use default implementations
  return init(cfg, nullptr, nullptr);
}

bool HUB75Driver::start(){
  if(!initialized){
    ESP_LOGE(TAG, "Driver not initialised");
    return false;
  }
  
  if(running){
    ESP_LOGW(TAG, "Driver already running");
    return true;
  }
  
  /** Convert initial framebuffer and set up front buffer */
  convertFramebufferToHUB75();
  swapBuffers();
  
  /** Start transmission using hardware interface */
  if(!hwInterface->setDirectBuffer(frontBuffer, buffer_size)){
    ESP_LOGE(TAG, "Failed to set front buffer");
    return false;
  }
  
  if(!hwInterface->start()){
    ESP_LOGE(TAG, "Failed to start transmission");
    return false;
  }
  
  running = true;
  
  /** Synchronize secondary OE pin */
  synchronizeOEPins();
  
  ESP_LOGI(TAG, "HUB75 transmission started%s", 
           (oe_pin2 != GPIO_NUM_NC) ? " (dual display mode)" : "");
  return true;
}

void HUB75Driver::stop(){
  if(running && hwInterface){
    hwInterface->stop();
    running = false;
    ESP_LOGI(TAG, "HUB75 transmission stopped%s", 
             (oe_pin2 != GPIO_NUM_NC) ? " (dual display mode)" : "");
  }
}

void HUB75Driver::setPixel(int x, int y, const RGB& colour){
  if(!isValidCoordinate(x, y)){
    return;
  }
  
  int fb_width = config.dual_display_mode ? config.effective_width : config.matrix_width;
  int index = y * fb_width + x;
  framebuffer[index].r = colour.r;
  framebuffer[index].g = colour.g;
  framebuffer[index].b = colour.b;
}

RGB HUB75Driver::getPixel(int x, int y) const{
  if(!isValidCoordinate(x, y)){
    return RGB(0, 0, 0);
  }
  
  int fb_width = config.dual_display_mode ? config.effective_width : config.matrix_width;
  int index = y * fb_width + x;
  return RGB(framebuffer[index].r, framebuffer[index].g, framebuffer[index].b);
}

void HUB75Driver::clear(){
  if(framebuffer){
    int fb_width = config.dual_display_mode ? config.effective_width : config.matrix_width;
    std::memset(framebuffer, 0, fb_width * config.matrix_height * sizeof(RGBPixel));
  }
}

void HUB75Driver::fill(const RGB& colour){
  if(!framebuffer){
    return;
  }
  
  int fb_width = config.dual_display_mode ? config.effective_width : config.matrix_width;
  for(int i = 0; i < fb_width * config.matrix_height; i++){
    framebuffer[i].r = colour.r;
    framebuffer[i].g = colour.g;
    framebuffer[i].b = colour.b;
  }
}

void HUB75Driver::show(){
  if(!initialized || !running){
    return;
  }
  
  /** Convert framebuffer to HUB75 format and swap buffers */
  convertFramebufferToHUB75();
  swapBuffers();
}

bool HUB75Driver::swapBuffers(){
  /** Swap buffers in the buffer manager */
  if(!bufferManager->swapBuffers()){
    ESP_LOGE(TAG, "Failed to swap buffers in buffer manager");
    return false;
  }
  
  /** Update local pointers */
  frontBuffer = bufferManager->getFrontBuffer();
  backBuffer = bufferManager->getBackBuffer();
  
  /** Update hardware interface to use new front buffer */
  if(!hwInterface->swapBuffer(frontBuffer, buffer_size)){
    ESP_LOGE(TAG, "Failed to swap buffer in hardware interface");
    return false;
  }
  
  return true;
}

uint8_t HUB75Driver::convert8to5(uint8_t value){
  uint8_t value_5bit = value >> 3;
  
  /** Apply gamma correction if enabled */
  if(config.enable_gamma_correction){
    return gamma_table[value_5bit];
  }
  
  return value_5bit;
}

uint8_t HUB75Driver::getBitFromValue(uint8_t value5bit, int bit_plane){
  return (value5bit >> bit_plane) & 1;
}

bool HUB75Driver::isValidCoordinate(int x, int y) const{
  int width = config.dual_display_mode ? config.effective_width : config.matrix_width;
  return (x >= 0 && x < width && y >= 0 && y < config.matrix_height);
}

void HUB75Driver::convertFramebufferToHUB75(){
  int buffer_index = 0;
  const int hub75_rows = config.matrix_height / 2;
  int fb_width = config.effective_width;
  
  /** BCM Protocol with Multi-Panel Support:
   *  For each row:
   *    - Go through all color buffers (bit planes)
   *    - For each color buffer, generate pixels for all panels with BCM pattern for OE
   *    - Add 1-bit delay after latching with OE disabled to prevent ghosting
   *  
   *  Expansion Modes:
   *  - SINGLE: 64 pixels per row
   *  - PARALLEL_OE: 64 * panel_count pixels (separate OE per panel)
   *  - SERIES_CHAIN: 64 * panel_count pixels (data flows through panels)
   */
  
  for(int sequence_index = 0; sequence_index < hub75_rows; sequence_index++){
    /** Address and data row mapping
     *  Note: Some panels have row offsets - handled per-panel in pixel reading
     */
    int address_row = sequence_index;
    int data_row = (sequence_index + 1) % hub75_rows;
    
    int upper_row = data_row;
    int lower_row = data_row + hub75_rows;
    
    /** Go through all color buffers for this row */
    for(int plane = 0; plane < config.colour_depth; plane++){
      
      /** Calculate total columns based on expansion mode */
      int total_columns = config.matrix_width * config.panel_count;
      
      /** Generate pixels for this row and color plane */
      for(int col = 0; col < total_columns; col++){
        uint16_t sample = 0;
        
        /** Set address lines for current row */
        if(address_row & (1 << 0)) sample |= (1 << A_BIT);
        if(address_row & (1 << 1)) sample |= (1 << B_BIT);
        if(address_row & (1 << 2)) sample |= (1 << C_BIT);
        if(address_row & (1 << 3)) sample |= (1 << D_BIT);
        
        /** Get pixel data from framebuffer
         *  For multi-panel modes, the framebuffer is laid out as:
         *  - PARALLEL_OE: [Panel0_64px][Panel1_64px]... per row
         *  - SERIES_CHAIN: [Panel0_64px][Panel1_64px][Panel2_64px]... per row
         *  Both use the same linear addressing: fb_width = matrix_width × panel_count
         *  
         *  Row offset correction for both panels:
         *  Both panels need row shift backward by 1 due to hardware addressing offset
         *  
         *  Panel Inversion Support:
         *  Apply horizontal/vertical flips per-panel during coordinate calculation
         */
        RGBPixel upper_pixel, lower_pixel;
        
        // Determine which panel this column belongs to
        int panel_index = col / config.matrix_width;
        int local_col = col % config.matrix_width;
        
        // Apply row offset correction FIRST (shift backward by 1 for hardware addressing)
        int upper_row_corrected = (upper_row - 1 + hub75_rows) % hub75_rows;
        int lower_row_corrected = upper_row_corrected + hub75_rows;
        
        // Apply panel inversion if configured
        int read_col = col;  // Default: no inversion
        int read_upper_row = upper_row_corrected;
        int read_lower_row = lower_row_corrected;
        
        if(panel_index < 4 && (config.panel_inversions[panel_index].flip_horizontal || 
                               config.panel_inversions[panel_index].flip_vertical)){
          // Calculate panel-local coordinates
          int panel_start_col = panel_index * config.matrix_width;
          
          // Horizontal flip: mirror column within panel
          if(config.panel_inversions[panel_index].flip_horizontal){
            local_col = (config.matrix_width - 1) - local_col;
          }
          
          // Vertical flip: swap upper/lower halves and mirror within each half
          if(config.panel_inversions[panel_index].flip_vertical){
            // For HUB75: upper half is rows 0-15, lower half is rows 16-31 (for 32 pixel height)
            // To flip vertically: upper row N becomes lower row (15-N), lower row N becomes upper row (15-N)
            int flipped_row_in_half = (hub75_rows - 1) - upper_row_corrected;
            read_upper_row = flipped_row_in_half + hub75_rows;  // Map to lower half
            read_lower_row = flipped_row_in_half;                // Map to upper half
          }
          
          // Reconstruct global column with flipped local column
          read_col = panel_start_col + local_col;
        }
        
        int upper_row_adjusted = read_upper_row;
        int lower_row_adjusted = read_lower_row;
        
        int upper_index = upper_row_adjusted * fb_width + read_col;
        int lower_index = lower_row_adjusted * fb_width + read_col;
        
        // Bounds check to prevent buffer overrun
        if(upper_index >= 0 && upper_index < fb_width * config.matrix_height &&
           lower_index >= 0 && lower_index < fb_width * config.matrix_height){
          upper_pixel = framebuffer[upper_index];
          lower_pixel = framebuffer[lower_index];
        } else {
          // Out of bounds - use black
          upper_pixel = {0, 0, 0};
          lower_pixel = {0, 0, 0};
        }
        
        /** Upper half pixel data (R0, G0, B0) */
        uint8_t r0_5bit = convert8to5(upper_pixel.r);
        uint8_t g0_5bit = convert8to5(upper_pixel.g);
        uint8_t b0_5bit = convert8to5(upper_pixel.b);
        
        uint8_t r0 = getBitFromValue(r0_5bit, plane);
        uint8_t g0 = getBitFromValue(g0_5bit, plane);
        uint8_t b0 = getBitFromValue(b0_5bit, plane);
        
        /** Lower half pixel data (R1, G1, B1) */
        uint8_t r1_5bit = convert8to5(lower_pixel.r);
        uint8_t g1_5bit = convert8to5(lower_pixel.g);
        uint8_t b1_5bit = convert8to5(lower_pixel.b);
        
        uint8_t r1 = getBitFromValue(r1_5bit, plane);
        uint8_t g1 = getBitFromValue(g1_5bit, plane);
        uint8_t b1 = getBitFromValue(b1_5bit, plane);
        
        /** Set RGB data bits */
        if(r0) sample |= (1 << R0_BIT);
        if(g0) sample |= (1 << G0_BIT);
        if(b0) sample |= (1 << B0_BIT);
        if(r1) sample |= (1 << R1_BIT);
        if(g1) sample |= (1 << G1_BIT);
        if(b1) sample |= (1 << B1_BIT);
        
        /** During pixel clocking: Keep OE disabled (HIGH) to prevent glitches
         *  BCM timing will be applied AFTER all pixels are latched
         *  All panels have OE disabled during data transfer
         */
        if(config.expansion_mode == HUB75Config::ExpansionMode::PARALLEL_OE){
          int panel_index = col / config.matrix_width;
          int local_col = col % config.matrix_width;
          bool is_latch_cycle = (local_col == config.matrix_width - 1);
          
          // OE disabled for ALL panels during pixel clocking
          sample |= (1 << OE_BIT);
          if(config.pins.oe_pin2 >= 0){
            sample |= (1 << OE2_BIT);
          }
          
          // Latch at end of each panel section
          if(is_latch_cycle){
            sample |= (1 << LAT_BIT);
          }
          
        } else if(config.expansion_mode == HUB75Config::ExpansionMode::SERIES_CHAIN){
          bool is_latch_cycle = (col == total_columns - 1);
          
          // OE disabled during pixel clocking
          sample |= (1 << OE_BIT);
          
          // Latch only at the very end of the chain
          if(is_latch_cycle){
            sample |= (1 << LAT_BIT);
          }
          
        } else {
          bool is_latch_cycle = (col == config.matrix_width - 1);
          
          // OE disabled during pixel clocking
          sample |= (1 << OE_BIT);
          
          // Latch at the last column
          if(is_latch_cycle){
            sample |= (1 << LAT_BIT);
          }
        }
        
        backBuffer[buffer_index++] = sample;
        
        /** BCM Timing per-panel for PARALLEL_OE mode
         *  After latching each panel, immediately apply BCM timing
         *  This ensures each panel displays for the correct duration
         */
        if(config.expansion_mode == HUB75Config::ExpansionMode::PARALLEL_OE){
          int panel_index = col / config.matrix_width;
          int local_col = col % config.matrix_width;
          bool is_latch_cycle = (local_col == config.matrix_width - 1);
          
          if(is_latch_cycle){
            // Just latched this panel - now add BCM timing
            int bcm_length = 1 << plane;
            uint16_t bcm_sample = 0;
            
            // Set address lines (same row)
            if(address_row & (1 << 0)) bcm_sample |= (1 << A_BIT);
            if(address_row & (1 << 1)) bcm_sample |= (1 << B_BIT);
            if(address_row & (1 << 2)) bcm_sample |= (1 << C_BIT);
            if(address_row & (1 << 3)) bcm_sample |= (1 << D_BIT);
            
            // Enable OE for THIS panel only
            if(panel_index == 0){
              // Panel 0: OE LOW (enabled), OE2 HIGH (disabled)
              bcm_sample |= (1 << OE2_BIT);
            } else if(panel_index == 1){
              // Panel 1: OE HIGH (disabled), OE2 LOW (enabled)
              bcm_sample |= (1 << OE_BIT);
            }
            
            // Add bcm_length samples with OE enabled for this panel
            for(int bcm_cycle = 0; bcm_cycle < bcm_length; bcm_cycle++){
              backBuffer[buffer_index++] = bcm_sample;
            }
          }
        }
      }
      
      /** BCM Timing for SERIES_CHAIN and SINGLE modes (once at end)
       *  For these modes, all pixels are latched together at the end
       */
      if(config.expansion_mode != HUB75Config::ExpansionMode::PARALLEL_OE){
        int bcm_length = 1 << plane;
        uint16_t bcm_sample = 0;
        
        // Set address lines (same row)
        if(address_row & (1 << 0)) bcm_sample |= (1 << A_BIT);
        if(address_row & (1 << 1)) bcm_sample |= (1 << B_BIT);
        if(address_row & (1 << 2)) bcm_sample |= (1 << C_BIT);
        if(address_row & (1 << 3)) bcm_sample |= (1 << D_BIT);
        
        // OE enabled (LOW) - leave bit at 0
        
        // Add bcm_length samples with OE enabled
        for(int bcm_cycle = 0; bcm_cycle < bcm_length; bcm_cycle++){
          backBuffer[buffer_index++] = bcm_sample;
        }
      }
      
      /** Add delay bits after BCM timing with OE disabled and latch cleared
       *  This prevents ghosting between bit planes, especially during brightness transitions
       *  Using 3 delay bits to give panels time to fully discharge
       */
      uint16_t delay_sample = 0;
      
      // Set address lines (keep same row address)
      if(address_row & (1 << 0)) delay_sample |= (1 << A_BIT);
      if(address_row & (1 << 1)) delay_sample |= (1 << B_BIT);
      if(address_row & (1 << 2)) delay_sample |= (1 << C_BIT);
      if(address_row & (1 << 3)) delay_sample |= (1 << D_BIT);
      
      // OE disabled (high) - keep panels off during transition
      delay_sample |= (1 << OE_BIT);
      if(config.pins.oe_pin2 >= 0){
        delay_sample |= (1 << OE2_BIT);
      }
      
      // Add 3 delay bits to give panels time to fully discharge
      for(int i = 0; i < 3; i++){
        backBuffer[buffer_index++] = delay_sample;
      }
    }
  }
}

/** Advanced framebuffer operations */
FrameBuffer HUB75Driver::getFrameBuffer() const{
  FrameBuffer buffer;
  buffer.width = config.matrix_width;
  buffer.height = config.matrix_height;
  buffer.format = FrameBuffer::RGB888;
  
  /** Allocate and copy pixel data */
  size_t pixel_count = buffer.width * buffer.height;
  buffer.pixels = new RGB[pixel_count];
  
  for(size_t i = 0; i < pixel_count; i++){
    buffer.pixels[i].r = framebuffer[i].r;
    buffer.pixels[i].g = framebuffer[i].g;
    buffer.pixels[i].b = framebuffer[i].b;
  }
  
  return buffer;
}

bool HUB75Driver::setFrameBuffer(const FrameBuffer& buffer){
  if(!validateConfig(config) || !isValidBufferSize(buffer.width, buffer.height)){
    ESP_LOGE(TAG, "Invalid buffer dimensions: %dx%d", buffer.width, buffer.height);
    return false;
  }
  
  if(buffer.format != FrameBuffer::RGB888){
    ESP_LOGE(TAG, "Unsupported buffer format");
    return false;
  }
  
  /** Copy pixel data to internal framebuffer */
  size_t pixel_count = buffer.width * buffer.height;
  for(size_t i = 0; i < pixel_count; i++){
    framebuffer[i].r = buffer.pixels[i].r;
    framebuffer[i].g = buffer.pixels[i].g;
    framebuffer[i].b = buffer.pixels[i].b;
  }
  
  return true;
}

bool HUB75Driver::uploadFrameBuffer(const RGB* pixels, int width, int height){
  if(!pixels || !isValidBufferSize(width, height)){
    ESP_LOGE(TAG, "Invalid buffer parameters");
    return false;
  }
  
  /** Direct copy from RGB array */
  size_t pixel_count = width * height;
  for(size_t i = 0; i < pixel_count; i++){
    framebuffer[i].r = pixels[i].r;
    framebuffer[i].g = pixels[i].g;
    framebuffer[i].b = pixels[i].b;
  }
  
  return true;
}

void HUB75Driver::copyFrameBuffer(RGB* destination) const{
  if(!destination || !framebuffer){
    ESP_LOGE(TAG, "Invalid destination buffer");
    return;
  }
  
  size_t pixel_count = config.matrix_width * config.matrix_height;
  for(size_t i = 0; i < pixel_count; i++){
    destination[i].r = framebuffer[i].r;
    destination[i].g = framebuffer[i].g;
    destination[i].b = framebuffer[i].b;
  }
}

/** Configuration management */
bool HUB75Driver::updateConfig(const HUB75Config& newConfig){
  if(!validateConfig(newConfig)){
    ESP_LOGE(TAG, "Invalid configuration");
    return false;
  }
  
  if(initialized){
    ESP_LOGW(TAG, "Updating config on initialised driver - restart required");
  }
  
  applyConfig(newConfig);
  return true;
}

/** Gamma correction controls */
void HUB75Driver::setGammaCorrection(bool enabled, float gamma){
  config.enable_gamma_correction = enabled;
  config.gamma_value = gamma;
  
  if(enabled){
    updateGammaTable(gamma);
  }
}

/** Internal helper methods */
void HUB75Driver::initializeLUT(){
  /** Initialize gamma correction table */
  if(config.enable_gamma_correction){
    updateGammaTable(config.gamma_value);
  }
}

void HUB75Driver::updateGammaTable(float gamma){
  /** Fast copy from compile-time optimised gamma tables - no pow() calculations */
  if(gamma >= 2.5f){
    memcpy(gamma_table, GAMMA_TABLE_26, sizeof(gamma_table));
    ESP_LOGI(TAG, "Updated to optimised gamma table (γ=2.6)");
  } else if(gamma >= 2.0f){
    memcpy(gamma_table, GAMMA_TABLE_22, sizeof(gamma_table));
    ESP_LOGI(TAG, "Updated to optimised gamma table (γ=2.2)");  
  } else {
    memcpy(gamma_table, GAMMA_TABLE_18, sizeof(gamma_table));
    ESP_LOGI(TAG, "Updated to optimised gamma table (γ=1.8)");
  }
  
  ESP_LOGI(TAG, "Fast gamma update - eliminated pow() runtime calculations");
}

bool HUB75Driver::validateConfig(const HUB75Config& cfg) const{
  /** Validate matrix dimensions */
  if(cfg.matrix_width <= 0 || cfg.matrix_height <= 0){
    ESP_LOGE(TAG, "Invalid matrix dimensions: %dx%d", cfg.matrix_width, cfg.matrix_height);
    return false;
  }
  
  /** Validate colour depth */
  if(cfg.colour_depth < 1 || cfg.colour_depth > 8){
    ESP_LOGE(TAG, "Invalid colour depth: %d (must be 1-8)", cfg.colour_depth);
    return false;
  }
  
  /** Validate clock frequency */
  if(cfg.clock_freq_hz < 1000000 || cfg.clock_freq_hz > 20000000){
    ESP_LOGE(TAG, "Invalid clock frequency: %d Hz", cfg.clock_freq_hz);
    return false;
  }
  
  /** Validate gamma value */
  if(cfg.enable_gamma_correction && (cfg.gamma_value < 0.1f || cfg.gamma_value > 5.0f)){
    ESP_LOGE(TAG, "Invalid gamma value: %.2f (must be 0.1-5.0)", cfg.gamma_value);
    return false;
  }
  
  return true;
}

void HUB75Driver::applyConfig(const HUB75Config& cfg){
  config = cfg;
  
  /** Initialize lookup tables if needed */
  if(cfg.enable_gamma_correction){
    updateGammaTable(cfg.gamma_value);
  }
}

bool HUB75Driver::isValidBufferSize(int width, int height) const{
  return (width == config.matrix_width && height == config.matrix_height);
}

void HUB75Driver::synchronizeOEPins(){
  /** Secondary OE pin is controlled via DMA buffer bit manipulation - no GPIO calls needed */
  if(oe_pin2 != GPIO_NUM_NC){
    ESP_LOGI(TAG, "Secondary OE pin %d synchronized via DMA buffer (bit %d)", 
             (int)oe_pin2, OE2_BIT);
  }
}