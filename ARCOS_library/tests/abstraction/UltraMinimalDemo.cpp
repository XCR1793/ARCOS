/*****************************************************************
 * File:      UltraMinimalDemo.cpp
 * Category:  tests/abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    The ABSOLUTE MINIMAL HUB75 example possible.
 *    Only 15 lines of actual code!
 *****************************************************************/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "abstraction/drivers/components/HUB75/driver_hub75_simple.hpp"

using namespace arcos::abstraction::drivers;

extern "C" void app_main(){
  SimpleHUB75Display display;
  display.begin();
  
  while(true){
    display.fill(RGB(255, 0, 0));    // Red
    display.show();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    display.fill(RGB(0, 255, 0));    // Green
    display.show();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    display.fill(RGB(0, 0, 255));    // Blue
    display.show();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
