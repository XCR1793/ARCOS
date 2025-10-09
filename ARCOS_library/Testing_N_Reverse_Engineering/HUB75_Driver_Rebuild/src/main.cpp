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
#include "panel_config.hpp"

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

void updatePlasmaDisplay(){
  /** Generate pattern and update display (controls both OE pins) */ 
  static uint32_t switch_time = 0;
  
  /** Switch between plasma and simple patterns every 10 seconds */
  uint32_t current_ms = esp_timer_get_time() / 1000;

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
  
  /** Use ARCOS custom pin configuration from panel_config.hpp */
  config.pins.r0_pin = PanelConfig::PinConfig::R1;
  config.pins.g0_pin = PanelConfig::PinConfig::G1;
  config.pins.b0_pin = PanelConfig::PinConfig::B1;
  config.pins.r1_pin = PanelConfig::PinConfig::R2;
  config.pins.g1_pin = PanelConfig::PinConfig::G2;
  config.pins.b1_pin = PanelConfig::PinConfig::B2;
  config.pins.a_pin = PanelConfig::PinConfig::A;
  config.pins.b_pin = PanelConfig::PinConfig::B;
  config.pins.c_pin = PanelConfig::PinConfig::C;
  config.pins.d_pin = PanelConfig::PinConfig::D;
  config.pins.e_pin = PanelConfig::PinConfig::E;
  config.pins.lat_pin = PanelConfig::PinConfig::LAT;
  config.pins.oe_pin = PanelConfig::PinConfig::OE;
  config.pins.oe_pin2 = 6;  // Secondary Output Enable (if needed)
  config.pins.clock_pin = PanelConfig::PinConfig::CLK;
  
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

  /** Main animation loop */
  uint32_t last_update_time = 0;
  uint32_t last_fps_report_time = 0;
  uint32_t last_palette_change_time = 0;
  uint32_t frame_count = 0;
  uint32_t fps_counter = 0;
  const uint32_t frame_interval_us = 13333; // ~75 FPS target (faster than 60)
  
  /** Brightness fade parameters */
  uint32_t last_brightness_update = 0;
  const uint32_t brightness_update_interval = 20000; // Update every 20ms for smooth fade
  float brightness_value = 128.0f; // Start at mid brightness
  float brightness_direction = 1.0f; // 1.0 = increasing, -1.0 = decreasing
  const float brightness_speed = 2.0f; // Brightness change per update (faster fade)
  const uint8_t min_brightness = 0; // Minimum brightness (completely off)
  const uint8_t max_brightness = 255; // Maximum brightness
  
  const char* palette_names[] = {"Heat", "Rainbow", "Lava", "Cloud", "Blueish"};
  
  ESP_LOGI(TAG, "Starting plasma animation with brightness fade");
  ESP_LOGI(TAG, "Brightness range: %d to %d", min_brightness, max_brightness);
  
  while(true){
    uint64_t current_time_us = esp_timer_get_time();
    
    /** Update brightness fade */
    if(current_time_us - last_brightness_update >= brightness_update_interval){
      brightness_value += brightness_direction * brightness_speed;
      
      /** Reverse direction at limits */
      if(brightness_value >= max_brightness){
        brightness_value = max_brightness;
        brightness_direction = -1.0f;
      } else if(brightness_value <= min_brightness){
        brightness_value = min_brightness;
        brightness_direction = 1.0f;
      }
      
      display.setBrightness((uint8_t)brightness_value);
      last_brightness_update = current_time_us;
    }
    
    /** Update frame - UNLIMITED FRAMERATE TEST */
    updatePlasmaDisplay();
    frame_count++;
    fps_counter++;
    last_update_time = current_time_us;
    
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
      
      ESP_LOGI(TAG, "=== MAXIMUM FPS BENCHMARK ===");
      ESP_LOGI(TAG, "  Current FPS: %.1f (UNLIMITED)", actual_fps);
      ESP_LOGI(TAG, "  Brightness: %d / 255 (%.1f%%)", 
               display.getBrightness(), 
               (display.getBrightness() * 100.0f) / 255.0f);
      ESP_LOGI(TAG, "  Palette: %s", palette_names[currentPaletteIndex]);
      ESP_LOGI(TAG, "  Frame Buffer: %d bytes", framebuffer_size);
      ESP_LOGI(TAG, "  DMA Buffers: %d bytes", dma_buffer_size);
      ESP_LOGI(TAG, "  Free RAM: %d KB / %d KB (%.1f%%)", 
               free_heap / 1024, total_heap / 1024, 
               (free_heap * 100.0f) / total_heap);
      ESP_LOGI(TAG, "  Total Frames: %d", frame_count);
      
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