/*****************************************************************
 * File:      driver_hub75.cpp
 * Category:  abstraction/drivers/components/HUB75
 * 
 * Purpose:    HUB75 LED matrix display driver implementation
 *****************************************************************/

#include "driver_hub75.hpp"
#include "../../../core/hal_protocal_parallel.hpp"
#include "../../../core/hal_protocal_dma.hpp"
#include "../../../core/hal_protocal_parallel_buffer.hpp"
#include "../../../core/platform_hal.hpp"
#include <cstring>
#include <cmath>

namespace arcos::abstraction::drivers{

// Use types from parallel and dma namespaces
using parallel::IParallelHardware;
using parallel::ParallelHardwareConfig;
using dma::IDmaBufferManager;
using dma::DmaBufferConfig;
using dma::BufferMode;

constexpr const char* TAG = "HUB75_DRIVER";

/** HUB75 protocol bit positions */
constexpr int R0_BIT  = 0;
constexpr int G0_BIT  = 1;
constexpr int B0_BIT  = 2;
constexpr int R1_BIT  = 3;
constexpr int G1_BIT  = 4;
constexpr int B1_BIT  = 5;
constexpr int LAT_BIT = 6;
constexpr int OE_BIT  = 7;
constexpr int A_BIT   = 8;
constexpr int B_BIT   = 9;
constexpr int C_BIT   = 10;
constexpr int D_BIT   = 11;
constexpr int E_BIT   = 12;
constexpr int OE2_BIT = 13;  // Second OE pin

HUB75Driver::HUB75Driver()
  : hwInterface(nullptr)
  , bufferManager(nullptr)
  , frontBuffer(nullptr)
  , backBuffer(nullptr)
  , oe_pin2(PIN_NC)
  , platform(getPlatformHAL())
  , initialized(false)
  , running(false)
  , framebuffer(nullptr)
  , buffer_size(0)
  , base_buffer_size(0)
  , bcm_brightness(255)
  , last_bcm_brightness(255)
{
  // Initialise gamma table with compile-time values for optimal cache performance
  memcpy(gamma_table, GAMMA_TABLE_22, sizeof(gamma_table));
}

HUB75Driver::~HUB75Driver(){
  stop();
  if(framebuffer && platform){
    platform->freeMemory(framebuffer);
  }
  
  // NOTE: Driver does NOT own hardware/buffer managers
  // Application is responsible for lifecycle management of injected dependencies
}

bool HUB75Driver::init(const HUB75Config& cfg, IParallelHardware* hardware, IDmaBufferManager* buffer_mgr){
  if(initialized){
    PLATFORM_LOG_W(TAG, "Driver already initialised");
    return true;
  }
  
  // Driver REQUIRES hardware and buffer manager to be injected by application
  if(!hardware || !buffer_mgr){
    PLATFORM_LOG_E(TAG, "Hardware interface and buffer manager must be provided (cannot be null)");
    return false;
  }
  
  config = cfg;
  hwInterface = hardware;
  bufferManager = buffer_mgr;
  
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
  
  // Buffer size calculation for BCM timing with variable brightness:
  // PARALLEL_OE mode: BCM timing happens per-panel (after each 64-pixel latch)
  // SERIES_CHAIN/SINGLE: BCM timing happens once at end
  // BCM timing varies: 1,2,4,8,16 samples for planes 0-4 = 31 base cycles
  // For brightness control: allocate 64x base cycles, fill only needed amount
  // Base BCM cycles: 31 per bit plane set
  // Max BCM cycles (64x brightness): 31 * 64 = 1984 per bit plane set
  // This gives 64 brightness levels (0-63) utilizing the 64 pixel clock cycles
  // Delay samples: 3 per plane × 5 planes = 15
  int base_bcm_cycles = 31;  // Base BCM timing (1+2+4+8+16)
  int max_brightness_scale = 64;  // Maximum brightness scale factor (64 levels, matching pixels per row)
  int total_bcm_samples;
  if(config.expansion_mode == HUB75Config::ExpansionMode::PARALLEL_OE){
    // BCM per panel: allocate for max brightness
    total_bcm_samples = base_bcm_cycles * max_brightness_scale * config.panel_count;
  } else {
    // BCM once at end: allocate for max brightness
    total_bcm_samples = base_bcm_cycles * max_brightness_scale;
  }
  int total_delay_samples = config.colour_depth * 3;  // 3 delay bits per plane
  buffer_size = hub75_rows * (config.colour_depth * pixels_per_row + total_bcm_samples + total_delay_samples);
  
  /** Allocate framebuffer */
  int fb_width = config.dual_display_mode ? config.effective_width : config.matrix_width;
  size_t framebuffer_bytes = fb_width * config.matrix_height * sizeof(RGBPixel);
  framebuffer = static_cast<RGBPixel*>(platform->allocateMemory(framebuffer_bytes, MEM_CAP_DEFAULT));
  if(!framebuffer){
    PLATFORM_LOG_E(TAG, "Failed to allocate framebuffer (%d bytes)", framebuffer_bytes);
    return false;
  }
  
  /** Clear framebuffer */
  std::memset(framebuffer, 0, framebuffer_bytes);
  
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
    PLATFORM_LOG_E(TAG, "Failed to initialize buffer manager");
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
    PLATFORM_LOG_E(TAG, "Failed to initialise hardware interface");
    delete[] lcd_data_pins;
    return false;
  }
  
