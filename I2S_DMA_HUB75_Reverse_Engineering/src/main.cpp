#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "lcd_parallel.hpp"
#include "parallel_buffer.hpp"

static const char* TAG = "HUB75_PLASMA";

/** Display configuration */
static const int MATRIX_WIDTH = 64;
static const int MATRIX_HEIGHT = 32;
static const int HUB75_ROWS = 16;  

/** RGB pixel structure */
struct RGBPixel {
  uint8_t r, g, b;
};

/** Hardware interface and buffer management */
static LcdParallel lcdInterface;
static ParallelBuffer dmaBuffer0;
static ParallelBuffer dmaBuffer1;
static uint16_t* frontBuffer = nullptr;
static uint16_t* backBuffer = nullptr;

/** Color depth configuration */
static const int COLOR_PLANES = 5;
static const int base_buffer_size = MATRIX_WIDTH * HUB75_ROWS;
static const int buffer_size = base_buffer_size * COLOR_PLANES;

/** Frame buffer representing actual LED matrix */
static RGBPixel prebuffer[MATRIX_HEIGHT][MATRIX_WIDTH];

/** Animation parameters */
struct SineWave {
  float frequency;
  float phase;
  float amplitude;
  float x_speed;
  float y_speed;
};

/** Plasma effect state */
static uint16_t time_counter = 0;
static uint16_t cycles = 0;

