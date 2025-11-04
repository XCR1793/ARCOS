/*****************************************************************
 * File:      AdvancedHUB75Demo.cpp
 * Category:  tests/abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Advanced HUB75 demo showing custom configuration options
 *    and advanced features with the simplified driver.
 *****************************************************************/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <math.h>

#include "abstraction/drivers/components/HUB75/driver_hub75_simple.hpp"

using namespace arcos::abstraction::drivers;

static const char* TAG = "ADVANCED_DEMO";

/** Advanced shape drawing - swaying polygon with RGB shader */
void drawSwayingShape(SimpleHUB75Display& display, float offset_x, float offset_y, float hue_offset) {
  const int panel_width = 64;
  
  // Draw on both panels
  for(int panel = 0; panel < 2; panel++) {
    int panel_x_offset = panel * panel_width;
    
    // Base shape coordinates
    const float center_offset_x = 15.0f;
    const float center_offset_y = 3.0f;
    
    float base_points[][2] = {
      {6, 8}, {14, 8}, {20, 11}, {26, 17}, {27, 19}, {28, 22}, 
      {23, 22}, {21, 19}, {19, 17}, {17, 17}, {16, 19}, {18, 22}, 
      {7, 22}, {4, 20}, {2, 17}, {2, 12}
    };
    int num_points = sizeof(base_points) / sizeof(base_points[0]);
    
    // Apply offset for animation
    for(int i = 0; i < num_points; i++) {
      float x = base_points[i][0] + center_offset_x + offset_x;
      float y = base_points[i][1] + center_offset_y + offset_y;
      
      // Simple fill with rainbow shader
      if(i < num_points - 1) {
        int x0 = (int)x;
        int y0 = (int)y;
        int x1 = (int)(base_points[i+1][0] + center_offset_x + offset_x);
        int y1 = (int)(base_points[i+1][1] + center_offset_y + offset_y);
        
        // Draw line
        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;
        
        while(true) {
          float hue = fmodf(hue_offset + (x0 * 15.0f) + (y0 * 10.0f), 360.0f);
          
          // Simple HSL to RGB
          float h = hue / 60.0f;
          float c = 0.5f;  // 50% lightness, 100% saturation
          float x_val = c * (1.0f - fabsf(fmodf(h, 2.0f) - 1.0f));
          
          uint8_t r, g, b;
          if(h < 1) { r = 255; g = (uint8_t)(x_val * 255); b = 0; }
          else if(h < 2) { r = (uint8_t)(x_val * 255); g = 255; b = 0; }
          else if(h < 3) { r = 0; g = 255; b = (uint8_t)(x_val * 255); }
          else if(h < 4) { r = 0; g = (uint8_t)(x_val * 255); b = 255; }
          else if(h < 5) { r = (uint8_t)(x_val * 255); g = 0; b = 255; }
          else { r = 255; g = 0; b = (uint8_t)(x_val * 255); }
          
          display.setPixel(panel_x_offset + x0, y0, RGB(r, g, b));
          
          if(x0 == x1 && y0 == y1) break;
          
          int e2 = 2 * err;
          if(e2 > -dy) {
            err -= dy;
            x0 += sx;
          }
          if(e2 < dx) {
            err += dx;
            y0 += sy;
          }
        }
      }
    }
  }
}

extern "C" void app_main() {
  vTaskDelay(pdMS_TO_TICKS(1000));
  
  ESP_LOGI(TAG, "=== Advanced HUB75 Demo ===");
  ESP_LOGI(TAG, "Demonstrating custom configuration and advanced features");
  ESP_LOGI(TAG, "");
  
  // Method 1: Use a preset configuration
  // SimpleHUB75Display display;
  // display.begin(HUB75Presets::dualPanelHorizontal());
  
  // Method 2: Create custom configuration
  HUB75Config config = HUB75Config::getDefault();
  
  // Customize settings
  config.dual_display_mode = true;
  config.effective_width = 128;
  config.enable_gamma_correction = true;
  config.gamma_value = 2.2f;
  config.colour_depth = 5;
  
  // Panel orientation (flip panel 0 vertically)
  config.panel_inversions[0].flip_vertical = true;
  config.panel_inversions[1].flip_vertical = false;
  
  // Pin configuration (already set correctly in defaults, but can override)
  config.pins.r0_pin = 7;
  config.pins.g0_pin = 15;
  config.pins.b0_pin = 16;
  config.pins.r1_pin = 17;
  config.pins.g1_pin = 18;
  config.pins.b1_pin = 8;
  config.pins.lat_pin = 36;
  config.pins.oe_pin = 35;
  config.pins.oe_pin2 = 6;  // Dual OE support
  config.pins.a_pin = 41;
  config.pins.b_pin = 40;
  config.pins.c_pin = 39;
  config.pins.d_pin = 38;
  config.pins.e_pin = 42;
  config.pins.clock_pin = 37;
  
  // Initialize with custom config
  SimpleHUB75Display display;
  if(!display.begin(config)) {
    ESP_LOGE(TAG, "Failed to initialize display");
    return;
  }
  
  ESP_LOGI(TAG, "Display ready: %dx%d pixels", display.getWidth(), display.getHeight());
  ESP_LOGI(TAG, "Gamma correction: %s (%.1f)", 
           config.enable_gamma_correction ? "enabled" : "disabled",
           config.gamma_value);
  ESP_LOGI(TAG, "Color depth: %d bits", config.colour_depth);
  ESP_LOGI(TAG, "");
  
  // Animation variables
  float time = 0.0f;
  float hue_offset = 0.0f;
  int brightness = 255;
  bool brightness_increasing = false;
  
  // Main animation loop
  while(true) {
    // Calculate smooth swaying motion
    float offset_x = sinf(time * 1.0f) * 3.0f;
    float offset_y = sinf(time * 1.4f) * 2.0f;
    
    // Clear and draw
    display.clear();
    drawSwayingShape(display, offset_x, offset_y, hue_offset);
    display.show();
    
    // Update animation
    time += 0.08f;
    hue_offset += 8.0f;
    if(hue_offset >= 360.0f) {
      hue_offset -= 360.0f;
    }
    
    // Demonstrate brightness control (pulse every few seconds)
    if((int)time % 10 == 0) {
      if(brightness_increasing) {
        brightness = (brightness + 5) % 256;
        if(brightness >= 250) brightness_increasing = false;
      } else {
        brightness = (brightness - 5);
        if(brightness <= 100) brightness_increasing = true;
      }
      display.setBrightness(brightness);
    }
    
    vTaskDelay(pdMS_TO_TICKS(16));
  }
}