  delete[] lcd_data_pins;

  /** Set up buffer pointers from buffer manager */
  frontBuffer = bufferManager->getFrontBuffer();
  backBuffer = bufferManager->getBackBuffer();
  
  if(!frontBuffer || !backBuffer){
    PLATFORM_LOG_E(TAG, "Failed to get buffer pointers from buffer manager");
    return false;
  }
  
  /** Initialize lookup tables and configurations */
  initializeLUT();
  
  /** Store second OE pin for reference */
  if(config.pins.oe_pin2 != PIN_NC){
    oe_pin2 = config.pins.oe_pin2;
    PLATFORM_LOG_I(TAG, "Dual OE mode: Primary=%d, Secondary=%d (controlled via DMA buffer)", 
             (int)config.pins.oe_pin, (int)config.pins.oe_pin2);
  }
  
  initialized = true;
  
  PLATFORM_LOG_I(TAG, "HUB75 driver initialised:");
  PLATFORM_LOG_I(TAG, "  Hardware backend: %s", hwInterface->getBackendName());
  PLATFORM_LOG_I(TAG, "  Matrix: %dx%d pixels", config.matrix_width, config.matrix_height);
  PLATFORM_LOG_I(TAG, "  Colour depth: %d-bit (%d planes)", config.colour_depth, config.colour_depth);
  PLATFORM_LOG_I(TAG, "  Clock: %dMHz", config.clock_freq_hz / 1000000);
  PLATFORM_LOG_I(TAG, "  Buffer size: %d samples", buffer_size);
  PLATFORM_LOG_I(TAG, "  Buffer mode: %s", 
           bufferManager->getMode() == BufferMode::DOUBLE_BUFFER ? "Double buffered" : "Single buffered");
  
  return true;
}

bool HUB75Driver::start(){
  if(!initialized){
    PLATFORM_LOG_E(TAG, "Driver not initialised");
    return false;
  }
  
  if(running){
    PLATFORM_LOG_W(TAG, "Driver already running");
    return true;
  }
  
  /** Convert initial framebuffer and set up front buffer */
  convertFramebufferToHUB75();
  swapBuffers();
  
  /** Start transmission using hardware interface */
  if(!hwInterface->setDirectBuffer(frontBuffer, buffer_size)){
    PLATFORM_LOG_E(TAG, "Failed to set front buffer");
    return false;
  }
  
  if(!hwInterface->start()){
    PLATFORM_LOG_E(TAG, "Failed to start transmission");
    return false;
  }
  
  running = true;
  
  /** Synchronize secondary OE pin */
  synchronizeOEPins();
  
  PLATFORM_LOG_I(TAG, "HUB75 transmission started%s", 
           (oe_pin2 != PIN_NC) ? " (dual display mode)" : "");
  return true;
}

void HUB75Driver::stop(){
  if(running && hwInterface){
    hwInterface->stop();
    running = false;
    PLATFORM_LOG_I(TAG, "HUB75 transmission stopped%s", 
             (oe_pin2 != PIN_NC) ? " (dual display mode)" : "");
  }
}