/** RGB color structure */
struct CRGB {
  uint8_t r, g, b;
  CRGB() : r(0), g(0), b(0) {}
  CRGB(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
};

/** Color palettes (16 colors each) */
static const CRGB HeatColors[16] = {
  {0,0,0}, {32,0,0}, {64,0,0}, {96,0,0}, {128,0,0}, {160,8,0}, {192,16,0}, {224,32,0},
  {255,64,0}, {255,96,0}, {255,128,0}, {255,160,0}, {255,192,0}, {255,224,0}, {255,255,0}, {255,255,255}
};

static const CRGB RainbowColors[16] = {
  {255,0,0}, {255,32,0}, {255,64,0}, {255,128,0}, {255,255,0}, {128,255,0}, {0,255,0}, {0,255,128},
  {0,255,255}, {0,128,255}, {0,0,255}, {128,0,255}, {255,0,255}, {255,0,128}, {255,0,64}, {255,0,32}
};

static const CRGB LavaColors[16] = {
  {0,0,0}, {64,0,0}, {128,0,0}, {192,0,0}, {255,0,0}, {255,32,0}, {255,64,0}, {255,96,0},
  {255,128,0}, {255,160,0}, {255,192,0}, {255,224,0}, {255,255,0}, {255,255,64}, {255,255,128}, {255,255,192}
};

static const CRGB CloudColors[16] = {
  {0,0,64}, {0,0,128}, {0,0,192}, {0,0,255}, {0,32,255}, {0,64,255}, {0,96,255}, {0,128,255},
  {32,160,255}, {64,192,255}, {96,224,255}, {128,255,255}, {160,255,255}, {192,255,255}, {224,255,255}, {255,255,255}
};

static const CRGB BlueishColors[16] = {
  {0xFF,0xFF,0xFF}, {0x26,0xCE,0xAA}, {0x98,0xE8,0xC1}, {0x07,0x8D,0x70}, {0x7B,0xAD,0xE2}, {0x50,0x49,0xCC}, {0x3D,0x1A,0x78}, {0xFF,0xFF,0xFF},
  {0x26,0xCE,0xAA}, {0x98,0xE8,0xC1}, {0x07,0x8D,0x70}, {0x7B,0xAD,0xE2}, {0x50,0x49,0xCC}, {0x3D,0x1A,0x78}, {0xFF,0xFF,0xFF}, {0x26,0xCE,0xAA}
};

/** Palette management */
static const CRGB* palettes[] = {HeatColors, RainbowColors, LavaColors, CloudColors, BlueishColors};
static const int NUM_PALETTES = 5;
static int currentPaletteIndex = 0;

/** Fast trigonometric lookup tables */
static const uint8_t sin8_table[256] = {
  128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220,218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,128,124,121,118,115,112,109,106,103,100,97,93,90,88,85,82,79,76,73,70,67,65,62,59,57,54,52,49,47,44,42,40,37,35,33,31,29,27,25,23,21,20,18,17,15,14,12,11,10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0,0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,10,11,12,14,15,17,18,20,21,23,25,27,29,31,33,35,37,40,42,44,47,49,52,54,57,59,62,65,67,70,73,76,79,82,85,88,90,93,97,100,103,106,109,112,115,118,121,124
};

static const int16_t sin16_table[256] = {
  0,804,1608,2410,3212,4011,4808,5602,6393,7179,7962,8739,9512,10278,11039,11793,12539,13279,14010,14732,15446,16151,16846,17530,18204,18868,19519,20159,20787,21403,22005,22594,23170,23731,24279,24811,25329,25832,26319,26790,27245,27683,28105,28510,28898,29268,29621,29956,30273,30571,30852,31113,31356,31580,31785,31971,32137,32285,32412,32521,32609,32678,32728,32757,32767,32757,32728,32678,32609,32521,32412,32285,32137,31971,31785,31580,31356,31113,30852,30571,30273,29956,29621,29268,28898,28510,28105,27683,27245,26790,26319,25832,25329,24811,24279,23731,23170,22594,22005,21403,20787,20159,19519,18868,18204,17530,16846,16151,15446,14732,14010,13279,12539,11793,11039,10278,9512,8739,7962,7179,6393,5602,4808,4011,3212,2410,1608,804,0,-804,-1608,-2410,-3212,-4011,-4808,-5602,-6393,-7179,-7962,-8739,-9512,-10278,-11039,-11793,-12539,-13279,-14010,-14732,-15446,-16151,-16846,-17530,-18204,-18868,-19519,-20159,-20787,-21403,-22005,-22594,-23170,-23731,-24279,-24811,-25329,-25832,-26319,-26790,-27245,-27683,-28105,-28510,-28898,-29268,-29621,-29956,-30273,-30571,-30852,-31113,-31356,-31580,-31785,-31971,-32137,-32285,-32412,-32521,-32609,-32678,-32728,-32757,-32767,-32757,-32728,-32678,-32609,-32521,-32412,-32285,-32137,-31971,-31785,-31580,-31356,-31113,-30852,-30571,-30273,-29956,-29621,-29268,-28898,-28510,-28105,-27683,-27245,-26790,-26319,-25832,-25329,-24811,-24279,-23731,-23170,-22594,-22005,-21403,-20787,-20159,-19519,-18868,-18204,-17530,-16846,-16151,-15446,-14732,-14010,-13279,-12539,-11793,-11039,-10278,-9512,-8739,-7962,-7179,-6393,-5602,-4808,-4011,-3212,-2410,-1608,-804
};

/** Fast trigonometric functions using lookup tables */
static inline uint8_t sin8(uint8_t theta){
  return sin8_table[theta];
}

static inline uint8_t cos8(uint8_t theta){
  return sin8_table[(theta + 64) & 0xFF];
}

static inline int16_t sin16(uint16_t theta){
  return sin16_table[theta >> 8];
}

static inline int16_t cos16(uint16_t theta){
  return sin16_table[((theta >> 8) + 64) & 0xFF];
}

/** Gamma correction lookup table (gamma = 2.2) */
static const uint8_t gamma_correction_table[256] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,
  1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,4,4,4,4,4,5,5,5,
  5,6,6,6,6,7,7,7,7,8,8,8,9,9,9,10,10,10,11,11,11,12,12,13,13,13,14,14,15,15,16,16,
  17,17,18,18,19,19,20,20,21,21,22,22,23,24,24,25,25,26,27,27,28,29,29,30,31,32,32,33,34,35,35,36,
  37,38,39,39,40,41,42,43,44,45,46,47,48,49,50,50,51,52,54,55,56,57,58,59,60,61,62,63,64,66,67,68,
  69,70,72,73,74,75,77,78,79,81,82,83,85,86,87,89,90,92,93,95,96,98,99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255
};

