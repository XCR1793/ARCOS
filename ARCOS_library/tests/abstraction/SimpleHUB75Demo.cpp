/*****************************************************************
 * File:      SimpleHUB75Demo.cpp
 * Category:  tests/abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Minimal HUB75 display demo using the simplified driver.
 *    Shows how easy it is to get started with just a few lines!
 *****************************************************************/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <math.h>

// Just include the simple driver - that's it!
#include "abstraction/drivers/components/HUB75/driver_hub75_simple.hpp"

using namespace arcos::abstraction::drivers;

static const char* TAG = "SIMPLE_DEMO";

/** Simple RGB color helper */
struct Color {
  static RGB red()     { return RGB(255, 0, 0); }
  static RGB green()   { return RGB(0, 255, 0); }
  static RGB blue()    { return RGB(0, 0, 255); }
  static RGB white()   { return RGB(255, 255, 255); }
  static RGB black()   { return RGB(0, 0, 0); }
  static RGB yellow()  { return RGB(255, 255, 0); }
  static RGB cyan()    { return RGB(0, 255, 255); }
  static RGB magenta() { return RGB(255, 0, 255); }
  
  // HSL to RGB conversion for rainbow effects
  static RGB fromHSL(float h, float s, float l) {
    h = fmodf(h, 360.0f);
    if(h < 0) h += 360.0f;
    s = fminf(fmaxf(s, 0.0f), 100.0f) / 100.0f;
    l = fminf(fmaxf(l, 0.0f), 100.0f) / 100.0f;
    
    float c = (1.0f - fabsf(2.0f * l - 1.0f)) * s;
    float h_prime = h / 60.0f;
    float x = c * (1.0f - fabsf(fmodf(h_prime, 2.0f) - 1.0f));
    float m = l - c / 2.0f;
    
    float r1, g1, b1;
    if(h_prime >= 0 && h_prime < 1){
      r1 = c; g1 = x; b1 = 0;
    }else if(h_prime >= 1 && h_prime < 2){
      r1 = x; g1 = c; b1 = 0;
    }else if(h_prime >= 2 && h_prime < 3){
      r1 = 0; g1 = c; b1 = x;
    }else if(h_prime >= 3 && h_prime < 4){
      r1 = 0; g1 = x; b1 = c;
    }else if(h_prime >= 4 && h_prime < 5){
      r1 = x; g1 = 0; b1 = c;
    }else{
      r1 = c; g1 = 0; b1 = x;
    }
    
    return RGB(
      (uint8_t)roundf((r1 + m) * 255.0f),
      (uint8_t)roundf((g1 + m) * 255.0f),
      (uint8_t)roundf((b1 + m) * 255.0f)
    );
  }
};