void HUB75Driver::setBrightness(uint8_t brightness){
  bcm_brightness = brightness;
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
  
  /** Only regenerate buffer if brightness changed or on first call */
  /** Note: We always regenerate for pixel changes, but this helps reduce
   *  unnecessary regeneration when only animating brightness */
  convertFramebufferToHUB75();
  swapBuffers();
  
  /** Track brightness for next frame */
  last_bcm_brightness = bcm_brightness;
}

bool HUB75Driver::swapBuffers(){
  /** Swap buffers in the buffer manager */
  if(!bufferManager->swapBuffers()){
    PLATFORM_LOG_E(TAG, "Failed to swap buffers in buffer manager");
    return false;
  }
  
  /** Update local pointers */
  frontBuffer = bufferManager->getFrontBuffer();
  backBuffer = bufferManager->getBackBuffer();
  
  /** Update hardware interface to use new front buffer */
  if(!hwInterface->swapBuffer(frontBuffer, buffer_size)){
    PLATFORM_LOG_E(TAG, "Failed to swap buffer in hardware interface");
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
            // Base BCM length for this plane
            int base_bcm_length = 1 << plane;
            
            // Calculate how many cycles to fill based on brightness
            // bcm_brightness ranges 0-255, map to 0-63 scale (using 64 pixel cycles)
            // Total allocated cycles: base_bcm_length * 64
            // Active cycles: base_bcm_length * (bcm_brightness >> 2)
            int max_bcm_cycles = base_bcm_length * 64;
            int brightness_scale = bcm_brightness >> 2;  // Map 0-255 to 0-63
            int active_bcm_cycles = base_bcm_length * brightness_scale;
            int inactive_bcm_cycles = max_bcm_cycles - active_bcm_cycles;
            
            // Sample with OE enabled for THIS panel only
            uint16_t bcm_sample_on = 0;
            if(address_row & (1 << 0)) bcm_sample_on |= (1 << A_BIT);
            if(address_row & (1 << 1)) bcm_sample_on |= (1 << B_BIT);
            if(address_row & (1 << 2)) bcm_sample_on |= (1 << C_BIT);
            if(address_row & (1 << 3)) bcm_sample_on |= (1 << D_BIT);
            
            if(panel_index == 0){
              // Panel 0: OE LOW (enabled), OE2 HIGH (disabled)
              bcm_sample_on |= (1 << OE2_BIT);
            } else if(panel_index == 1){
              // Panel 1: OE HIGH (disabled), OE2 LOW (enabled)
              bcm_sample_on |= (1 << OE_BIT);
            }
            
            // Sample with OE disabled (both OE and OE2 HIGH)
            uint16_t bcm_sample_off = bcm_sample_on | (1 << OE_BIT) | (1 << OE2_BIT);
            // But keep address lines from bcm_sample_on
            bcm_sample_off = (bcm_sample_on & ~((1 << OE_BIT) | (1 << OE2_BIT))) | (1 << OE_BIT) | (1 << OE2_BIT);
            
            // Fill active cycles with OE enabled
            for(int bcm_cycle = 0; bcm_cycle < active_bcm_cycles; bcm_cycle++){
              backBuffer[buffer_index++] = bcm_sample_on;
            }
            
            // Fill remaining cycles with OE disabled
            for(int bcm_cycle = 0; bcm_cycle < inactive_bcm_cycles; bcm_cycle++){
              backBuffer[buffer_index++] = bcm_sample_off;
            }
          }
        }
      }
      
      /** BCM Timing for SERIES_CHAIN and SINGLE modes (once at end)
       *  For these modes, all pixels are latched together at the end
       *  Fill-up strategy: allocate max cycles, fill only needed amount with OE
       */
      if(config.expansion_mode != HUB75Config::ExpansionMode::PARALLEL_OE){
        int base_bcm_length = 1 << plane;
        
        // Calculate active vs inactive cycles based on brightness
        // Map 0-255 brightness to 0-63 scale (64 levels matching pixel count)
        int max_bcm_cycles = base_bcm_length * 64;
        int brightness_scale = bcm_brightness >> 2;  // Map 0-255 to 0-63
        int active_bcm_cycles = base_bcm_length * brightness_scale;
        int inactive_bcm_cycles = max_bcm_cycles - active_bcm_cycles;
        
        // Sample with OE enabled (LOW)
        uint16_t bcm_sample_on = 0;
        if(address_row & (1 << 0)) bcm_sample_on |= (1 << A_BIT);
        if(address_row & (1 << 1)) bcm_sample_on |= (1 << B_BIT);
        if(address_row & (1 << 2)) bcm_sample_on |= (1 << C_BIT);
        if(address_row & (1 << 3)) bcm_sample_on |= (1 << D_BIT);
        // OE enabled (LOW) - leave bit at 0
        
        // Sample with OE disabled (HIGH)
        uint16_t bcm_sample_off = bcm_sample_on | (1 << OE_BIT);
        
        // Fill active cycles with OE enabled
        for(int bcm_cycle = 0; bcm_cycle < active_bcm_cycles; bcm_cycle++){
          backBuffer[buffer_index++] = bcm_sample_on;
        }
        
        // Fill remaining cycles with OE disabled
        for(int bcm_cycle = 0; bcm_cycle < inactive_bcm_cycles; bcm_cycle++){
          backBuffer[buffer_index++] = bcm_sample_off;
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
    PLATFORM_LOG_E(TAG, "Invalid buffer dimensions: %dx%d", buffer.width, buffer.height);
    return false;
  }
  
  if(buffer.format != FrameBuffer::RGB888){
    PLATFORM_LOG_E(TAG, "Unsupported buffer format");
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
    PLATFORM_LOG_E(TAG, "Invalid buffer parameters");
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
    PLATFORM_LOG_E(TAG, "Invalid destination buffer");
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
    PLATFORM_LOG_E(TAG, "Invalid configuration");
    return false;
  }
  
  if(initialized){
    PLATFORM_LOG_W(TAG, "Updating config on initialised driver - restart required");
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
    PLATFORM_LOG_I(TAG, "Updated to optimised gamma table (γ=2.6)");
  } else if(gamma >= 2.0f){
    memcpy(gamma_table, GAMMA_TABLE_22, sizeof(gamma_table));
    PLATFORM_LOG_I(TAG, "Updated to optimised gamma table (γ=2.2)");  
  } else {
    memcpy(gamma_table, GAMMA_TABLE_18, sizeof(gamma_table));
    PLATFORM_LOG_I(TAG, "Updated to optimised gamma table (γ=1.8)");
  }
  
  PLATFORM_LOG_I(TAG, "Fast gamma update - eliminated pow() runtime calculations");
}

