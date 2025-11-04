/*****************************************************************
 * File:      LineDrawingDemo.cpp
 * Category:  tests/abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Animated line drawing demo for HUB75 LED matrix displays
 *    demonstrating the ARCOS abstraction framework with looping
 *    line patterns on dual displays.
 *****************************************************************/

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

// ARCOS-style abstraction
#include "abstraction/hal.hpp"       // All HAL APIs + platform implementation
#include "abstraction/drivers/components/HUB75/driver_hub75.hpp"      // HUB75 display driver
#include "abstraction/drivers/components/HUB75/driver_hub75_i2s.hpp"  // I2S protocol for HUB75

using namespace arcos::abstraction;
using namespace arcos::abstraction::drivers;

static const char* TAG = "LINE_DEMO";

/** Platform implementations (injected into protocol) */
static HAL_PARALLEL_DEFAULT hardware;
static ParallelBuffer bufferManager;

/** HUB75 I2S protocol implementation */
static HUB75_I2S_Protocol i2sProtocol;

/** HUB75 display driver with dual OE support */
static HUB75Driver display;

/** RGB colour structure */
struct CRGB {
  uint8_t r, g, b;
  CRGB() : r(0), g(0), b(0) {}
  CRGB(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
};

/** HSL to RGB conversion
 * H: 0-360 degrees (hue)
 * S: 0-100 percent (saturation)
 * L: 0-100 percent (lightness/luminosity)
 * Returns RGB with 0-255 values
 */
CRGB hslToRgb(float h, float s, float l){
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
  
  uint8_t r = (uint8_t)roundf((r1 + m) * 255.0f);
  uint8_t g = (uint8_t)roundf((g1 + m) * 255.0f);
  uint8_t b = (uint8_t)roundf((b1 + m) * 255.0f);
  
  return CRGB(r, g, b);
}

/** Draw a line between two points using Bresenham's algorithm */
void drawLine(int x0, int y0, int x1, int y1, int panel_offset, const RGB& color){
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;
  
  while(true){
    display.setPixel(panel_offset + x0, y0, color);
    
    if(x0 == x1 && y0 == y1) break;
    
    int e2 = 2 * err;
    if(e2 > -dy){
      err -= dy;
      x0 += sx;
    }
    if(e2 < dx){
      err += dx;
      y0 += sy;
    }
  }
}

/** Calculate coverage percentage for a pixel based on distance from line segment */
float calculateLineCoverage(float px, float py, float x0, float y0, float x1, float y1, float line_width){
  // Calculate distance from pixel center to line segment
  float dx = x1 - x0;
  float dy = y1 - y0;
  float len_sq = dx * dx + dy * dy;
  
  if(len_sq < 0.0001f){
    // Line segment is a point
    float dist = sqrtf((px - x0) * (px - x0) + (py - y0) * (py - y0));
    return fmaxf(0.0f, fminf(1.0f, 1.0f - (dist / line_width)));
  }
  
  // Project pixel center onto line segment
  float t = ((px - x0) * dx + (py - y0) * dy) / len_sq;
  t = fmaxf(0.0f, fminf(1.0f, t));
  
  float closest_x = x0 + t * dx;
  float closest_y = y0 + t * dy;
  
  float dist = sqrtf((px - closest_x) * (px - closest_x) + (py - closest_y) * (py - closest_y));
  
  // Calculate coverage based on distance (linear falloff)
  return fmaxf(0.0f, fminf(1.0f, 1.0f - (dist / line_width)));
}

/** Draw a line with subpixel coverage-based brightness */
void drawLineWithCoverage(float x0, float y0, float x1, float y1, int panel_offset, const RGB& color, float line_width){
  // Calculate bounding box for the line
  int min_x = (int)floorf(fminf(x0, x1) - line_width);
  int max_x = (int)ceilf(fmaxf(x0, x1) + line_width);
  int min_y = (int)floorf(fminf(y0, y1) - line_width);
  int max_y = (int)ceilf(fmaxf(y0, y1) + line_width);
  
  // Clamp to panel bounds
  min_x = fmaxf(0, min_x);
  max_x = fminf(63, max_x);
  min_y = fmaxf(0, min_y);
  max_y = fminf(31, max_y);
  
  // For each pixel in the bounding box
  for(int py = min_y; py <= max_y; py++){
    for(int px = min_x; px <= max_x; px++){
      // Calculate coverage for this pixel
      float coverage = calculateLineCoverage((float)px + 0.5f, (float)py + 0.5f, x0, y0, x1, y1, line_width);
      
      if(coverage > 0.01f){
        // Apply coverage as brightness multiplier
        uint8_t r = (uint8_t)(color.r * coverage);
        uint8_t g = (uint8_t)(color.g * coverage);
        uint8_t b = (uint8_t)(color.b * coverage);
        display.setPixel(panel_offset + px, py, RGB(r, g, b));
      }
    }
  }
}

/** Calculate coverage for a point inside/outside polygon */
float calculatePolygonCoverage(float px, float py, float points[][2], int num_points){
  // Check if inside using ray casting
  bool inside = false;
  float min_edge_dist = 1000.0f;
  
  for(int i = 0; i < num_points; i++){
    int j = (i + 1) % num_points;
    float x0 = points[i][0];
    float y0 = points[i][1];
    float x1 = points[j][0];
    float y1 = points[j][1];
    
    // Ray casting
    if(((y0 > py) != (y1 > py)) && 
       (px < (x1 - x0) * (py - y0) / (y1 - y0) + x0)){
      inside = !inside;
    }
    
    // Distance to edge
    float dx = x1 - x0;
    float dy = y1 - y0;
    float len_sq = dx * dx + dy * dy;
    
    if(len_sq > 0.0001f){
      float t = ((px - x0) * dx + (py - y0) * dy) / len_sq;
      t = fmaxf(0.0f, fminf(1.0f, t));
      float closest_x = x0 + t * dx;
      float closest_y = y0 + t * dy;
      float dist = sqrtf((px - closest_x) * (px - closest_x) + (py - closest_y) * (py - closest_y));
      min_edge_dist = fminf(min_edge_dist, dist);
    }
  }
  
  if(inside){
    return 1.0f;  // Fully inside
  } else {
    // Partially covered if near edge
    return fmaxf(0.0f, fminf(1.0f, 1.0f - min_edge_dist));
  }
}

/** Draw a line between two points using Bresenham's algorithm with subpixel offset */
void drawLineSubpixel(float x0, float y0, float x1, float y1, int panel_offset, const RGB& color){
  // Round to nearest pixel for clean lines
  int ix0 = (int)roundf(x0);
  int iy0 = (int)roundf(y0);
  int ix1 = (int)roundf(x1);
  int iy1 = (int)roundf(y1);
  
  int dx = abs(ix1 - ix0);
  int dy = abs(iy1 - iy0);
  int sx = (ix0 < ix1) ? 1 : -1;
  int sy = (iy0 < iy1) ? 1 : -1;
  int err = dx - dy;
  
  while(true){
    display.setPixel(panel_offset + ix0, iy0, color);
    
    if(ix0 == ix1 && iy0 == iy1) break;
    
    int e2 = 2 * err;
    if(e2 > -dy){
      err -= dy;
      ix0 += sx;
    }
    if(e2 < dx){
      err += dx;
      iy0 += sy;
    }
  }
}

/** Scanline fill for polygon with subpixel coordinates */
void fillPolygonSubpixel(float points[][2], int num_points, int panel_offset, const RGB& color){
  const int panel_height = 32;
  
  // For each scanline
  for(int y = 0; y < panel_height; y++){
    float intersections[32];
    int intersection_count = 0;
    
    // Find all edge intersections with this scanline
    for(int i = 0; i < num_points; i++){
      float x0 = points[i][0];
      float y0 = points[i][1];
      float x1 = points[(i + 1) % num_points][0];
      float y1 = points[(i + 1) % num_points][1];
      
      // Check if edge crosses this scanline
      if((y0 <= y && y1 > y) || (y1 <= y && y0 > y)){
        // Calculate x intersection
        float t = (float)(y - y0) / (float)(y1 - y0);
        float x = x0 + t * (x1 - x0);
        intersections[intersection_count++] = x;
      }
    }
    
    // Sort intersections
    for(int i = 0; i < intersection_count - 1; i++){
      for(int j = i + 1; j < intersection_count; j++){
        if(intersections[i] > intersections[j]){
          float temp = intersections[i];
          intersections[i] = intersections[j];
          intersections[j] = temp;
        }
      }
    }
    
    // Fill between pairs of intersections
    for(int i = 0; i < intersection_count - 1; i += 2){
      int x_start = (int)roundf(intersections[i]);
      int x_end = (int)roundf(intersections[i + 1]);
      for(int x = x_start; x <= x_end; x++){
        display.setPixel(panel_offset + x, y, color);
      }
    }
  }
}

/** Draw shape with red fill and outline, then apply RGB shader */
void drawShapeWithRGBShader(int panel_index, float offset_x, float offset_y, float hue_offset){
  const int panel_width = 64;
  const int panel_height = 32;
  int panel_x_offset = panel_index * panel_width;
  
  // Array of points to draw (base coordinates)
  // Original shape ranges from x: 2-28 (26 wide), y: 8-22 (14 tall)
  // Add center offset to keep shape away from edges
  // Shape center should be around panel center (32, 16)
  // With sway of ±3 in x and ±2 in y, we need padding
  const float center_offset_x = 15.0f;  // Centers shape horizontally with margin
  const float center_offset_y = 3.0f;   // Centers shape vertically with margin
  
  float base_points[][2] = {{6, 8}, {14, 8}, {20, 11}, {26, 17}, {27, 19}, {28, 22}, 
                            {23, 22}, {21, 19}, {19, 17}, {17, 17}, {16, 19}, {18, 22}, 
                            {7, 22}, {4, 20}, {2, 17}, {2, 12}};
  int num_points = sizeof(base_points) / sizeof(base_points[0]);
  
  // Apply center offset + subpixel animation offset to points
  float points[16][2];
  for(int i = 0; i < num_points; i++){
    points[i][0] = base_points[i][0] + center_offset_x + offset_x;
    points[i][1] = base_points[i][1] + center_offset_y + offset_y;
  }
  
  // Calculate bounding box for optimization
  float min_x = 64.0f, max_x = 0.0f, min_y = 32.0f, max_y = 0.0f;
  for(int i = 0; i < num_points; i++){
    if(points[i][0] < min_x) min_x = points[i][0];
    if(points[i][0] > max_x) max_x = points[i][0];
    if(points[i][1] < min_y) min_y = points[i][1];
    if(points[i][1] > max_y) max_y = points[i][1];
  }
  
  int bbox_min_x = fmaxf(0, (int)floorf(min_x) - 2);
  int bbox_max_x = fminf(63, (int)ceilf(max_x) + 2);
  int bbox_min_y = fmaxf(0, (int)floorf(min_y) - 2);
  int bbox_max_y = fminf(31, (int)ceilf(max_y) + 2);
  
  // Step 1: Fast fill interior (no coverage calculation)
  for(int y = bbox_min_y; y <= bbox_max_y; y++){
    for(int x = bbox_min_x; x <= bbox_max_x; x++){
      // Quick inside test only
      bool inside = false;
      for(int i = 0; i < num_points; i++){
        int j = (i + 1) % num_points;
        float x0 = points[i][0];
        float y0 = points[i][1];
        float x1 = points[j][0];
        float y1 = points[j][1];
        
        if(((y0 > y) != (y1 > y)) && 
           (x < (x1 - x0) * (y - y0) / (y1 - y0) + x0)){
          inside = !inside;
        }
      }
      
      if(inside){
        // Fast RGB shader - full brightness for interior
        float hue = fmodf(hue_offset + (x * 15.0f) + (y * 10.0f), 360.0f);
        CRGB color = hslToRgb(hue, 100.0f, 50.0f);
        display.setPixel(panel_x_offset + x, y, RGB(color.r, color.g, color.b));
      }
    }
  }
  
  // Step 2: Draw edges with coverage-based smoothing only (overlays on top)
  const float line_width = 1.5f;  // Slightly wider to ensure full coverage
  
  for(int i = 0; i < num_points; i++){
    float x0 = points[i][0];
    float y0 = points[i][1];
    float x1 = points[(i + 1) % num_points][0];
    float y1 = points[(i + 1) % num_points][1];
    
    // Optimized edge rendering with local bounding box
    int edge_min_x = fmaxf(0, (int)floorf(fminf(x0, x1) - line_width));
    int edge_max_x = fminf(63, (int)ceilf(fmaxf(x0, x1) + line_width));
    int edge_min_y = fmaxf(0, (int)floorf(fminf(y0, y1) - line_width));
    int edge_max_y = fminf(31, (int)ceilf(fmaxf(y0, y1) + line_width));
    
    for(int py = edge_min_y; py <= edge_max_y; py++){
      for(int px = edge_min_x; px <= edge_max_x; px++){
        // Check if this pixel is inside the polygon
        bool inside = false;
        for(int j = 0; j < num_points; j++){
          int k = (j + 1) % num_points;
          float px0 = points[j][0];
          float py0 = points[j][1];
          float px1 = points[k][0];
          float py1 = points[k][1];
          
          if(((py0 > py) != (py1 > py)) && 
             (px < (px1 - px0) * (py - py0) / (py1 - py0) + px0)){
            inside = !inside;
          }
        }
        
        // Only apply antialiasing to pixels OUTSIDE the shape (outer edge only)
        if(!inside){
          float coverage = calculateLineCoverage((float)px + 0.5f, (float)py + 0.5f, x0, y0, x1, y1, line_width);
          
          if(coverage > 0.01f){
            // Calculate edge color with RGB shader
            float hue = fmodf(hue_offset + (px * 15.0f) + (py * 10.0f), 360.0f);
            CRGB color = hslToRgb(hue, 100.0f, 60.0f);
            
            uint8_t r = (uint8_t)(color.r * coverage);
            uint8_t g = (uint8_t)(color.g * coverage);
            uint8_t b = (uint8_t)(color.b * coverage);
            
            display.setPixel(panel_x_offset + px, py, RGB(r, g, b));
          }
        }
      }
    }
  }
}

extern "C" void app_main(){
  // VERY FIRST THING - print something
  vTaskDelay(pdMS_TO_TICKS(2000));
  printf("\n\n\n*** ESP32 BOOTED - APP STARTING ***\n\n\n");
  vTaskDelay(pdMS_TO_TICKS(100)); // Give time for serial to flush
  
  ESP_LOGI(TAG, "=== Line Drawing Demo ===");
  ESP_LOGI(TAG, "Drawing animated line patterns on dual displays");
  ESP_LOGI(TAG, "");
  
  /** Configure display with dual OE pins */
  HUB75Config config = HUB75Config::getDefault();
  config.enable_gamma_correction = true;  // Enable built-in gamma correction
  config.gamma_value = 2.2f;
  config.dual_display_mode = true;        // Enable dual display spillover
  config.effective_width = 128;           // 64x2 = 128 pixels wide
  
  /** Panel inversion: Flip panel 0 vertically (required for hardware orientation) */
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
  
  /** Calculate buffer size using driver helper method */
  int buffer_size = HUB75Driver::calculateBufferSize(config);
  
  ESP_LOGI(TAG, "Calculated buffer size: %d samples (%d KB)", 
           buffer_size, (buffer_size * 2) / 1024);
  
  /** Initialize I2S protocol with hardware dependencies */
  if(!i2sProtocol.init(config, buffer_size, &hardware, &bufferManager)){
    ESP_LOGE(TAG, "Failed to initialise I2S protocol");
    return;
  }
  
  /** Initialise display with I2S protocol */
  if(!display.init(config, &i2sProtocol)){
    ESP_LOGE(TAG, "Failed to initialise HUB75 display");
    return;
  }
  
  /** Start display */
  if(!display.start()){
    ESP_LOGE(TAG, "Failed to start display");
    return;
  }

  ESP_LOGI(TAG, "Display initialized: %dx%d pixels (2x 64x32 panels)",
           display.getWidth(), display.getHeight());
  ESP_LOGI(TAG, "Starting smooth subpixel swaying shape with RGB shader...");
  ESP_LOGI(TAG, "");
  
  /** Animation variables */
  float time = 0.0f;
  float hue_offset = 0.0f;
  
  /** Main loop: Draw smooth swaying shapes with RGB shader on both displays */
  while(true){
    // Calculate gentle sway using sine waves with FLOAT precision for smooth animation
    // This creates subpixel movement that gets rounded during rendering
    float offset_x = sinf(time * 1.0f) * 3.0f;  // Faster sway
    float offset_y = sinf(time * 1.4f) * 2.0f;  // Faster sway
    
    // Clear the display
    display.clear();
    
    // Draw red shape then apply RGB shader on both panels
    drawShapeWithRGBShader(0, offset_x, offset_y, hue_offset);  // Panel 0
    drawShapeWithRGBShader(1, offset_x, offset_y, hue_offset);  // Panel 1
    
    // Update the display
    display.show();
    
    // Update animation parameters with faster increments
    time += 0.08f;  // Faster time increment for quicker sway
    hue_offset += 8.0f;  // Much faster hue cycling speed
    if(hue_offset >= 360.0f){
      hue_offset -= 360.0f;
    }
    
    // Minimal delay for maximum speed (~60 FPS target)
    vTaskDelay(pdMS_TO_TICKS(16));
  }
}
