/*****************************************************************
 * File:      main_oled_test.cpp
 * Category:  test
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Test application for DRIVER_OLED_SH1107 HAL-based driver.
 *    Demonstrates various display features including text, graphics,
 *    and animations using only ARCOS abstractions.
 *****************************************************************/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "abstraction/hal.hpp"
#include "abstraction/drivers/components/OLED/driver_oled_sh1107.hpp"

static const char* TAG = "OLED_TEST";

// Use explicit namespace qualification instead of 'using namespace'
// to avoid conflicts with C++ standard library
// using namespace arcos::abstraction;

/** Global OLED driver instance */
static arcos::abstraction::DRIVER_OLED_SH1107 oled(0x3C, 0);  // Address 0x3C, Bus 0

/** Test pattern enumeration */
enum TestPattern{
  TEST_TEXT,
  TEST_GRAPHICS,
  TEST_ANIMATION,
  TEST_BOUNCING_BALL,
  TEST_SCROLLING_TEXT,
  TEST_PATTERNS,
  TEST_COUNT
};

static int current_test = 0;

/** Helper function to draw a border */
void drawBorder(){
  oled.drawRect(0, 0, 128, 128, false, true);
}

/** Test 1: Text rendering */
void testText(){
  oled.clearBuffer();
  drawBorder();
  
  oled.drawString(10, 10, "ARCOS v1.0", true);
  oled.drawString(10, 25, "OLED Driver", true);
  oled.drawString(10, 40, "SH1107 128x128", true);
  oled.drawString(10, 55, "HAL Abstraction", true);
  
  // Draw some info
  char buf[32];
  snprintf(buf, sizeof(buf), "Time: %lu ms", (unsigned long)(esp_timer_get_time() / 1000));
  oled.drawString(5, 100, buf, true);
  
  oled.drawString(20, 115, "Press to continue", true);
  
  oled.updateDisplay();
}

/** Test 2: Graphics primitives */
void testGraphics(){
  oled.clearBuffer();
  drawBorder();
  
  oled.drawString(20, 5, "Graphics Test", true);
  
  // Draw some shapes
  oled.drawLine(10, 20, 50, 60, true);
  oled.drawLine(50, 60, 90, 20, true);
  oled.drawLine(90, 20, 10, 20, true);
  
  oled.drawRect(15, 70, 30, 20, false, true);
  oled.drawRect(55, 70, 30, 20, true, true);
  oled.drawRect(95, 70, 20, 30, false, true);
  
  oled.drawCircle(30, 110, 10, false, true);
  oled.drawCircle(64, 110, 10, true, true);
  oled.drawCircle(98, 110, 8, false, true);
  
  oled.updateDisplay();
}

/** Test 3: Simple animation */
void testAnimation(){
  static int frame = 0;
  
  oled.clearBuffer();
  drawBorder();
  
  oled.drawString(30, 5, "Animation", true);
  
  // Rotating line
  int center_x = 64;
  int center_y = 64;
  int radius = 50;
  
  float angle = (frame * 3.14159f * 2.0f) / 60.0f;
  int end_x = center_x + (int)(radius * cos(angle));
  int end_y = center_y + (int)(radius * sin(angle));
  
  oled.drawLine(center_x, center_y, end_x, end_y, true);
  oled.drawCircle(center_x, center_y, 3, true, true);
  
  // Progress bar
  int progress = (frame * 100) / 60;
  oled.drawRect(10, 115, 108, 8, false, true);
  oled.drawRect(12, 117, progress, 4, true, true);
  
  char buf[16];
  snprintf(buf, sizeof(buf), "%d%%", progress);
  oled.drawString(50, 105, buf, true);
  
  oled.updateDisplay();
  
  frame++;
  if(frame >= 60) frame = 0;
}

/** Test 4: Bouncing ball */
void testBouncingBall(){
  static float ball_x = 64.0f;
  static float ball_y = 64.0f;
  static float vel_x = 2.5f;
  static float vel_y = 1.8f;
  static int ball_radius = 8;
  
  oled.clearBuffer();
  drawBorder();
  
  oled.drawString(25, 5, "Bouncing Ball", true);
  
  // Update ball position
  ball_x += vel_x;
  ball_y += vel_y;
  
  // Bounce off walls
  if(ball_x - ball_radius < 2 || ball_x + ball_radius > 126){
    vel_x = -vel_x;
    ball_x += vel_x * 2;
  }
  if(ball_y - ball_radius < 15 || ball_y + ball_radius > 126){
    vel_y = -vel_y;
    ball_y += vel_y * 2;
  }
  
  // Draw ball with motion trail
  oled.drawCircle((int)ball_x - vel_x, (int)ball_y - vel_y, ball_radius - 2, false, true);
  oled.drawCircle((int)ball_x, (int)ball_y, ball_radius, true, true);
  
  oled.updateDisplay();
}

