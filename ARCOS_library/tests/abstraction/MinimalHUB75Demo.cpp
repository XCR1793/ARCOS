/*****************************************************************
 * File:      MinimalHUB75Demo.cpp
 * Category:  tests/abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    The absolute minimal HUB75 example - just a few lines!
 *    Perfect starting point for new projects.
 *****************************************************************/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "abstraction/drivers/components/HUB75/driver_hub75_simple.hpp"

using namespace arcos::abstraction::drivers;

extern "C" void app_main(){
  // Wait for serial
  vTaskDelay(pdMS_TO_TICKS(1000));
  
  // Create and initialize display in one line each
  SimpleHUB75Display display;
  display.begin();  // Uses sensible defaults
  
  // Draw some pixels
  display.setPixel(10, 10, RGB(255, 0, 0));    // Red pixel
  display.setPixel(20, 10, RGB(0, 255, 0));    // Green pixel
  display.setPixel(30, 10, RGB(0, 0, 255));    // Blue pixel
  
  // Show on display
  display.show();
  
  // Animation loop
  int x = 0;
  while(true){
    display.clear();
    display.setPixel(x, 16, RGB(255, 255, 0));  // Yellow moving pixel
    display.show();
    
    x = (x + 1) % display.getWidth();
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
