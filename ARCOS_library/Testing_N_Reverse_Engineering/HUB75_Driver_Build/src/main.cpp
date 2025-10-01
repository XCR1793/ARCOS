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
#include "hub75_driver.hpp"

static const char* TAG = "PLASMA_DEMO";

/** HUB75 display driver with dual OE support */
static HUB75Driver display;

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

/** Performance optimisation: Pre-calculated lookup tables */
static int16_t plasma_x_cache[128];  // X-component cache for 128 pixels wide
static int16_t plasma_y_cache[32];   // Y-component cache for 32 pixels high
static bool cache_initialized = false;

/** RGB colour structure */
struct CRGB {
  uint8_t r, g, b;
  CRGB() : r(0), g(0), b(0) {}
  CRGB(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
};

/** Colour palettes (16 colours each) */
static const CRGB HeatColours[16] = {
  {0,0,0}, {32,0,0}, {64,0,0}, {96,0,0}, {128,0,0}, {160,8,0}, {192,16,0}, {224,32,0},
  {255,64,0}, {255,96,0}, {255,128,0}, {255,160,0}, {255,192,0}, {255,224,0}, {255,255,0}, {255,255,255}
};

static const CRGB RainbowColours[16] = {
  {255,0,0}, {255,32,0}, {255,64,0}, {255,128,0}, {255,255,0}, {128,255,0}, {0,255,0}, {0,255,128},
  {0,255,255}, {0,128,255}, {0,0,255}, {128,0,255}, {255,0,255}, {255,0,128}, {255,0,64}, {255,0,32}
};

static const CRGB LavaColours[16] = {
  {0,0,0}, {64,0,0}, {128,0,0}, {192,0,0}, {255,0,0}, {255,32,0}, {255,64,0}, {255,96,0},
  {255,128,0}, {255,160,0}, {255,192,0}, {255,224,0}, {255,255,0}, {255,255,64}, {255,255,128}, {255,255,192}
};

static const CRGB CloudColours[16] = {
  {0,0,64}, {0,0,128}, {0,0,192}, {0,0,255}, {0,32,255}, {0,64,255}, {0,96,255}, {0,128,255},
  {32,160,255}, {64,192,255}, {96,224,255}, {128,255,255}, {160,255,255}, {192,255,255}, {224,255,255}, {255,255,255}
};

static const CRGB BlueishColours[16] = {
  {0xFF,0xFF,0xFF}, {0x26,0xCE,0xAA}, {0x98,0xE8,0xC1}, {0x07,0x8D,0x70}, {0x7B,0xAD,0xE2}, {0x50,0x49,0xCC}, {0x3D,0x1A,0x78}, {0xFF,0xFF,0xFF},
  {0x26,0xCE,0xAA}, {0x98,0xE8,0xC1}, {0x07,0x8D,0x70}, {0x7B,0xAD,0xE2}, {0x50,0x49,0xCC}, {0x3D,0x1A,0x78}, {0xFF,0xFF,0xFF}, {0x26,0xCE,0xAA}
};

/** Palette management */
static const CRGB* palettes[] = {HeatColours, RainbowColours, LavaColours, CloudColours, BlueishColours};
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

/** Note: Gamma correction is now handled by the HUB75Driver built-in system */

static CRGB colourFromPalette(const CRGB* palette, uint8_t index){
  uint8_t paletteIndex = index >> 4;
  uint8_t blend = index & 0x0F;
  
  if(paletteIndex >= 15){
    return palette[15];
  }
  
  /** Linear interpolation between palette colours */
  CRGB colour1 = palette[paletteIndex];
  CRGB colour2 = palette[paletteIndex + 1];
  
  CRGB interpolated = CRGB(
    colour1.r + ((colour2.r - colour1.r) * blend / 16),
    colour1.g + ((colour2.g - colour1.g) * blend / 16),
    colour1.b + ((colour2.b - colour1.b) * blend / 16)
  );
  
  return interpolated; // Gamma correction handled by HUB75Driver
}

/** Initialise performance optimisation caches */
void initialisePerformanceCaches(){
  if(cache_initialized) return;
  
  /** Pre-calculate base X and Y components (fixed per pixel position) */
  for(int x = 0; x < 128; x++){
    plasma_x_cache[x] = x * 256; // Scale for integer math
  }
  
  for(int y = 0; y < 32; y++){
    plasma_y_cache[y] = y * 512; // Scale for integer math  
  }
  
  cache_initialized = true;
  ESP_LOGI(TAG, "Performance caches initialised");
}

/** Optimised plasma calculation using integer math and caches */
int16_t calculatePlasmaValue(int x, int y, uint8_t wibble, uint8_t cos_time){
  int16_t v = 128;
  
  /** Use cached values and integer math for speed */
  uint16_t x_comp = (plasma_x_cache[x] * wibble) >> 6; // Divide by 64 instead of multiply by wibble/64
  uint16_t y_comp = (plasma_y_cache[y] * (128 - wibble)) >> 7; // Divide by 128
  
  /** Multiple sine wave interference creates plasma effect */
  v += sin16(x_comp + (time_counter << 2));
  v += cos16(y_comp + (time_counter << 1));
  v += sin16(((plasma_x_cache[x] + plasma_y_cache[y]) * cos_time) >> 11);
  
  return v;
}

/**
 * High-performance plasma pattern generation
 * Optimised for 60+ FPS with caching and integer math
 * Updated to work with dual display setup
 * Adapted from Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 */
void generatePlasmaPattern(HUB75Driver& display){
  const CRGB* currentPalette = palettes[currentPaletteIndex];
  
  /** Pre-calculate animation values */
  uint8_t wibble = sin8(time_counter);
  uint8_t cos_time = cos8(-time_counter);
  
  /** Generate pattern with optimised single-sample approach */
  for(int y = 0; y < display.getHeight(); y++){
    for(int x = 0; x < display.getWidth(); x++){
      /** Single sample calculation (4x faster than anti-aliasing) */
      int16_t v = calculatePlasmaValue(x, y, wibble, cos_time);
      CRGB pixel_colour = colourFromPalette(currentPalette, (uint8_t)(v >> 8));
      
      /** Set pixel in display (gamma correction handled by driver) */
      display.setPixel(x, y, RGB(pixel_colour.r, pixel_colour.g, pixel_colour.b));
    }
  }
  
  time_counter += 3; // Higher animation speed for smoother motion
  ++cycles;
}

/** Animation state for moving LUT triangles */
static float animation_time = 0.0f;

/** Brightness fade control */
static float brightness_phase = 0.0f;  // 0 to 2π for smooth sine wave
static uint8_t global_brightness = 255;

/** Helper function: Calculate barycentric coordinates and interpolate color
 *  Returns true if point is inside triangle, with interpolated RGB color
 */
bool sampleTriangleColor(float px, float py, float v0_x, float v0_y, float v1_x, float v1_y, 
                         float v2_x, float v2_y, bool is_cmy, uint8_t& r, uint8_t& g, uint8_t& b){
  // Compute barycentric weights
  float denom = (v1_y - v2_y) * (v0_x - v2_x) + (v2_x - v1_x) * (v0_y - v2_y);
  if(denom == 0) return false;
  
  float w0 = ((v1_y - v2_y) * (px - v2_x) + (v2_x - v1_x) * (py - v2_y)) / denom;
  float w1 = ((v2_y - v0_y) * (px - v2_x) + (v0_x - v2_x) * (py - v2_y)) / denom;
  float w2 = 1.0f - w0 - w1;
  
  // Check if point is inside triangle (with small tolerance for edge anti-aliasing)
  if(w0 >= -0.01f && w1 >= -0.01f && w2 >= -0.01f){
    // Clamp weights to [0, 1] for color calculation
    w0 = w0 < 0 ? 0 : (w0 > 1 ? 1 : w0);
    w1 = w1 < 0 ? 0 : (w1 > 1 ? 1 : w1);
    w2 = w2 < 0 ? 0 : (w2 > 1 ? 1 : w2);
    
    if(is_cmy){
      // CMY triangle: v0 = CYAN, v1 = MAGENTA, v2 = YELLOW
      r = (uint8_t)((w1 * 255 + w2 * 255) > 255 ? 255 : (w1 * 255 + w2 * 255));
      g = (uint8_t)((w0 * 255 + w2 * 255) > 255 ? 255 : (w0 * 255 + w2 * 255));
      b = (uint8_t)((w0 * 255 + w1 * 255) > 255 ? 255 : (w0 * 255 + w1 * 255));
    } else {
      // RGB triangle: v0 = RED, v1 = GREEN, v2 = BLUE
      r = (uint8_t)(w0 * 255);
      g = (uint8_t)(w1 * 255);
      b = (uint8_t)(w2 * 255);
    }
    return true;
  }
  return false;
}

void drawMovingLUTTriangles(){
  /** Clear display */
  for(int y = 0; y < display.getHeight(); y++){
    for(int x = 0; x < display.getWidth(); x++){
      display.setPixel(x, y, RGB(0, 0, 0));
    }
  }
  
  const int tri_size = 28;  // 28-pixel triangle (fits well in panel with motion)
  const int height = display.getHeight();
  
  /** Panel 0: RGB Triangle with 2x2 supersampling antialiasing
   *  Equilateral-ish triangle with vertices at:
   *  - Top vertex (center-top): RED (255, 0, 0)
   *  - Bottom-left vertex: GREEN (0, 255, 0)
   *  - Bottom-right vertex: BLUE (0, 0, 255)
   */
  
  // Calculate smooth position using sine waves (keep as float for smooth sub-pixel motion)
  float x_offset = 18.0f + 10.0f * sin8((uint8_t)(animation_time * 1.5f)) / 128.0f;
  float y_offset = 2.0f + 6.0f * cos8((uint8_t)(animation_time * 2.0f)) / 128.0f;
  
  // Define triangle vertices (relative to base position)
  float v0_x = tri_size / 2.0f;        // Top vertex (red)
  float v0_y = 0.0f;
  float v1_x = 0.0f;                    // Bottom-left vertex (green)
  float v1_y = tri_size;
  float v2_x = tri_size;                // Bottom-right vertex (blue)
  float v2_y = tri_size;
  
  // Calculate bounding box for efficient rendering
  int min_x = (int)(x_offset - 1);
  int max_x = (int)(x_offset + tri_size + 1);
  int min_y = (int)(y_offset - 1);
  int max_y = (int)(y_offset + tri_size + 1);
  
  // Clamp to panel 0 bounds
  if(min_x < 0) min_x = 0;
  if(max_x > 63) max_x = 63;
  if(min_y < 0) min_y = 0;
  if(max_y >= height) max_y = height - 1;
  
  // Draw RGB triangle with 2x2 supersampling antialiasing
  for(int screen_y = min_y; screen_y <= max_y; screen_y++){
    for(int screen_x = min_x; screen_x <= max_x; screen_x++){
      // 2x2 supersampling: sample at 4 sub-pixel locations
      int total_r = 0, total_g = 0, total_b = 0;
      int sample_count = 0;
      
      // Sample offsets: 0.25 and 0.75 within pixel for 2x2 grid
      float offsets[2] = {0.25f, 0.75f};
      
      for(int sy = 0; sy < 2; sy++){
        for(int sx = 0; sx < 2; sx++){
          // Convert screen coordinates to triangle-local coordinates
          float sample_x = (screen_x + offsets[sx]) - x_offset;
          float sample_y = (screen_y + offsets[sy]) - y_offset;
          
          uint8_t r, g, b;
          if(sampleTriangleColor(sample_x, sample_y, v0_x, v0_y, v1_x, v1_y, v2_x, v2_y, false, r, g, b)){
            total_r += r;
            total_g += g;
            total_b += b;
            sample_count++;
          }
        }
      }
      
      // Average the samples for antialiasing
      if(sample_count > 0){
        uint8_t avg_r = total_r / sample_count;
        uint8_t avg_g = total_g / sample_count;
        uint8_t avg_b = total_b / sample_count;
        
        // Blend with background based on coverage
        float coverage = sample_count / 4.0f;
        avg_r = (uint8_t)(avg_r * coverage);
        avg_g = (uint8_t)(avg_g * coverage);
        avg_b = (uint8_t)(avg_b * coverage);
        
        // Apply global brightness
        avg_r = (avg_r * global_brightness) / 255;
        avg_g = (avg_g * global_brightness) / 255;
        avg_b = (avg_b * global_brightness) / 255;
        
        display.setPixel(screen_x, screen_y, RGB(avg_r, avg_g, avg_b));
      }
    }
  }
  
  /** Panel 1: CMY Triangle with 2x2 supersampling antialiasing
   *  Equilateral-ish triangle with vertices at:
   *  - Top vertex (center-top): CYAN (0, 255, 255)
   *  - Bottom-left vertex: MAGENTA (255, 0, 255)
   *  - Bottom-right vertex: YELLOW (255, 255, 0)
   */
  
  // Different motion pattern for panel 1 (phase shifted, keep as float)
  float x_offset1 = 82.0f + 10.0f * cos8((uint8_t)(animation_time * 1.2f)) / 128.0f;
  float y_offset1 = 2.0f + 6.0f * sin8((uint8_t)(animation_time * 1.8f)) / 128.0f;
  
  // Calculate bounding box for efficient rendering
  int min_x1 = (int)(x_offset1 - 1);
  int max_x1 = (int)(x_offset1 + tri_size + 1);
  int min_y1 = (int)(y_offset1 - 1);
  int max_y1 = (int)(y_offset1 + tri_size + 1);
  
  // Clamp to panel 1 bounds (64-127)
  if(min_x1 < 64) min_x1 = 64;
  if(max_x1 > 127) max_x1 = 127;
  if(min_y1 < 0) min_y1 = 0;
  if(max_y1 >= height) max_y1 = height - 1;
  
  // Draw CMY triangle with 2x2 supersampling antialiasing
  for(int screen_y = min_y1; screen_y <= max_y1; screen_y++){
    for(int screen_x = min_x1; screen_x <= max_x1; screen_x++){
      // 2x2 supersampling: sample at 4 sub-pixel locations
      int total_r = 0, total_g = 0, total_b = 0;
      int sample_count = 0;
      
      // Sample offsets: 0.25 and 0.75 within pixel for 2x2 grid
      float offsets[2] = {0.25f, 0.75f};
      
      for(int sy = 0; sy < 2; sy++){
        for(int sx = 0; sx < 2; sx++){
          // Convert screen coordinates to triangle-local coordinates
          float sample_x = (screen_x + offsets[sx]) - x_offset1;
          float sample_y = (screen_y + offsets[sy]) - y_offset1;
          
          uint8_t r, g, b;
          if(sampleTriangleColor(sample_x, sample_y, v0_x, v0_y, v1_x, v1_y, v2_x, v2_y, true, r, g, b)){
            total_r += r;
            total_g += g;
            total_b += b;
            sample_count++;
          }
        }
      }
      
      // Average the samples for antialiasing
      if(sample_count > 0){
        uint8_t avg_r = total_r / sample_count;
        uint8_t avg_g = total_g / sample_count;
        uint8_t avg_b = total_b / sample_count;
        
        // Blend with background based on coverage
        float coverage = sample_count / 4.0f;
        avg_r = (uint8_t)(avg_r * coverage);
        avg_g = (uint8_t)(avg_g * coverage);
        avg_b = (uint8_t)(avg_b * coverage);
        
        // Apply global brightness
        avg_r = (avg_r * global_brightness) / 255;
        avg_g = (avg_g * global_brightness) / 255;
        avg_b = (avg_b * global_brightness) / 255;
        
        display.setPixel(screen_x, screen_y, RGB(avg_r, avg_g, avg_b));
      }
    }
  }
  
  // Update animation with smoother increment (smaller steps for fluid motion)
  animation_time += 0.5f;
  
  // Update brightness fade: 0→255 in 1 second, 255→0 in 1 second (2 second cycle)
  // Using time-based calculation for smooth fade independent of frame rate
  brightness_phase += 0.1f;  // ~60 updates per second = smooth fade
  if(brightness_phase >= 6.28318530718f) brightness_phase -= 6.28318530718f;  // Wrap at 2π
  
  // Calculate brightness using sine wave: 0→1→0 over 2 seconds
  float brightness_norm = (sin8((uint8_t)(brightness_phase * 40.6f)) / 255.0f);  // 0 to 1
  global_brightness = (uint8_t)(brightness_norm * 255);
  
  display.show();
}

void updatePlasmaDisplay(){
  /** Generate pattern and update display (controls both OE pins) */ 
  generatePlasmaPattern(display);
  display.show();
}

extern "C" void app_main(){
  ESP_LOGI(TAG, "HUB75 Dual Display Starting");
  
  /** Disable watchdog timer */
  esp_task_wdt_deinit();
  
  /** Configure display with dual OE pins */
  HUB75Config config = HUB75Config::getDefault();
  config.enable_gamma_correction = true;  // Enable built-in gamma correction
  config.gamma_value = 2.2f;
  config.dual_display_mode = true;        // Enable dual display spillover
  config.effective_width = 128;           // 64x2 = 128 pixels wide
  
  /** Panel inversion: Flip panel 0 (RGB triangle) vertically */
  config.panel_inversions[0].flip_vertical = true;   // Panel 0: flip upside down
  config.panel_inversions[1].flip_vertical = false;  // Panel 1: normal orientation
  
  /** Use the correct working pin configuration */
  config.pins.r0_pin = 7;   // Red 0
  config.pins.g0_pin = 15;  // Green 0  
  config.pins.b0_pin = 16;  // Blue 0
  config.pins.r1_pin = 17;  // Red 1
  config.pins.g1_pin = 18;  // Green 1
  config.pins.b1_pin = 8;   // Blue 1
  config.pins.a_pin = 41;   // Address A
  config.pins.b_pin = 40;   // Address B
  config.pins.c_pin = 39;   // Address C
  config.pins.d_pin = 38;   // Address D
  config.pins.e_pin = 42;   // Address E
  config.pins.lat_pin = 36; // Latch
  config.pins.oe_pin = 35;  // Primary Output Enable
  config.pins.oe_pin2 = 6;  // Secondary Output Enable
  config.pins.clock_pin = 37; // Clock
  
  /** Initialise display with dual OE support */
  if(!display.init(config)){
    ESP_LOGE(TAG, "Failed to initialise HUB75 display");
    return;
  }
  
  /** Start display */
  if(!display.start()){
    ESP_LOGE(TAG, "Failed to start display");
    return;
  }

  /** Calculate memory usage */
  size_t free_heap = esp_get_free_heap_size();
  size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_8BIT);
  size_t used_heap = total_heap - free_heap;
  