/** Test 5: Scrolling text */
void testScrollingText(){
  static int scroll_offset = 128;
  static const char* message = "*** ARCOS OLED Driver - HAL Abstraction Layer - SH1107 Controller - 128x128 Display ***";
  
  oled.clearBuffer();
  drawBorder();
  
  oled.drawString(15, 5, "Scrolling Text", true);
  
  // Calculate text width
  int text_width;
  oled.getTextSize(message, &text_width, nullptr);
  
  // Draw scrolling text
  oled.drawString(scroll_offset, 60, message, true);
  
  scroll_offset -= 2;
  if(scroll_offset < -text_width){
    scroll_offset = 128;
  }
  
  oled.updateDisplay();
}

/** Test 6: Pattern tests */
void testPatterns(){
  static int pattern_frame = 0;
  static int pattern_type = 0;
  
  oled.clearBuffer();
  
  switch(pattern_type){
    case 0: // Checkerboard
      for(int y = 0; y < 128; y += 8){
        for(int x = 0; x < 128; x += 8){
          if(((x / 8) + (y / 8)) % 2 == 0){
            oled.drawRect(x, y, 8, 8, true, true);
          }
        }
      }
      oled.drawString(25, 60, "Checkerboard", true);
      break;
      
    case 1: // Horizontal lines
      for(int y = 0; y < 128; y += 4){
        oled.drawLine(0, y, 127, y, true);
      }
      oled.drawString(10, 60, "Horizontal Lines", true);
      break;
      
    case 2: // Vertical lines
      for(int x = 0; x < 128; x += 4){
        oled.drawLine(x, 0, x, 127, true);
      }
      oled.drawString(15, 60, "Vertical Lines", true);
      break;
      
    case 3: // Diagonal lines
      for(int i = 0; i < 128; i += 8){
        oled.drawLine(0, i, 127 - i, 127, true);
        oled.drawLine(i, 0, 127, 127 - i, true);
      }
      oled.drawString(20, 60, "Diagonal Lines", true);
      break;
      
    case 4: // Concentric circles
      for(int r = 10; r < 64; r += 10){
        oled.drawCircle(64, 64, r, false, true);
      }
      oled.drawString(10, 110, "Concentric Circles", true);
      break;
  }
  
  oled.updateDisplay();
  
  pattern_frame++;
  if(pattern_frame >= 60){
    pattern_frame = 0;
    pattern_type = (pattern_type + 1) % 5;
  }
}

/** Main test loop */
void runTests(){
  static int test_duration = 0;
  const int TEST_FRAMES = 120; // 2 seconds per test at 60fps
  
  switch(current_test){
    case TEST_TEXT:
      testText();
      break;
    case TEST_GRAPHICS:
      testGraphics();
      break;
    case TEST_ANIMATION:
      testAnimation();
      break;
    case TEST_BOUNCING_BALL:
      testBouncingBall();
      break;
    case TEST_SCROLLING_TEXT:
      testScrollingText();
      break;
    case TEST_PATTERNS:
      testPatterns();
      break;
  }
  
  test_duration++;
  if(test_duration >= TEST_FRAMES){
    test_duration = 0;
    current_test = (current_test + 1) % TEST_COUNT;
    ESP_LOGI(TAG, "Switching to test: %d", current_test);
  }
}

/** Performance monitoring */
void displayPerformanceStats(){
  static uint32_t frame_count = 0;
  static int64_t last_time = 0;
  
  frame_count++;
  
  int64_t now = esp_timer_get_time();
  if(now - last_time >= 5000000){ // Every 5 seconds
    float fps = frame_count * 1000000.0f / (now - last_time);
    ESP_LOGI(TAG, "Performance: %.2f FPS, %lu frames", fps, (unsigned long)frame_count);
    
    frame_count = 0;
    last_time = now;
  }
}

/** Test task */
void oledTestTask(void* pvParameters){
  ESP_LOGI(TAG, "OLED test task started");
  
  const TickType_t frame_delay = pdMS_TO_TICKS(16); // ~60 FPS
  
  while(true){
    runTests();
    displayPerformanceStats();
    vTaskDelay(frame_delay);
  }
}

/** Smart update test - demonstrates efficient partial updates */
void testSmartUpdate(){
  ESP_LOGI(TAG, "Testing smart update feature...");
  
  oled.clearBuffer();
  oled.drawString(10, 10, "Smart Update Test", true);
  oled.updateDisplay();
  vTaskDelay(pdMS_TO_TICKS(1000));
  
  // Mark everything clean
  oled.updateSmart();
  
  // Now update only a small area
  int64_t start = esp_timer_get_time();
  oled.drawString(10, 60, "Partial update!", true);
  oled.updateSmart(); // Only sends changed pages
  int64_t elapsed = esp_timer_get_time() - start;
  
  ESP_LOGI(TAG, "Smart update took: %lld us", elapsed);
  ESP_LOGI(TAG, "Dirty pages: %d/16", oled.getDirtyPageCount());
  
  vTaskDelay(pdMS_TO_TICKS(2000));
}