static inline uint8_t applyGammaCorrection(uint8_t value){
  return gamma_correction_table[value];
}

static CRGB applyGammaToColor(const CRGB& color){
  return CRGB(
    applyGammaCorrection(color.r),
    applyGammaCorrection(color.g),
    applyGammaCorrection(color.b)
  );
}

static CRGB colorFromPalette(const CRGB* palette, uint8_t index){
  uint8_t paletteIndex = index >> 4;
  uint8_t blend = index & 0x0F;
  
  if(paletteIndex >= 15){
    return palette[15];
  }
  
  /** Linear interpolation between palette colors */
  CRGB color1 = palette[paletteIndex];
  CRGB color2 = palette[paletteIndex + 1];
  
  CRGB interpolated = CRGB(
    color1.r + ((color2.r - color1.r) * blend / 16),
    color1.g + ((color2.g - color1.g) * blend / 16),
    color1.b + ((color2.b - color1.b) * blend / 16)
  );
  
  return applyGammaToColor(interpolated);
}

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

bool initI2S(){
  ESP_LOGI(TAG, "Initializing LCD interface with double buffering");
  
  /** Configure LCD parallel interface */
  LcdParallelConfig lcd_config = LcdParallel::getDefaultConfig();
  lcd_config.clock_freq_hz = 10000000;
  lcd_config.data_width = 13;
  lcd_config.continuous_mode = true;
  lcd_config.clock_pin = static_cast<gpio_num_t>(37);

  /** GPIO pin mapping for HUB75 protocol */
  gpio_num_t lcd_data_pins[13] = {
    static_cast<gpio_num_t>(7),   // R0
    static_cast<gpio_num_t>(15),  // G0
    static_cast<gpio_num_t>(16),  // B0
    static_cast<gpio_num_t>(17),  // R1
    static_cast<gpio_num_t>(18),  // G1
    static_cast<gpio_num_t>(8),   // B1
    static_cast<gpio_num_t>(36),  // LAT
    static_cast<gpio_num_t>(35),  // OE
    static_cast<gpio_num_t>(41),  // A
    static_cast<gpio_num_t>(40),  // B
    static_cast<gpio_num_t>(39),  // C
    static_cast<gpio_num_t>(38),  // D
    static_cast<gpio_num_t>(42)   // E
  };

  /** Allocate DMA buffers */
  if(!dmaBuffer0.alloc(buffer_size)){
    ESP_LOGE(TAG, "Failed to allocate front DMA buffer");
    return false;
  }
  
  if(!dmaBuffer1.alloc(buffer_size)){
    ESP_LOGE(TAG, "Failed to allocate back DMA buffer");
    return false;
  }

  /** Initialize LCD interface */
  if(!lcdInterface.init(lcd_data_pins, lcd_config)){
    ESP_LOGE(TAG, "Failed to initialize LCD interface");
    return false;
  }

  /** Set up buffer pointers */
  frontBuffer = dmaBuffer0.getBuffer();
  backBuffer = dmaBuffer1.getBuffer();
  
  ESP_LOGI(TAG, "Double buffering initialized (front: %p, back: %p)", frontBuffer, backBuffer);
  return true;
}

bool startI2S(){
  if(!lcdInterface.setDirectBuffer(frontBuffer, buffer_size)){
    ESP_LOGE(TAG, "Failed to set front buffer");
    return false;
  }
  if(!lcdInterface.start()){
    ESP_LOGE(TAG, "Failed to start transmission");
    return false;
  }
  ESP_LOGI(TAG, "Transmission started with double buffering");
  return true;
}

bool swapBuffers(){
  /** Swap the DMA to use back buffer as new front buffer */
  if(!lcdInterface.swapBuffer(backBuffer, buffer_size)){
    ESP_LOGE(TAG, "Failed to swap buffers");
    return false;
  }
  
  /** Swap local pointers */
  uint16_t* temp = frontBuffer;
  frontBuffer = backBuffer;
  backBuffer = temp;
  
  ESP_LOGD(TAG, "Buffers swapped (front: %p, back: %p)", frontBuffer, backBuffer);
  return true;
}