  int fb_width = config.dual_display_mode ? config.effective_width : config.matrix_width;
  size_t framebuffer_size = fb_width * config.matrix_height * 3; // RGB bytes
  size_t dma_buffer_size = config.matrix_width * (config.matrix_height / 2) * config.colour_depth * 2 * 2; // 2 buffers, 2 bytes each
  
  ESP_LOGI(TAG, "=== DUAL DISPLAY CONFIGURATION ===");
  ESP_LOGI(TAG, "  Effective Canvas: %dx%d pixels (spans both displays)", display.getWidth(), display.getHeight());
  ESP_LOGI(TAG, "  Physical Displays: 2x 64x32 panels");
  ESP_LOGI(TAG, "  Primary OE: pin %d, Secondary OE: pin %d", 
           config.pins.oe_pin, config.pins.oe_pin2);
  ESP_LOGI(TAG, "  Colour depth: 5-bit BCM with built-in gamma correction (%.1f)", config.gamma_value);
  ESP_LOGI(TAG, "  Anti-aliasing: 2x2 supersampling");
  ESP_LOGI(TAG, "");
  ESP_LOGI(TAG, "=== MEMORY USAGE ===");
  ESP_LOGI(TAG, "  Frame buffer: %d bytes", framebuffer_size);
  ESP_LOGI(TAG, "  DMA buffers: %d bytes (dual buffered)", dma_buffer_size);
  ESP_LOGI(TAG, "  Total RAM used: %d KB / %d KB", used_heap / 1024, total_heap / 1024);
  ESP_LOGI(TAG, "  Free RAM: %d KB", free_heap / 1024);
  ESP_LOGI(TAG, "");
  ESP_LOGI(TAG, "=== PERFORMANCE TARGET ===");
  ESP_LOGI(TAG, "  Target FPS: 60");
  