/** Orientation test */
void testOrientation(){
  ESP_LOGI(TAG, "Testing orientation changes...");
  
  const char* orientations[] = {"Normal", "Flip H", "Flip V", "Flip Both"};
  
  for(int i = 0; i < 4; i++){
    oled.clearBuffer();
    
    // Apply different flip combinations
    switch(i){
      case 0: // Normal
        oled.setFlipHorizontal(false);
        oled.setFlipVertical(false);
        break;
      case 1: // Horizontal flip only
        oled.setFlipHorizontal(true);
        oled.setFlipVertical(false);
        break;
      case 2: // Vertical flip only
        oled.setFlipHorizontal(false);
        oled.setFlipVertical(true);
        break;
      case 3: // Both (same as upside down)
        oled.setFlipHorizontal(true);
        oled.setFlipVertical(true);
        break;
    }
    
    oled.drawString(20, 40, orientations[i], true);
    oled.drawString(25, 55, "Orientation", true);
    
    // Draw arrow pointing up to show orientation
    oled.drawLine(64, 80, 64, 100, true);
    oled.drawLine(64, 80, 54, 90, true);
    oled.drawLine(64, 80, 74, 90, true);
    
    // Draw corner markers to show flip direction
    oled.drawRect(5, 5, 10, 10, true, true);      // Top-left marker
    oled.drawString(8, 8, "TL", false);
    
    oled.updateDisplay();
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
  
  // Reset to normal
  oled.setFlipHorizontal(false);
  oled.setFlipVertical(false);
}

extern "C" void app_main(void){
  ESP_LOGI(TAG, "==================================");
  ESP_LOGI(TAG, "ARCOS OLED Driver Test");
  ESP_LOGI(TAG, "Driver: DRIVER_OLED_SH1107");
  ESP_LOGI(TAG, "Display: 128x128 White OLED");
  ESP_LOGI(TAG, "Controller: SH1107G");
  ESP_LOGI(TAG, "==================================");
  
  // Initialize I2C bus
  // Pins: SDA=GPIO38, SCL=GPIO39 (matches HUB75_and_OLED configuration)
  ESP_LOGI(TAG, "Initializing I2C bus...");
  arcos::abstraction::HalResult result = arcos::abstraction::ESP32S3_I2C::Initialize(0, 1, 2, 400000);
  if(result != arcos::abstraction::HalResult::Success){
    ESP_LOGE(TAG, "Failed to initialize I2C bus");
    ESP_LOGE(TAG, "Check OLED wiring: SCL->GPIO39, SDA->GPIO38");
    return;
  }
  ESP_LOGI(TAG, "I2C bus initialized successfully (SDA=GPIO38, SCL=GPIO39)");
  
  // Initialize OLED display
  ESP_LOGI(TAG, "Initializing OLED display...");
  arcos::abstraction::OLEDConfig config;
  config.contrast = 0x80;           // Medium contrast
  config.flip_horizontal = false;   // No horizontal flip
  config.flip_vertical = true;      // Flip vertically
  
  if(!oled.initialize(config)){
    ESP_LOGE(TAG, "Failed to initialize OLED display");
    return;
  }
  ESP_LOGI(TAG, "OLED display initialized successfully");
  
  // Show startup screen
  oled.clearBuffer();
  oled.drawRect(0, 0, 128, 128, false, true);
  oled.drawString(35, 40, "ARCOS", true);
  oled.drawString(15, 55, "OLED Driver", true);
  oled.drawString(30, 75, "Starting...", true);
  oled.updateDisplay();
  vTaskDelay(pdMS_TO_TICKS(2000));
  
  // Run orientation test
  testOrientation();
  
  // Run smart update test
  testSmartUpdate();
  
  // Show main menu
  oled.clearBuffer();
  oled.drawRect(0, 0, 128, 128, false, true);
  oled.drawString(20, 20, "Test Menu", true);
  oled.drawString(10, 40, "1. Text", true);
  oled.drawString(10, 55, "2. Graphics", true);
  oled.drawString(10, 70, "3. Animation", true);
  oled.drawString(10, 85, "4. Bouncing Ball", true);
  oled.drawString(10, 100, "5. Scrolling", true);
  oled.updateDisplay();
  vTaskDelay(pdMS_TO_TICKS(2000));
  
  // Start test task
  ESP_LOGI(TAG, "Starting continuous test task...");
  xTaskCreatePinnedToCore(
    oledTestTask,
    "oled_test",
    4096,
    nullptr,
    5,
    nullptr,
    1  // Run on core 1
  );
  
  ESP_LOGI(TAG, "OLED test running. Tests will cycle automatically.");
}