uint8_t generateBCMPattern(uint8_t brightness, int bit_position){
  /** BCM: each bit represents different time duration */
  return (brightness >> bit_position) & 1;
}

int16_t calculatePlasmaValue(float x, float y, uint8_t wibble, uint8_t cos_time){
  int16_t v = 128;
  
  /** Multiple sine wave interference creates plasma effect */
  v += sin16((uint16_t)(x * wibble * 3 + time_counter));
  v += cos16((uint16_t)(y * (128 - wibble) + time_counter));
  v += sin16((uint16_t)(y * x * cos_time / 8));
  
  return v;
}

/**
 * Generate plasma pattern with 2x2 supersampling anti-aliasing
 * Adapted from Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 */
void generatePlasmaPattern(){
  const CRGB* currentPalette = palettes[currentPaletteIndex];
  
  /** Pre-calculate animation values */
  uint8_t wibble = sin8(time_counter);
  uint8_t cos_time = cos8(-time_counter);
  
  /** Generate pattern with anti-aliasing */
  for(int y = 0; y < MATRIX_HEIGHT; y++){
    for(int x = 0; x < MATRIX_WIDTH; x++){
      /** 2x2 supersampling locations */
      float sub_samples[4][2] = {
        {x - 0.25f, y - 0.25f},
        {x + 0.25f, y - 0.25f},
        {x - 0.25f, y + 0.25f},
        {x + 0.25f, y + 0.25f}
      };
      
      /** Accumulate RGB values from sub-samples */
      uint32_t total_r = 0, total_g = 0, total_b = 0;
      
      for(int sample = 0; sample < 4; sample++){
        int16_t v = calculatePlasmaValue(sub_samples[sample][0], sub_samples[sample][1], wibble, cos_time);
        CRGB sample_color = colorFromPalette(currentPalette, (uint8_t)(v >> 8));
        
        total_r += sample_color.r;
        total_g += sample_color.g;
        total_b += sample_color.b;
      }
      
      /** Average and apply gamma correction */
      CRGB averaged_color = CRGB(
        (uint8_t)(total_r / 4),
        (uint8_t)(total_g / 4),
        (uint8_t)(total_b / 4)
      );
      
      CRGB gamma_corrected = applyGammaToColor(averaged_color);
      
      prebuffer[y][x].r = gamma_corrected.r;
      prebuffer[y][x].g = gamma_corrected.g;
      prebuffer[y][x].b = gamma_corrected.b;
    }
  }
  
  time_counter += 1;
  ++cycles;
}

static inline uint8_t convert8to5(uint8_t value){
  return value >> 3;
}

static inline uint8_t getBitFromValue(uint8_t value5bit, int bit_plane){
  return (value5bit >> bit_plane) & 1;
}

/**
 * Convert prebuffer to HUB75 DMA format with 5-bit color planes
 * Uses Binary Code Modulation (BCM) for brightness control
 */
void convertPrebufferToHUB75(){
  int buffer_index = 0;
  
  /** Generate each color plane for BCM */
  for(int plane = 0; plane < COLOR_PLANES; plane++){
    /** HUB75 row sequence with address/data desync fix */
    for(int sequence_index = 0; sequence_index < HUB75_ROWS; sequence_index++){
      int address_row = sequence_index;
      int data_row = (sequence_index + 1) % HUB75_ROWS;
      
      int upper_row = data_row;
      int lower_row = data_row + 16;
      
      /** Generate column data for this row and color plane */
      for(int col = 0; col < MATRIX_WIDTH; col++){
        uint16_t sample = 0;
        
        /** Set address lines for current row */
        if(address_row & (1 << 0)) sample |= (1 << A_BIT);
        if(address_row & (1 << 1)) sample |= (1 << B_BIT);
        if(address_row & (1 << 2)) sample |= (1 << C_BIT);
        if(address_row & (1 << 3)) sample |= (1 << D_BIT);
        
        /** Upper half pixel data (R0, G0, B0) */
        RGBPixel upper_pixel = prebuffer[upper_row][col];
        uint8_t r0_5bit = convert8to5(upper_pixel.r);
        uint8_t g0_5bit = convert8to5(upper_pixel.g);
        uint8_t b0_5bit = convert8to5(upper_pixel.b);
        
        uint8_t r0 = getBitFromValue(r0_5bit, plane);
        uint8_t g0 = getBitFromValue(g0_5bit, plane);
        uint8_t b0 = getBitFromValue(b0_5bit, plane);
        
        /** Lower half pixel data (R1, G1, B1) */
        RGBPixel lower_pixel = prebuffer[lower_row][col];
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
        
        /** Latch and output enable control */
        if(col == MATRIX_WIDTH - 1){
          sample |= (1 << LAT_BIT);
          sample |= (1 << OE_BIT);
        }
        
        backBuffer[buffer_index++] = sample;
      }
    }
  }
}

