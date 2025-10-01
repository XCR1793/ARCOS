#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lcd_parallel.hpp"
#include "parallel_buffer.hpp"

static const char* TAG = "PIN_TEST";

extern "C" void app_main(){
  ESP_LOGI(TAG, "Individual Pin Test Starting");
  
  /** Configure which pin to test */
  int test_bit = 9;  // -1=ALL, 0-12=specific bit
  
  if(test_bit == -1){
    ESP_LOGI(TAG, "Testing ALL 13 data pins with staggered patterns");
  } else if(test_bit >= 0 && test_bit <= 12){
    const char* pin_names[13] = {
      "R0 (GPIO 7)", "G0 (GPIO 15)", "B0 (GPIO 16)", "R1 (GPIO 17)", 
      "G1 (GPIO 18)", "B1 (GPIO 8)", "LAT (GPIO 36)", "OE (GPIO 35)",
      "A (GPIO 41)", "B (GPIO 40)", "C (GPIO 39)", "D (GPIO 38)", "E (GPIO 42)"
    };
    ESP_LOGI(TAG, "Testing ONLY Bit %d: %s", test_bit, pin_names[test_bit]);
  } else {
    ESP_LOGE(TAG, "Invalid test_bit value: %d (must be -1 or 0-12)", test_bit);
    return;
  }

  /** Initialize hardware interfaces */
  LcdParallel lcdInterface;
  ParallelBuffer dmaBuffer;

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

  ESP_LOGI(TAG, "Configuration:");
  ESP_LOGI(TAG, "  Clock: GPIO 37 @ %d Hz", lcd_config.clock_freq_hz);
  ESP_LOGI(TAG, "  Test pattern: 5MHz square waves");

  /** Buffer configuration */
  const int buffer_size = 64;

  /** Allocate DMA buffer */
  if(!dmaBuffer.alloc(buffer_size)){
    ESP_LOGE(TAG, "Failed to allocate DMA buffer");
    return;
  }

  /** Initialize LCD interface */
  if(!lcdInterface.init(lcd_data_pins, lcd_config)){
    ESP_LOGE(TAG, "Failed to initialize LCD interface");
    return;
  }

  /** Generate test pattern */
  uint16_t* buffer = dmaBuffer.getBuffer();
  
  if(test_bit == -1){
    ESP_LOGI(TAG, "Generating comprehensive test pattern for all 13 pins");
    ESP_LOGI(TAG, "Each pin will alternate HIGH/LOW every sample (5MHz square waves)");
    
    /** Pin mapping reference */
    const char* pin_names[13] = {
      "R0 (GPIO 7)",   "G0 (GPIO 15)",  "B0 (GPIO 16)",  "R1 (GPIO 17)", 
      "G1 (GPIO 18)",  "B1 (GPIO 8)",   "LAT (GPIO 36)", "OE (GPIO 35)",
      "A (GPIO 41)",   "B (GPIO 40)",   "C (GPIO 39)",   "D (GPIO 38)", 
      "E (GPIO 42)"
    };
    
    ESP_LOGI(TAG, "Pin mapping:");
    for(int pin = 0; pin < 13; pin++){
      ESP_LOGI(TAG, "  Bit %2d: %s", pin, pin_names[pin]);
    }
    
    /** Generate staggered pattern for all pins */
    for(int i = 0; i < buffer_size; i++){
      uint16_t sample = 0;
      
      /** Create 5MHz square waves with staggered phases */
      for(int bit = 0; bit < 13; bit++){
        if((i + bit) % 2 == 0){
          sample |= (1 << bit);
        }
      }
      
      buffer[i] = sample;
      
      /** Log first few samples for debugging */
      if(i < 16){
        ESP_LOGI(TAG, "Sample[%2d] = 0x%04X", i, sample);
        if(i < 8){
          char bit_status[64] = "";
          for(int bit = 0; bit < 13; bit++){
            if(sample & (1 << bit)){
              char temp[8];
              snprintf(temp, sizeof(temp), "%d ", bit);
              strcat(bit_status, temp);
            }
          }
          ESP_LOGI(TAG, "  Active bits: %s", bit_status);
        }
      }
    }
  } else {
    ESP_LOGI(TAG, "Generating single pin test pattern for bit %d", test_bit);
    ESP_LOGI(TAG, "Only the selected pin will alternate HIGH/LOW at 5MHz");
    
    /** Generate single pin pattern */
    for(int i = 0; i < buffer_size; i++){
      uint16_t sample = 0;
      
      if(i % 2 == 0){
        sample |= (1 << test_bit);
      }
      
      buffer[i] = sample;
      
      /** Log first few samples for debugging */
      if(i < 8){
        ESP_LOGI(TAG, "Sample[%d] = 0x%04X (bit %d = %s)", 
                 i, sample, test_bit, (sample & (1 << test_bit)) ? "HIGH" : "LOW");
      }
    }
  }

  ESP_LOGI(TAG, "Pattern: 5MHz square waves at %dMHz base clock", lcd_config.clock_freq_hz / 1000000);

  /** Start continuous transmission */
  if(!lcdInterface.setDirectBuffer(buffer, buffer_size)){
    ESP_LOGE(TAG, "Failed to set buffer");
    return;
  }

  if(!lcdInterface.start()){
    ESP_LOGE(TAG, "Failed to start transmission");
    return;
  }

  ESP_LOGI(TAG, "Continuous transmission started");
  
  if(test_bit == -1){
    ESP_LOGI(TAG, "Monitor pins with oscilloscope/logic analyzer:");
    ESP_LOGI(TAG, "DATA PINS (staggered 5MHz square waves):");
    ESP_LOGI(TAG, "  Bit 0:  GPIO 7  (R0)");
    ESP_LOGI(TAG, "  Bit 1:  GPIO 15 (G0)");
    ESP_LOGI(TAG, "  Bit 2:  GPIO 16 (B0)");
    ESP_LOGI(TAG, "  Bit 3:  GPIO 17 (R1)");
    ESP_LOGI(TAG, "  Bit 4:  GPIO 18 (G1)");
    ESP_LOGI(TAG, "  Bit 5:  GPIO 8  (B1)");
    ESP_LOGI(TAG, "  Bit 6:  GPIO 36 (LAT)");
    ESP_LOGI(TAG, "  Bit 7:  GPIO 35 (OE)");
    ESP_LOGI(TAG, "  Bit 8:  GPIO 41 (A)");
    ESP_LOGI(TAG, "  Bit 9:  GPIO 40 (B)");
    ESP_LOGI(TAG, "  Bit 10: GPIO 39 (C)");
    ESP_LOGI(TAG, "  Bit 11: GPIO 38 (D)");
    ESP_LOGI(TAG, "  Bit 12: GPIO 42 (E)");
    ESP_LOGI(TAG, "Each pin has unique phase offset for identification");
  } else {
    const char* pin_names[13] = {
      "R0 (GPIO 7)", "G0 (GPIO 15)", "B0 (GPIO 16)", "R1 (GPIO 17)", 
      "G1 (GPIO 18)", "B1 (GPIO 8)", "LAT (GPIO 36)", "OE (GPIO 35)",
      "A (GPIO 41)", "B (GPIO 40)", "C (GPIO 39)", "D (GPIO 38)", "E (GPIO 42)"
    };
    ESP_LOGI(TAG, "Monitor target pin:");
    ESP_LOGI(TAG, "  TARGET: Bit %d - %s (5MHz square wave)", test_bit, pin_names[test_bit]);
    ESP_LOGI(TAG, "  All other pins should be LOW");
  }
  
  ESP_LOGI(TAG, "CLOCK PIN:");
  ESP_LOGI(TAG, "  GPIO 37 (CLK) - 10MHz");
  ESP_LOGI(TAG, "Diagnostics:");
  ESP_LOGI(TAG, "  No data pins toggle: LCD/DMA setup issue");
  ESP_LOGI(TAG, "  Selected pin no toggle: bit mapping issue");

  /** Main monitoring loop */
  while(true){
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    if(test_bit == -1){
      ESP_LOGI(TAG, "Running - All 13 pins show staggered 5MHz patterns");
      ESP_LOGI(TAG, "Check each GPIO to identify working pins");
    } else {
      const char* pin_names[13] = {
        "R0 (GPIO 7)", "G0 (GPIO 15)", "B0 (GPIO 16)", "R1 (GPIO 17)", 
        "G1 (GPIO 18)", "B1 (GPIO 8)", "LAT (GPIO 36)", "OE (GPIO 35)",
        "A (GPIO 41)", "B (GPIO 40)", "C (GPIO 39)", "D (GPIO 38)", "E (GPIO 42)"
      };
      ESP_LOGI(TAG, "Running - Bit %d (%s) shows 5MHz square wave", test_bit, pin_names[test_bit]);
    }
  }
}