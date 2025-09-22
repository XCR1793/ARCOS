#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2c_driver.h"
#include "oled_driver.h"

static const char *TAG = "MAIN";

/**
 * @brief Simple demo showing direct buffer manipulation
 */
void demo_direct_buffer(void) {
    ESP_LOGI(TAG, "Direct buffer demo");
    
    // Get direct access to buffer and write a pattern
    uint8_t* buffer = oled_get_buffer();
    
    // Write checkerboard pattern directly to buffer
    for (int page = 0; page < 16; page++) {
        for (int col = 0; col < 128; col++) {
            buffer[page * 128 + col] = ((page + col/8) % 2) ? 0xFF : 0x00;
        }
    }
    
    oled_update_display();
    vTaskDelay(pdMS_TO_TICKS(3000));
}

/**
 * @brief Demo showing selective area updates
 */
void demo_area_updates(void) {
    ESP_LOGI(TAG, "Area update demo");
    
    oled_clear_buffer();
    oled_draw_string(20, 10, "Area Updates", true);
    oled_update_display();
    
    // Update different areas independently
    for (int i = 0; i < 4; i++) {
        int x = (i % 2) * 64;
        int y = (i / 2) * 64 + 30;
        
        oled_draw_rect(x + 5, y, 50, 30, true, true);
        
        oled_rect_t area = oled_rect(x + 5, y, 50, 30);
        oled_update_area(&area);
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    vTaskDelay(pdMS_TO_TICKS(2000));
}

/**
 * @brief Demo showing smart updates (only changed pages)
 */
void demo_smart_updates(void) {
    ESP_LOGI(TAG, "Smart update demo");
    
    oled_clear_buffer();
    oled_draw_string(15, 10, "Smart Updates", true);
    oled_update_display();
    
    // Make small changes and use smart update
    for (int i = 0; i < 10; i++) {
        int x = 10 + i * 10;
        int y = 50;
        
        oled_set_pixel(x, y, true);
        oled_set_pixel(x, y + 1, true);
        
        // Only updates changed pages
        oled_update_smart();
        
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    
    vTaskDelay(pdMS_TO_TICKS(2000));
}

/**
 * @brief Main application entry point
 */
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Starting OLED modular driver demo");
    
    // Initialize I2C driver
    ESP_ERROR_CHECK(i2c_driver_init());
    ESP_LOGI(TAG, "I2C initialized");
    
    // Scan for devices
    i2c_scan_devices();
    
    // Try to initialize OLED (don't crash if it fails)
    esp_err_t ret = oled_init();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OLED initialized successfully!");
    } else {
        ESP_LOGE(TAG, "OLED initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Check wiring: SCL->GPIO39, SDA->GPIO38, VCC->3.3V, GND->GND");
        return;
    }
    
    // Run demos in loop
    while (1) {
        ESP_LOGI(TAG, "=== Demo Loop ===");
        
        // Test 1: Direct buffer access
        demo_direct_buffer();
        
        // Test 2: Area-specific updates  
        demo_area_updates();
        
        // Test 3: Smart updates
        demo_smart_updates();
        
        ESP_LOGI(TAG, "Demo complete, restarting in 2 seconds...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}