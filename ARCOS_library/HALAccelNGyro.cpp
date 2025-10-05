/*****************************************************************
 * File:      main.cpp (Clean Sensor Demo)
 * Category:  application
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Clean sensor demonstration using ARCOS driver architecture.
 *    Displays all sensor data in a single line every second.
 *****************************************************************/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

// ARCOS HAL and drivers
#include "abstraction/hal.hpp"  // Includes ESP32S3_I2C and HAL_TIMER_DEFAULT
#include "abstraction/drivers/components/BME280/driver_bme280.hpp"
#include "abstraction/drivers/components/ICM20948/driver_icm20948.hpp"

using namespace arcos::abstraction;

static const char* TAG = "SENSOR_DEMO";

extern "C" void app_main(void){
    ESP_LOGI(TAG, "Starting ARCOS Sensor Demo");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Step 1: Initialize I2C HAL with custom pins (SDA=9, SCL=10, 400kHz)
    // The HAL handles all low-level I2C operations
    auto hal_result = ESP32S3_I2C::Initialize(0, 9, 10, 400000);
    if(hal_result != HalResult::Success){
        ESP_LOGE(TAG, "Failed to initialize I2C HAL");
        return;
    }
    ESP_LOGI(TAG, "I2C HAL initialized successfully");

    // Step 2: Create sensor driver instances
    // Simply specify device address and bus ID - drivers handle everything else!
    DRIVER_BME280 bme280(0x76, 0);      // BME280 environmental sensor at address 0x76, bus 0
    DRIVER_ICM20948 icm20948(0x68, 0);  // ICM20948 9-axis IMU at address 0x68, bus 0
    
    // Step 3: Initialize sensors - they auto-configure with sensible defaults
    // All the heavy lifting (calibration, register config, etc.) happens internally!
    bool bme_ok = bme280.initialize();
    bool icm_ok = icm20948.initialize();
    
    // Step 4: Check initialization results
    if(bme_ok){
        ESP_LOGI(TAG, "✅ BME280 Environmental Sensor Ready");
    }else{
        ESP_LOGE(TAG, "❌ BME280 initialization failed");
    }
    
    if(icm_ok){
        ESP_LOGI(TAG, "✅ ICM20948 IMU Ready");
        if(icm20948.isMagnetometerInitialized()){
            ESP_LOGI(TAG, "✅ Magnetometer Ready");
        }else{
            ESP_LOGW(TAG, "⚠️  Magnetometer not available");
        }
    }else{
        ESP_LOGE(TAG, "❌ ICM20948 initialization failed");
    }

    if(!bme_ok && !icm_ok){
        ESP_LOGE(TAG, "No sensors available, exiting");
        return;
    }

    ESP_LOGI(TAG, "Starting sensor data stream...");
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Step 5: Main sensor reading loop - just call readData()!
    // The drivers handle all register reads, data parsing, and calibration
    while(true){
        BME280Data env_data;
        ICM20948Data imu_data;
        
        // Simple API: just read the data structures
        bool env_success = bme_ok && bme280.readData(env_data);
        bool imu_success = icm_ok && icm20948.readData(imu_data);
        
        // Default values for failed readings
        if(!env_success){
            env_data.temperature = 0.0f;
            env_data.humidity = 0.0f;
            env_data.pressure = 0.0f;
        }
        
        if(!imu_success){
            imu_data.accel_x = imu_data.accel_y = imu_data.accel_z = 0.0f;
            imu_data.gyro_x = imu_data.gyro_y = imu_data.gyro_z = 0.0f;
            imu_data.mag_x = imu_data.mag_y = imu_data.mag_z = 0.0f;
        }
        
        // Display all sensor data in one clean line
        printf("Accel: %.2f,%.2f,%.2f | Gyro: %.2f,%.2f,%.2f | Mag: %.1f,%.1f,%.1f | Temp: %.1f°C | Hum: %.1f%% | Press: %.0fPa\n",
               imu_data.accel_x, imu_data.accel_y, imu_data.accel_z,
               imu_data.gyro_x, imu_data.gyro_y, imu_data.gyro_z,
               imu_data.mag_x, imu_data.mag_y, imu_data.mag_z,
               env_data.temperature, env_data.humidity, env_data.pressure);
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second interval
    }
}

/*****************************************************************
 * ADVANCED USAGE EXAMPLES
 * 
 * The drivers support optional configuration for power users who
 * need custom sensor settings. Here are some examples:
 * 
 * EXAMPLE 1: High-precision environmental sensing
 * -----------------------------------------------
 * BME280Config high_precision;
 * high_precision.temp_oversampling = 5;   // 16x oversampling
 * high_precision.press_oversampling = 5;  // 16x oversampling
 * high_precision.hum_oversampling = 5;    // 16x oversampling
 * high_precision.mode = 3;                // Normal mode (continuous)
 * 
 * DRIVER_BME280 bme280(0x76, 0);
 * bme280.initialize(high_precision);
 * 
 * 
 * EXAMPLE 2: High-G accelerometer for impact detection
 * ----------------------------------------------------
 * ICM20948Config high_g;
 * high_g.accel_range = 3;           // ±16g range
 * high_g.gyro_range = 3;            // ±2000 dps range
 * high_g.enable_magnetometer = false;  // Disable mag for faster sampling
 * 
 * DRIVER_ICM20948 icm20948(0x68, 0);
 * icm20948.initialize(high_g);
 * 
 * 
 * EXAMPLE 3: Low-power environmental monitoring
 * ---------------------------------------------
 * BME280Config low_power;
 * low_power.temp_oversampling = 1;   // 1x oversampling (faster)
 * low_power.press_oversampling = 1;
 * low_power.hum_oversampling = 1;
 * low_power.mode = 1;                // Forced mode (one-shot)
 * 
 * DRIVER_BME280 bme280(0x76, 0);
 * bme280.initialize(low_power);
 * 
 * // In forced mode, trigger measurement manually:
 * // ESP32S3_I2C::WriteRegister(0, 0x76, 0xF4, 0x25);
 * 
 * 
 * KEY BENEFITS OF THIS DESIGN:
 * ----------------------------
 * ✅ No drivers.hpp needed - each driver is standalone
 * ✅ Simple default initialization - just call initialize()
 * ✅ Advanced configuration available when needed
 * ✅ Clean API - all heavy lifting hidden in private methods
 * ✅ Direct HAL usage - initialize I2C once, use anywhere
 * ✅ Type-safe configuration structures
 * 
 *****************************************************************/