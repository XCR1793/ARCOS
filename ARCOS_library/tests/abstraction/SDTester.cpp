/*****************************************************************
 * File:      main.cpp
 * Category:  src
 * Author:    ARCOS Team
 * 
 * Purpose:
 *    SD card file browser with console output. Displays card
 *    metadata and lists all files in the root directory.
 * 
 * Hardware Configuration:
 *    SD Card SPI Pins:
 *    - MISO: GPIO 14
 *    - MOSI: GPIO 47
 *    - SCK:  GPIO 21
 *    - CS:   GPIO 48
 *****************************************************************/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "abstraction/drivers/components/SD_CARD/driver_sd_card.hpp"

static const char* TAG = "SD_BROWSER";

/** Global driver instance */
static arcos::abstraction::DRIVER_SD_CARD sd_card;

/** Browser state */
static arcos::abstraction::FileInfo files[50];
static size_t file_count = 0;

/**
 * @brief Format file size to human-readable string
 */
void formatFileSize(uint64_t size, char* buffer, size_t buffer_size){
  if(size < 1024){
    snprintf(buffer, buffer_size, "%llu B", size);
  }else if(size < 1024 * 1024){
    snprintf(buffer, buffer_size, "%.1f KB", size / 1024.0);
  }else{
    snprintf(buffer, buffer_size, "%.1f MB", size / (1024.0 * 1024.0));
  }
}

/**
 * @brief Load file list from SD card
 */
void loadFileList(const char* directory = ""){
  file_count = 0;
  
  arcos::abstraction::SdCardResult result = sd_card.listDirectory(
    directory, 
    files, 
    50, 
    &file_count
  );
  
  if(result != arcos::abstraction::SdCardResult::Success){
    ESP_LOGE(TAG, "Failed to list directory");
    file_count = 0;
  }
  
  ESP_LOGI(TAG, "Loaded %zu files from directory '%s'", file_count, directory);
}

/**
 * @brief Print SD card metadata
 */
void printMetadata(){
  printf("\n========================================\n");
  printf("       SD CARD INFORMATION\n");
  printf("========================================\n");
  
  arcos::abstraction::SdCardInfo info;
  if(sd_card.getCardInfo(&info) == arcos::abstraction::SdCardResult::Success){
    const char* type_str = (info.type == 1) ? "SDSC" : (info.type == 2) ? "SDHC/XC" : "Unknown";
    
    printf("Card Name:      %s\n", info.name);
    printf("Card Type:      %s\n", type_str);
    printf("Total Capacity: %llu MB\n", info.capacity_mb);
    printf("Free Space:     %llu MB\n", info.free_mb);
    printf("Used Space:     %llu MB\n", info.capacity_mb - info.free_mb);
    
    // Calculate and display usage percentage
    uint64_t free_bytes = 0;
    uint64_t total_bytes = 0;
    if(sd_card.getFreeSpace(&free_bytes) == arcos::abstraction::SdCardResult::Success &&
       sd_card.getTotalSpace(&total_bytes) == arcos::abstraction::SdCardResult::Success){
      uint64_t free_mb = free_bytes / (1024 * 1024);
      uint64_t total_mb = total_bytes / (1024 * 1024);
      uint64_t used_mb = total_mb - free_mb;
      
      int usage_pct = total_mb > 0 ? (int)((used_mb * 100) / total_mb) : 0;
      printf("Usage:          %d%%\n", usage_pct);
    }
    
    printf("File Count:     %zu\n", file_count);
  }else{
    printf("Card info unavailable\n");
  }
  
  printf("========================================\n\n");
}

/**
 * @brief Print file list
 */
void printFileList(){
  printf("========================================\n");
  printf("       ROOT DIRECTORY LISTING\n");
  printf("========================================\n");
  
  if(file_count == 0){
    printf("(Empty directory)\n");
  }else{
    printf("Found %zu items:\n\n", file_count);
    
    for(size_t i = 0; i < file_count; i++){
      if(files[i].is_directory){
        printf("  [DIR]  %-40s\n", files[i].name);
      }else{
        char size_str[32];
        formatFileSize(files[i].size, size_str, sizeof(size_str));
        printf("  [FILE] %-40s %12s\n", files[i].name, size_str);
      }
    }
  }
  
  printf("========================================\n\n");
}

/**
 * @brief Application entry point
 */
