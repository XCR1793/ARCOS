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

// ARCOS drivers
#include "abstraction/hal.hpp"
#include "abstraction/drivers/components/BME280/driver_bme280.hpp"
#include "abstraction/drivers/components/ICM20948/driver_icm20948.hpp"

using namespace arcos::abstraction;

static const char* TAG = "SENSOR_DEMO";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting ARCOS Sensor Demo");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize I2C HAL with default settings (SDA=9, SCL=10, 400kHz)
    auto hal_result = HAL_I2C_DEFAULT::Initialize(0, 9, 10, 400000);
    if(hal_result != HalResult::Success){
        ESP_LOGE(TAG, "Failed to initialize I2C HAL");
        return;
    }

    // Initialize sensors with bus ID 0
    DRIVER_BME280 bme280(0x76, 0);  // BME280 at 0x76, bus 0
    DRIVER_ICM20948 icm20948(0x68, 0);  // ICM20948 at 0x68, bus 0
    
    bool bme_ok = bme280.initialize();
    bool icm_ok = icm20948.initialize();
    
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

    // Main sensor reading loop
    while(true){
        BME280Data env_data;
        ICM20948Data imu_data;
        
        // Read sensor data
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