void setupHUB75Buffer(){
  /** Generate plasma pattern and convert to HUB75 format */
  generatePlasmaPattern();
  convertPrebufferToHUB75();
}

extern "C" void app_main(){
  ESP_LOGI(TAG, "HUB75 Plasma Display Starting");
  
  /** Disable watchdog timer */
  esp_task_wdt_deinit();
  
  /** Initialize hardware */
  if(!initI2S()){
    ESP_LOGE(TAG, "Initialization failed");
    return;
  }

  ESP_LOGI(TAG, "Configuration:");
  ESP_LOGI(TAG, "  Matrix: 64x32 pixels");
  ESP_LOGI(TAG, "  Color depth: 5-bit (555) with %d planes", COLOR_PLANES);
  ESP_LOGI(TAG, "  Buffer size: %d samples", buffer_size);  
  ESP_LOGI(TAG, "  Anti-aliasing: 2x2 supersampling");
  ESP_LOGI(TAG, "  Target FPS: 60");
  
  /** Initialize random seed and setup initial buffer */
  srand(esp_timer_get_time());
  setupHUB75Buffer();
  swapBuffers();
  
  /** Start transmission */
  if(!startI2S()){
    ESP_LOGE(TAG, "Failed to start transmission");
    return;
  }

  ESP_LOGI(TAG, "GPIO Mapping:");
  ESP_LOGI(TAG, "  Clock: GPIO 37 (10MHz)");
  ESP_LOGI(TAG, "  RGB0: GPIO 7,15,16   RGB1: GPIO 17,18,8");
  ESP_LOGI(TAG, "  Control: LAT=36, OE=35, ABCD=41,40,39,38");

  /** Main animation loop */
  uint32_t last_update_time = 0;
  uint32_t last_fps_report_time = 0;
  uint32_t last_palette_change_time = 0;
  uint32_t frame_count = 0;
  uint32_t fps_counter = 0;
  const uint32_t frame_interval_us = 16667;
  
  const char* palette_names[] = {"Heat", "Rainbow", "Lava", "Cloud", "Blueish"};
  
  ESP_LOGI(TAG, "Starting plasma animation");
  
  while(true){
    uint64_t current_time_us = esp_timer_get_time();
    
    /** Update frame */
    if(current_time_us - last_update_time >= frame_interval_us){
      setupHUB75Buffer();
      
      if(swapBuffers()){
        frame_count++;
        fps_counter++;
      }
      
      last_update_time = current_time_us;
    }
    
    /** Report FPS every 3 seconds */
    if(current_time_us - last_fps_report_time >= 3000000){
      float actual_fps = fps_counter / 3.0f;
      ESP_LOGI(TAG, "Running at %.1f FPS (palette: %s)", actual_fps, palette_names[currentPaletteIndex]);
      fps_counter = 0;
      last_fps_report_time = current_time_us;
    }
    
    /** Change palette every 5 seconds */
    if(current_time_us - last_palette_change_time >= 5000000){
      currentPaletteIndex = (currentPaletteIndex + 1) % NUM_PALETTES;
      ESP_LOGI(TAG, "Switched to %s palette", palette_names[currentPaletteIndex]);
      last_palette_change_time = current_time_us;
    }
    
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}