/** Draw a line using Bresenham's algorithm */
void drawLine(SimpleHUB75Display& display, int x0, int y0, int x1, int y1, const RGB& color) {
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;
  
  while(true) {
    display.setPixel(x0, y0, color);
    
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

/** Draw a filled rectangle */
void fillRect(SimpleHUB75Display& display, int x, int y, int w, int h, const RGB& color) {
  for(int dy = 0; dy < h; dy++) {
    for(int dx = 0; dx < w; dx++) {
      display.setPixel(x + dx, y + dy, color);
    }
  }
}

/** Draw a circle outline */
void drawCircle(SimpleHUB75Display& display, int cx, int cy, int radius, const RGB& color) {
  int x = radius;
  int y = 0;
  int err = 0;
  
  while(x >= y) {
    display.setPixel(cx + x, cy + y, color);
    display.setPixel(cx + y, cy + x, color);
    display.setPixel(cx - y, cy + x, color);
    display.setPixel(cx - x, cy + y, color);
    display.setPixel(cx - x, cy - y, color);
    display.setPixel(cx - y, cy - x, color);
    display.setPixel(cx + y, cy - x, color);
    display.setPixel(cx + x, cy - y, color);
    
    if(err <= 0) {
      y += 1;
      err += 2 * y + 1;
    }
    if(err > 0) {
      x -= 1;
      err -= 2 * x + 1;
    }
  }
}

extern "C" void app_main() {
  // Small delay for serial to stabilize
  vTaskDelay(pdMS_TO_TICKS(1000));
  
  ESP_LOGI(TAG, "=== Simple HUB75 Demo ===");
  ESP_LOGI(TAG, "Demonstrating the simplified driver interface");
  ESP_LOGI(TAG, "");
  
  // Create display with default configuration
  SimpleHUB75Display display;
  
  // Initialize - that's it! All the complexity is handled for you
  if(!display.begin()) {
    ESP_LOGE(TAG, "Failed to initialize display");
    return;
  }
  
  ESP_LOGI(TAG, "Display initialized: %dx%d pixels", 
           display.getWidth(), display.getHeight());
  ESP_LOGI(TAG, "Starting animations...");
  ESP_LOGI(TAG, "");
  
  // Animation variables
  float time = 0.0f;
  int demo = 0;
  int demo_counter = 0;
  
  // Main loop - draw different demos
  while(true) {
    // Switch demos every 5 seconds
    if(demo_counter >= 300) {  // 300 frames * 16ms ≈ 5 seconds
      demo = (demo + 1) % 5;
      demo_counter = 0;
    }
    demo_counter++;
    
    // Clear screen
    display.clear();
    
    switch(demo) {
      case 0: {
        // Demo 1: Bouncing rainbow dots
        ESP_LOGI(TAG, "Demo: Bouncing rainbow dots");
        for(int i = 0; i < 10; i++) {
          float angle = time * 2.0f + i * 36.0f;
          int x = (int)(display.getWidth() / 2 + cosf(angle * 0.0174533f) * 30.0f);
          int y = (int)(display.getHeight() / 2 + sinf(angle * 0.0174533f) * 12.0f);
          
          RGB color = Color::fromHSL(i * 36.0f + time * 10.0f, 100.0f, 50.0f);
          display.setPixel(x, y, color);
        }
        break;
      }
      
      case 1: {
        // Demo 2: Rainbow wave
        for(int x = 0; x < display.getWidth(); x++) {
          float wave = sinf(x * 0.1f + time * 0.1f) * display.getHeight() * 0.3f;
          int y = (int)(display.getHeight() / 2 + wave);
          
          RGB color = Color::fromHSL((x * 3.0f + time * 10.0f), 100.0f, 50.0f);
          display.setPixel(x, y, color);
        }
        break;
      }
      
      case 2: {
        // Demo 3: Spinning lines
        int cx = display.getWidth() / 2;
        int cy = display.getHeight() / 2;
        
        for(int i = 0; i < 8; i++) {
          float angle = time * 5.0f + i * 45.0f;
          int x1 = (int)(cx + cosf(angle * 0.0174533f) * 40.0f);
          int y1 = (int)(cy + sinf(angle * 0.0174533f) * 15.0f);
          
          RGB color = Color::fromHSL(i * 45.0f, 100.0f, 50.0f);
          drawLine(display, cx, cy, x1, y1, color);
        }
        break;
      }
      
      case 3: {
        // Demo 4: Pulsing circles
        int cx = display.getWidth() / 2;
        int cy = display.getHeight() / 2;
        
        for(int i = 0; i < 3; i++) {
          float phase = time * 3.0f + i * 2.0f;
          int radius = (int)(5.0f + sinf(phase) * 5.0f);
          
          RGB color = Color::fromHSL(i * 120.0f + time * 5.0f, 100.0f, 50.0f);
          drawCircle(display, cx, cy, radius, color);
        }
        break;
      }
      
      case 4: {
        // Demo 5: Sliding rectangles
        for(int i = 0; i < 3; i++) {
          int x = (int)((time * 20.0f + i * 40.0f)) % (display.getWidth() + 20) - 10;
          int y = 5 + i * 10;
          
          RGB color = Color::fromHSL(i * 120.0f, 100.0f, 50.0f);
          fillRect(display, x, y, 8, 6, color);
        }
        break;
      }
    }
    
    // Update display
    display.show();
    
    // Update animation
    time += 0.1f;
    
    // Run at ~60 FPS
    vTaskDelay(pdMS_TO_TICKS(16));
  }
}