extern "C" void app_main(){
  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "    ARCOS SD Card File Browser");
  ESP_LOGI(TAG, "    (Enhanced Driver with Validation)");
  ESP_LOGI(TAG, "========================================");
  
  // Wait 3 seconds before starting
  ESP_LOGI(TAG, "Waiting 3 seconds before initialization...");
  vTaskDelay(pdMS_TO_TICKS(3000));
  
  // Configure SD card
  ESP_LOGI(TAG, "Configuring SD card...");
  arcos::abstraction::SdCardConfig sd_config;
  sd_config.spi_host = 1;              // SPI2_HOST
  sd_config.pin_mosi = 47;             // GPIO 47
  sd_config.pin_miso = 14;             // GPIO 14
  sd_config.pin_sck = 21;              // GPIO 21
  sd_config.pin_cs = 48;               // GPIO 48
  sd_config.max_frequency_hz = 20000000; // 20 MHz
  sd_config.max_open_files = 5;
  sd_config.format_if_failed = false;
  sd_config.mount_point = "/sdcard";
  
  ESP_LOGI(TAG, "SD Card SPI Configuration:");
  ESP_LOGI(TAG, "  MOSI: GPIO %d", sd_config.pin_mosi);
  ESP_LOGI(TAG, "  MISO: GPIO %d", sd_config.pin_miso);
  ESP_LOGI(TAG, "  SCK:  GPIO %d", sd_config.pin_sck);
  ESP_LOGI(TAG, "  CS:   GPIO %d", sd_config.pin_cs);
  ESP_LOGI(TAG, "  Freq: %lu Hz", (unsigned long)sd_config.max_frequency_hz);
  
  // Validate configuration (optional - initialize() does this automatically)
  if(!arcos::abstraction::DRIVER_SD_CARD::validateConfig(sd_config)){
    ESP_LOGE(TAG, "Configuration validation failed!");
    while(true){ vTaskDelay(pdMS_TO_TICKS(1000)); }
    return;
  }
  ESP_LOGI(TAG, "Configuration validated successfully");
  
  // Initialize SD card (with automatic validation)
  ESP_LOGI(TAG, "Initializing SD card...");
  arcos::abstraction::SdCardResult result = sd_card.initialize(sd_config);
  
  if(result != arcos::abstraction::SdCardResult::Success){
    ESP_LOGE(TAG, "========================================");
    ESP_LOGE(TAG, "  SD CARD INITIALIZATION FAILED!");
    ESP_LOGE(TAG, "========================================");
    ESP_LOGE(TAG, "Error: %s (code %d)", 
             arcos::abstraction::DRIVER_SD_CARD::getErrorString(result),
             (int)result);
    ESP_LOGE(TAG, "");
    ESP_LOGE(TAG, "Troubleshooting:");
    ESP_LOGE(TAG, "  1. Ensure SD card is inserted");
    ESP_LOGE(TAG, "  2. Check wiring: MOSI=47, MISO=14, SCK=21, CS=48");
    ESP_LOGE(TAG, "  3. Verify card is formatted as FAT32");
    ESP_LOGE(TAG, "  4. Check 3.3V power supply is stable");
    ESP_LOGE(TAG, "========================================");
    
    // Keep task alive
    while(true){ vTaskDelay(pdMS_TO_TICKS(1000)); }
    return;
  }
  
  ESP_LOGI(TAG, "SD card initialized successfully!");
  ESP_LOGI(TAG, "Mount point: %s\n", sd_card.getMountPoint());
  
  // Load file list from root directory
  ESP_LOGI(TAG, "Loading file list from SD card root...");
  loadFileList("");  // Empty string = mount point root (/sdcard/)
  
  if(file_count == 0){
    ESP_LOGW(TAG, "No files found on SD card");
  }else{
    ESP_LOGI(TAG, "Successfully loaded %zu files\n", file_count);
  }
  
  // Print metadata
  printMetadata();
  
  // Print file list
  printFileList();
  
  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "File browser completed successfully!");
  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "");
  ESP_LOGI(TAG, "Driver improvements:");
  ESP_LOGI(TAG, "  + Automatic configuration validation");
  ESP_LOGI(TAG, "  + Human-readable error messages");
  ESP_LOGI(TAG, "  + Pin conflict detection");
  ESP_LOGI(TAG, "  + Convenience methods (getFiles)");
  
  // Keep task alive
  while(true){
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
