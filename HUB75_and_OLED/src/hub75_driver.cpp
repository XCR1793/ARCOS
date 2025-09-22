#include "hub75_driver.hpp"
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
  : frontBuffer(nullptr)
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
}

bool HUB75Driver::init(const HUB75Config& cfg){
  if(initialized){
    ESP_LOGW(TAG, "Driver already initialised");
    return true;
  }
  
  config = cfg;
  
  /** Calculate buffer sizes */
  base_buffer_size = config.matrix_width * (config.matrix_height / 2);
  
  /** For dual display mode, we need double the buffer size 
   *  because we generate two separate sections (Panel 1 + Panel 2) */
  if(config.dual_display_mode){
    buffer_size = base_buffer_size * config.colour_depth * 2;  // Double for dual panels
  } else {
    buffer_size = base_buffer_size * config.colour_depth;
  }
  
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
  
  /** Configure LCD parallel interface */
  LcdParallelConfig lcd_config = LcdParallel::getDefaultConfig();
  lcd_config.clock_freq_hz = config.clock_freq_hz;
  lcd_config.data_width = num_pins;
  lcd_config.continuous_mode = true;
  lcd_config.clock_pin = static_cast<gpio_num_t>(config.pins.clock_pin);
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

  /** Allocate DMA buffers */
  if(!dmaBuffer0.alloc(buffer_size)){
    ESP_LOGE(TAG, "Failed to allocate front DMA buffer");
    return false;
  }
  
  if(!dmaBuffer1.alloc(buffer_size)){
    ESP_LOGE(TAG, "Failed to allocate back DMA buffer");
    return false;
  }

  /** Initialise LCD interface */
  if(!lcdInterface.init(lcd_data_pins, lcd_config)){
    ESP_LOGE(TAG, "Failed to initialise LCD interface");
    delete[] lcd_data_pins;
    return false;
  }
  
  delete[] lcd_data_pins;

  /** Set up buffer pointers */
  frontBuffer = dmaBuffer0.getBuffer();
  backBuffer = dmaBuffer1.getBuffer();
  
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
  ESP_LOGI(TAG, "  Matrix: %dx%d pixels", config.matrix_width, config.matrix_height);
  ESP_LOGI(TAG, "  Colour depth: %d-bit (%d planes)", config.colour_depth, config.colour_depth);
  ESP_LOGI(TAG, "  Clock: %dMHz", config.clock_freq_hz / 1000000);
  ESP_LOGI(TAG, "  Buffer size: %d samples", buffer_size);
  
  return true;
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
  
  /** Start transmission */
  if(!lcdInterface.setDirectBuffer(frontBuffer, buffer_size)){
    ESP_LOGE(TAG, "Failed to set front buffer");
    return false;
  }
  
  if(!lcdInterface.start()){
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
  if(running){
    lcdInterface.stop();
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
  /** Swap the DMA to use back buffer as new front buffer */
  if(!lcdInterface.swapBuffer(backBuffer, buffer_size)){
    ESP_LOGE(TAG, "Failed to swap buffers");
    return false;
  }
  
  /** Swap local pointers */
  uint16_t* temp = frontBuffer;
  frontBuffer = backBuffer;
  backBuffer = temp;
  
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
  int fb_width = config.dual_display_mode ? config.effective_width : config.matrix_width;
  
  /** Generate each colour plane for BCM */
  for(int plane = 0; plane < config.colour_depth; plane++){
    /** HUB75 row sequence with address/data desync fix (from old working code) */
    for(int sequence_index = 0; sequence_index < hub75_rows; sequence_index++){
      int address_row = sequence_index;
      int data_row = (sequence_index + 1) % hub75_rows;  // +1 offset fix for row-pixel shift bug
      
      int upper_row = data_row;
      int lower_row = data_row + hub75_rows;
      
      if(config.dual_display_mode){
        /** DUAL DISPLAY MODE: Generate data for both panels sequentially
         *  First: Panel 0 data (columns 0-63) with OE1 active
         *  Then:  Panel 1 data (columns 64-127) with OE2 active
         */
        
        // === PANEL 0 DATA (columns 0-63) ===
        // Panel 0 needs -1 offset to align with Panel 1 (Panel 0 was 1 row ahead)
        int panel0_data_row = (sequence_index + 0 + hub75_rows) % hub75_rows;  // -1 offset with wrap-around
        int panel0_upper_row = panel0_data_row;
        int panel0_lower_row = panel0_data_row + hub75_rows;
        
        for(int col = 0; col < config.matrix_width; col++){
          uint16_t sample = 0;
          
          /** Set address lines for current row */
          if(address_row & (1 << 0)) sample |= (1 << A_BIT);
          if(address_row & (1 << 1)) sample |= (1 << B_BIT);
          if(address_row & (1 << 2)) sample |= (1 << C_BIT);
          if(address_row & (1 << 3)) sample |= (1 << D_BIT);
          
          // Panel 0: read from buffer columns 0-63 with Panel 0 specific row offset
          int upper_index = panel0_upper_row * fb_width + col;
          int lower_index = panel0_lower_row * fb_width + col;
          
          RGBPixel upper_pixel = framebuffer[upper_index];
          RGBPixel lower_pixel = framebuffer[lower_index];
          
          /** Convert to bit planes */
          uint8_t r0 = getBitFromValue(convert8to5(upper_pixel.r), plane);
          uint8_t g0 = getBitFromValue(convert8to5(upper_pixel.g), plane);
          uint8_t b0 = getBitFromValue(convert8to5(upper_pixel.b), plane);
          uint8_t r1 = getBitFromValue(convert8to5(lower_pixel.r), plane);
          uint8_t g1 = getBitFromValue(convert8to5(lower_pixel.g), plane);
          uint8_t b1 = getBitFromValue(convert8to5(lower_pixel.b), plane);
          
          /** Set RGB data bits */
          if(r0) sample |= (1 << R0_BIT);
          if(g0) sample |= (1 << G0_BIT);
          if(b0) sample |= (1 << B0_BIT);
          if(r1) sample |= (1 << R1_BIT);
          if(g1) sample |= (1 << G1_BIT);
          if(b1) sample |= (1 << B1_BIT);
          
          /** Panel 0 control signals */
          if(col == config.matrix_width - 1){
            sample |= (1 << LAT_BIT);
            sample |= (1 << OE_BIT);  // Enable OE1 for Panel 0
            // OE2 is NOT active for Panel 0 data
          }
          
          backBuffer[buffer_index++] = sample;
        }
        
        // === PANEL 1 DATA (columns 64-127) ===
        for(int col = 0; col < config.matrix_width; col++){
          uint16_t sample = 0;
          
          /** Set address lines for current row */
          if(address_row & (1 << 0)) sample |= (1 << A_BIT);
          if(address_row & (1 << 1)) sample |= (1 << B_BIT);
          if(address_row & (1 << 2)) sample |= (1 << C_BIT);
          if(address_row & (1 << 3)) sample |= (1 << D_BIT);
          
          // Panel 1: read from buffer columns 64-127
          int upper_index = upper_row * fb_width + col + config.matrix_width;
          int lower_index = lower_row * fb_width + col + config.matrix_width;
          
          RGBPixel upper_pixel = framebuffer[upper_index];
          RGBPixel lower_pixel = framebuffer[lower_index];
          
          /** Convert to bit planes */
          uint8_t r0 = getBitFromValue(convert8to5(upper_pixel.r), plane);
          uint8_t g0 = getBitFromValue(convert8to5(upper_pixel.g), plane);
          uint8_t b0 = getBitFromValue(convert8to5(upper_pixel.b), plane);
          uint8_t r1 = getBitFromValue(convert8to5(lower_pixel.r), plane);
          uint8_t g1 = getBitFromValue(convert8to5(lower_pixel.g), plane);
          uint8_t b1 = getBitFromValue(convert8to5(lower_pixel.b), plane);
          
          /** Set RGB data bits */
          if(r0) sample |= (1 << R0_BIT);
          if(g0) sample |= (1 << G0_BIT);
          if(b0) sample |= (1 << B0_BIT);
          if(r1) sample |= (1 << R1_BIT);
          if(g1) sample |= (1 << G1_BIT);
          if(b1) sample |= (1 << B1_BIT);
          
          /** Panel 1 control signals */
          if(col == config.matrix_width - 1){
            sample |= (1 << LAT_BIT);
            // OE1 is NOT active for Panel 1 data
            if(config.pins.oe_pin2 >= 0){
              sample |= (1 << OE2_BIT);  // Enable OE2 for Panel 1
            }
          }
          
          backBuffer[buffer_index++] = sample;
        }
        
      } else {
        /** SINGLE DISPLAY MODE */
        for(int col = 0; col < config.matrix_width; col++){
          uint16_t sample = 0;
          
          /** Set address lines for current row */
          if(address_row & (1 << 0)) sample |= (1 << A_BIT);
          if(address_row & (1 << 1)) sample |= (1 << B_BIT);
          if(address_row & (1 << 2)) sample |= (1 << C_BIT);
          if(address_row & (1 << 3)) sample |= (1 << D_BIT);
          
          /** Single display coordinates */
          int upper_index = upper_row * config.matrix_width + col;
          int lower_index = lower_row * config.matrix_width + col;
          
          RGBPixel upper_pixel = framebuffer[upper_index];
          RGBPixel lower_pixel = framebuffer[lower_index];
          
          /** Convert to bit planes */
          uint8_t r0 = getBitFromValue(convert8to5(upper_pixel.r), plane);
          uint8_t g0 = getBitFromValue(convert8to5(upper_pixel.g), plane);
          uint8_t b0 = getBitFromValue(convert8to5(upper_pixel.b), plane);
          uint8_t r1 = getBitFromValue(convert8to5(lower_pixel.r), plane);
          uint8_t g1 = getBitFromValue(convert8to5(lower_pixel.g), plane);
          uint8_t b1 = getBitFromValue(convert8to5(lower_pixel.b), plane);
          
          /** Set RGB data bits */
          if(r0) sample |= (1 << R0_BIT);
          if(g0) sample |= (1 << G0_BIT);
          if(b0) sample |= (1 << B0_BIT);
          if(r1) sample |= (1 << R1_BIT);
          if(g1) sample |= (1 << G1_BIT);
          if(b1) sample |= (1 << B1_BIT);
          
          /** Single display control signals */
          if(col == config.matrix_width - 1){
            sample |= (1 << LAT_BIT);
            sample |= (1 << OE_BIT);
          }
          
          backBuffer[buffer_index++] = sample;
        }
      }
    }
  }
  
  if(false) {  // Disable the old separate buffer sections approach
    /** SINGLE DISPLAY MODE: Original logic */
    for(int plane = 0; plane < config.colour_depth; plane++){
      for(int sequence_index = 0; sequence_index < hub75_rows; sequence_index++){
        int address_row = sequence_index;
        int data_row = (sequence_index + 1) % hub75_rows;
        
        int upper_row = data_row;
        int lower_row = data_row + hub75_rows;
        
        /** Generate column data for this row and colour plane */
        for(int col = 0; col < config.matrix_width; col++){
          uint16_t sample = 0;
          
          /** Set address lines for current row */
          if(address_row & (1 << 0)) sample |= (1 << A_BIT);
          if(address_row & (1 << 1)) sample |= (1 << B_BIT);
          if(address_row & (1 << 2)) sample |= (1 << C_BIT);
          if(address_row & (1 << 3)) sample |= (1 << D_BIT);
          
          /** Single display coordinates */
          int upper_index = upper_row * config.matrix_width + col;
          int lower_index = lower_row * config.matrix_width + col;
          
          RGBPixel upper_pixel = framebuffer[upper_index];
          RGBPixel lower_pixel = framebuffer[lower_index];
          
          /** Convert and set RGB bits */
          uint8_t r0 = getBitFromValue(convert8to5(upper_pixel.r), plane);
          uint8_t g0 = getBitFromValue(convert8to5(upper_pixel.g), plane);
          uint8_t b0 = getBitFromValue(convert8to5(upper_pixel.b), plane);
          uint8_t r1 = getBitFromValue(convert8to5(lower_pixel.r), plane);
          uint8_t g1 = getBitFromValue(convert8to5(lower_pixel.g), plane);
          uint8_t b1 = getBitFromValue(convert8to5(lower_pixel.b), plane);
          
          if(r0) sample |= (1 << R0_BIT);
          if(g0) sample |= (1 << G0_BIT);
          if(b0) sample |= (1 << B0_BIT);
          if(r1) sample |= (1 << R1_BIT);
          if(g1) sample |= (1 << G1_BIT);
          if(b1) sample |= (1 << B1_BIT);
          
          /** Control signals */
          if(col == config.matrix_width - 1){
            sample |= (1 << LAT_BIT);
            sample |= (1 << OE_BIT);
          }
          
          backBuffer[buffer_index++] = sample;
        }
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
  int expected_width = config.dual_display_mode ? config.effective_width : config.matrix_width;
  return (width == expected_width && height == config.matrix_height);
}

void HUB75Driver::synchronizeOEPins(){
  /** Secondary OE pin is controlled via DMA buffer bit manipulation - no GPIO calls needed */
  if(oe_pin2 != GPIO_NUM_NC){
    ESP_LOGI(TAG, "Secondary OE pin %d synchronized via DMA buffer (bit %d)", 
             (int)oe_pin2, OE2_BIT);
  }
}