  /** Initialise random seed */
  srand(esp_timer_get_time());
  
  /** Initialise performance optimisation caches */
  initialisePerformanceCaches();

  /** Main monitoring loop */
  uint32_t last_fps_report_time = 0;
  uint32_t frame_count = 0;
  uint32_t fps_counter = 0;
  
  ESP_LOGI(TAG, "Animating moving LUT triangles:");
  ESP_LOGI(TAG, "  Panel 0: RGB triangle (R=top, G=bottom-left, B=bottom-right)");
  ESP_LOGI(TAG, "  Panel 1: CMY triangle (C=top, M=bottom-left, Y=bottom-right)");
  ESP_LOGI(TAG, "  2x2 supersampling antialiasing for smooth edges");
  ESP_LOGI(TAG, "  Smooth sine wave motion with barycentric color interpolation");
  
  /** Animation update interval */
  uint64_t last_animation_update = 0;
  const uint64_t animation_update_interval = 16667;  // ~60 FPS (16.67ms per frame)
  
  while(true){
    uint64_t current_time_us = esp_timer_get_time();
    
    /** Update animation every ~16ms for smooth 60 FPS motion */
    if(current_time_us - last_animation_update >= animation_update_interval){
      // Draw the moving LUT triangles
      drawMovingLUTTriangles();
      
      last_animation_update = current_time_us;
    }
    
    frame_count++;
    fps_counter++;
    
    /** Report FPS every 1 second for performance testing */
    if(current_time_us - last_fps_report_time >= 1000000){
      float actual_fps = fps_counter / 1.0f;
      
      /** Get current memory usage */
      size_t free_heap = esp_get_free_heap_size();
      size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_8BIT);
      
      /** Calculate buffer sizes */
      int fb_width = config.dual_display_mode ? config.effective_width : config.matrix_width;
      size_t framebuffer_size = fb_width * config.matrix_height * 3;
      size_t dma_buffer_size = config.matrix_width * (config.matrix_height / 2) * config.colour_depth * 2 * 2;
      
      ESP_LOGI(TAG, "=== LUT TRIANGLE ANIMATION STATUS ===");
      ESP_LOGI(TAG, "  Animation FPS: %.1f", actual_fps);
      ESP_LOGI(TAG, "  Panel 0: RGB Triangle | Panel 1: CMY Triangle");
      ESP_LOGI(TAG, "  Animation time: %.1f", animation_time);
      ESP_LOGI(TAG, "  Frame Buffer: %d bytes", framebuffer_size);
      ESP_LOGI(TAG, "  DMA Buffers: %d bytes", dma_buffer_size);
      ESP_LOGI(TAG, "  Free RAM: %d KB / %d KB (%.1f%%)", 
               free_heap / 1024, total_heap / 1024, 
               (free_heap * 100.0f) / total_heap);
      ESP_LOGI(TAG, "  Total Frames: %d", frame_count);
      
      fps_counter = 0;
      last_fps_report_time = current_time_us;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));  // Check status every 100ms
  }
}