bool HUB75Driver::validateConfig(const HUB75Config& cfg) const{
  /** Validate matrix dimensions */
  if(cfg.matrix_width <= 0 || cfg.matrix_height <= 0){
    PLATFORM_LOG_E(TAG, "Invalid matrix dimensions: %dx%d", cfg.matrix_width, cfg.matrix_height);
    return false;
  }
  
  /** Validate colour depth */
  if(cfg.colour_depth < 1 || cfg.colour_depth > 8){
    PLATFORM_LOG_E(TAG, "Invalid colour depth: %d (must be 1-8)", cfg.colour_depth);
    return false;
  }
  
  /** Validate clock frequency */
  if(cfg.clock_freq_hz < 1000000 || cfg.clock_freq_hz > 20000000){
    PLATFORM_LOG_E(TAG, "Invalid clock frequency: %d Hz", cfg.clock_freq_hz);
    return false;
  }
  
  /** Validate gamma value */
  if(cfg.enable_gamma_correction && (cfg.gamma_value < 0.1f || cfg.gamma_value > 5.0f)){
    PLATFORM_LOG_E(TAG, "Invalid gamma value: %.2f (must be 0.1-5.0)", cfg.gamma_value);
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
  if(oe_pin2 != PIN_NC){
    PLATFORM_LOG_I(TAG, "Secondary OE pin %d synchronized via DMA buffer (bit %d)", 
             (int)oe_pin2, OE2_BIT);
  }
}

} // namespace arcos::abstraction::drivers