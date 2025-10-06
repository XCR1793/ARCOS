/*****************************************************************
 * File:      main_sd_file_browser.cpp
 * Category:  application
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    SD card file browser with OLED display. Features:
 *    - File list view with scrolling
 *    - SD card metadata view (capacity, usage, free space)
 *    - Toggle between metadata and file list views
 *    - File content preview
 *    - FAT32 and exFAT filesystem support
 *    - Visual storage usage bar graph
 * 
 * View Toggle:
 *    The browser cycles through:
 *    1. Metadata view - shows card info and storage statistics
 *    2. File list view - shows directory contents
 *    3. File content view - shows selected file preview
 * 
 * exFAT Support:
 *    To enable exFAT (for cards >32GB or files >4GB):
 *    - Set CONFIG_FATFS_LFN_HEAP=y in sdkconfig
 *    - See EXFAT_SUPPORT.md for details
 *****************************************************************/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "abstraction/hal.hpp"
#include "abstraction/drivers/components/OLED/driver_oled_sh1107.hpp"
#include "abstraction/drivers/components/SD_CARD/driver_sd_card.hpp"

static const char* TAG = "SD_BROWSER";

/** Global driver instances */
static arcos::abstraction::DRIVER_OLED_SH1107 oled(0x3C, 0);
static arcos::abstraction::DRIVER_SD_CARD sd_card;

/** Browser state */
static arcos::abstraction::FileInfo files[50];
static size_t file_count = 0;
static int selected_index = 0;
static int scroll_offset = 0;
static const int MAX_VISIBLE_ITEMS = 8;  // Lines that fit on 128x128 display

/** UI state */
static bool show_file_content = false;
static bool show_metadata = false;  // Toggle between metadata and file list
static char current_file[256];
static uint8_t file_buffer[2048];
static size_t file_size = 0;
static int content_scroll = 0;

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
  selected_index = 0;
  scroll_offset = 0;
  
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
  
  ESP_LOGI(TAG, "Loaded %zu files from %s", file_count, directory);
}

/**
 * @brief Draw SD card metadata screen
 */
void drawMetadata(){
  oled.clearBuffer();
  
  // Draw title
  oled.drawRect(0, 0, 128, 16, true, true);
  oled.drawString(10, 4, "SD Card Info", false);
  oled.drawLine(0, 16, 127, 16, true);
  
  // Get card info
  arcos::abstraction::SdCardInfo info;
  if(sd_card.getCardInfo(&info) == arcos::abstraction::SdCardResult::Success){
    // Card type
    const char* type_str = (info.type == 1) ? "SDSC" : (info.type == 2) ? "SDHC/XC" : "Unknown";
    char line[32];
    
    snprintf(line, sizeof(line), "Type: %s", type_str);
    oled.drawString(2, 20, line, true);
    
    // Capacity
    snprintf(line, sizeof(line), "Size: %llu MB", info.capacity_mb);
    oled.drawString(2, 33, line, true);
    
    // Free space
    uint64_t free_bytes = 0;
    uint64_t total_bytes = 0;
    if(sd_card.getFreeSpace(&free_bytes) == arcos::abstraction::SdCardResult::Success &&
       sd_card.getTotalSpace(&total_bytes) == arcos::abstraction::SdCardResult::Success){
      uint64_t free_mb = free_bytes / (1024 * 1024);
      uint64_t total_mb = total_bytes / (1024 * 1024);
      uint64_t used_mb = total_mb - free_mb;
      
      snprintf(line, sizeof(line), "Free: %llu MB", free_mb);
      oled.drawString(2, 46, line, true);
      
      snprintf(line, sizeof(line), "Used: %llu MB", used_mb);
      oled.drawString(2, 59, line, true);
      
      // Usage percentage bar
      int bar_width = 120;
      int bar_height = 8;
      int bar_x = 4;
      int bar_y = 75;
      
      oled.drawRect(bar_x, bar_y, bar_width, bar_height, false, true);
      if(total_mb > 0){
        int filled = (int)((used_mb * (bar_width - 4)) / total_mb);
        if(filled > 0){
          oled.drawRect(bar_x + 2, bar_y + 2, filled, bar_height - 4, true, true);
        }
      }
      
      // Percentage text
      int usage_pct = total_mb > 0 ? (int)((used_mb * 100) / total_mb) : 0;
      snprintf(line, sizeof(line), "Usage: %d%%", usage_pct);
      oled.drawString(35, 90, line, true);
    }
    
    // File count
    snprintf(line, sizeof(line), "Files: %zu", file_count);
    oled.drawString(2, 105, line, true);
  }else{
    oled.drawString(10, 50, "Card info", true);
    oled.drawString(10, 65, "unavailable", true);
  }
  
  oled.updateDisplay();
}

/**
 * @brief Draw header with SD card info
 */
void drawHeader(){
  // Draw title bar
  oled.drawRect(0, 0, 128, 16, true, true);
  oled.drawString(2, 4, "SD Card Browser", false);
  
  // Draw separator line
  oled.drawLine(0, 16, 127, 16, true);
}

/**
 * @brief Draw file list view
 */
void drawFileList(){
  // Check if we should show metadata instead
  if(show_metadata){
    drawMetadata();
    return;
  }
  
  oled.clearBuffer();
  drawHeader();
  
  if(file_count == 0){
    oled.drawString(10, 40, "No files found", true);
    oled.drawString(10, 55, "or SD card not", true);
    oled.drawString(10, 70, "initialized", true);
    oled.updateDisplay();
    return;
  }
  
  // Draw file counter
  char counter[32];
  snprintf(counter, sizeof(counter), "%d/%zu", selected_index + 1, file_count);
  oled.drawString(90, 4, counter, false);
  
  // Calculate visible range
  int visible_start = scroll_offset;
  int visible_end = scroll_offset + MAX_VISIBLE_ITEMS;
  if(visible_end > (int)file_count) visible_end = file_count;
  
  // Draw visible files
  int y_pos = 20;
  for(int i = visible_start; i < visible_end; i++){
    bool is_selected = (i == selected_index);
    
    // Draw selection indicator
    if(is_selected){
      oled.drawString(2, y_pos, ">", true);
    }
    
    // Draw file/folder icon and name
    if(files[i].is_directory){
      oled.drawString(10, y_pos, "[D]", true);
    }else{
      oled.drawString(10, y_pos, "[ ]", true);
    }
    
    // Truncate long names
    char name[20];
    strncpy(name, files[i].name, 15);
    name[15] = '\0';
    if(strlen(files[i].name) > 15){
      strcat(name, "...");
    }
    
    oled.drawString(28, y_pos, name, true);
    
    // Draw file size if not directory
    if(!files[i].is_directory){
      char size_str[16];
      formatFileSize(files[i].size, size_str, sizeof(size_str));
      
      // Right-align size
      int size_x = 128 - strlen(size_str) * 6 - 2;
      oled.drawString(size_x, y_pos, size_str, true);
    }
    
    y_pos += 13;
  }
  
  // Draw scroll indicator if needed
  if(file_count > MAX_VISIBLE_ITEMS){
    int scroll_bar_height = (MAX_VISIBLE_ITEMS * 90) / file_count;
    int scroll_bar_pos = (scroll_offset * 90) / file_count;
    oled.drawRect(126, 18 + scroll_bar_pos, 2, scroll_bar_height, true, true);
  }
  
  // Draw footer with controls
  oled.drawLine(0, 112, 127, 112, true);
  oled.drawString(2, 116, "UP/DOWN:Nav ENTER:Open", true);
  
  oled.updateDisplay();
}

/**
 * @brief Draw file content view
 */
void drawFileContent(){
  oled.clearBuffer();
  
  // Draw header
  oled.drawRect(0, 0, 128, 16, true, true);
  oled.drawString(2, 4, "File Content", false);
  
  oled.drawLine(0, 16, 127, 16, true);
  
  // Display file name
  char short_name[20];
  strncpy(short_name, files[selected_index].name, 18);
  short_name[18] = '\0';
  oled.drawString(2, 20, short_name, true);
  
  // Display file size
  char size_info[32];
  snprintf(size_info, sizeof(size_info), "Size: %u bytes", (unsigned int)file_size);
  oled.drawString(2, 32, size_info, true);
  
  // Draw separator
  oled.drawLine(0, 44, 127, 44, true);
  
  // Display content (text preview)
  int y = 48;
  int line_start = content_scroll;
  int chars_per_line = 21;  // ~6 pixels per char, 128 pixels wide
  
  for(int line = 0; line < 5 && line_start < (int)file_size; line++){
    char line_buffer[22];
    int char_count = 0;
    
    // Read characters for this line
    while(char_count < chars_per_line && line_start < (int)file_size){
      char c = file_buffer[line_start++];
      
      // Handle newline
      if(c == '\n'){
        break;
      }
      
      // Only display printable characters
      if(c >= 32 && c <= 126){
        line_buffer[char_count++] = c;
      }else if(c == '\t'){
        line_buffer[char_count++] = ' ';
      }
    }
    
    line_buffer[char_count] = '\0';
    oled.drawString(2, y, line_buffer, true);
    y += 12;
  }
  
  // Draw footer
  oled.drawLine(0, 112, 127, 112, true);
  oled.drawString(2, 116, "ESC:Back", true);
  
  oled.updateDisplay();
}

/**
 * @brief Handle navigation up
 */
void navigateUp(){
  if(file_count == 0) return;
  
  if(selected_index > 0){
    selected_index--;
    
    // Adjust scroll if needed
    if(selected_index < scroll_offset){
      scroll_offset = selected_index;
    }
  }
}

/**
 * @brief Handle navigation down
 */
void navigateDown(){
  if(file_count == 0) return;
  
  if(selected_index < (int)file_count - 1){
    selected_index++;
    
    // Adjust scroll if needed
    if(selected_index >= scroll_offset + MAX_VISIBLE_ITEMS){
      scroll_offset = selected_index - MAX_VISIBLE_ITEMS + 1;
    }
  }
}

/**
 * @brief Handle file selection (open/view)
 */
void selectFile(){
  if(file_count == 0) return;
  
  if(files[selected_index].is_directory){
    ESP_LOGI(TAG, "Cannot open directories yet");
    return;
  }
  
  // Read file content
  strncpy(current_file, files[selected_index].name, sizeof(current_file) - 1);
  
  size_t bytes_read = 0;
  arcos::abstraction::SdCardResult result = sd_card.readFile(
    current_file,
    file_buffer,
    sizeof(file_buffer),
    &bytes_read
  );
  
  if(result == arcos::abstraction::SdCardResult::Success){
    file_size = bytes_read;
    content_scroll = 0;
    show_file_content = true;
    ESP_LOGI(TAG, "Opened file: %s (%zu bytes)", current_file, file_size);
  }else{
    ESP_LOGE(TAG, "Failed to read file: %s", current_file);
  }
}

/**
 * @brief Display SD card information screen
 */
void displayCardInfo(){
  oled.clearBuffer();
  
  arcos::abstraction::SdCardInfo info;
  arcos::abstraction::SdCardResult result = sd_card.getCardInfo(&info);
  
  if(result == arcos::abstraction::SdCardResult::Success){
    oled.drawString(10, 5, "SD Card Info", true);
    oled.drawLine(0, 18, 127, 18, true);
    
    char buf[32];
    
    snprintf(buf, sizeof(buf), "Name: %s", info.name);
    oled.drawString(5, 25, buf, true);
    
    snprintf(buf, sizeof(buf), "Capacity: %llu MB", info.capacity_mb);
    oled.drawString(5, 40, buf, true);
    
    snprintf(buf, sizeof(buf), "Free: %llu MB", info.free_mb);
    oled.drawString(5, 55, buf, true);
    
    snprintf(buf, sizeof(buf), "Used: %llu MB", info.capacity_mb - info.free_mb);
    oled.drawString(5, 70, buf, true);
    
    float usage = 0;
    if(info.capacity_mb > 0){
      usage = ((float)(info.capacity_mb - info.free_mb) / info.capacity_mb) * 100.0f;
    }
    snprintf(buf, sizeof(buf), "Usage: %.1f%%", usage);
    oled.drawString(5, 85, buf, true);
    
    snprintf(buf, sizeof(buf), "Files: %zu", file_count);
    oled.drawString(5, 100, buf, true);
  }else{
    oled.drawString(10, 40, "SD Card Error", true);
    oled.drawString(10, 55, "Check connection", true);
  }
  
  oled.updateDisplay();
  vTaskDelay(pdMS_TO_TICKS(3000));
}

/**
 * @brief Main browser task
 */
void browserTask(void* parameter){
  uint32_t last_refresh = 0;
  const uint32_t refresh_interval = 100; // ms
  
  while(true){
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    
    // Periodic refresh
    if(now - last_refresh >= refresh_interval){
      last_refresh = now;
      
      if(show_file_content){
        drawFileContent();
      }else{
        drawFileList();
      }
    }
    
    // Handle input (simulated with timer-based auto-scroll for demo)
    // In real application, you would read from buttons/input device
    static uint32_t last_action = 0;
    static int demo_state = 0;
    if(now - last_action > 2000){  // Auto-demo every 2 seconds
      last_action = now;
      
      if(show_file_content){
        // Auto-close file view
        show_file_content = false;
        demo_state = 0;
      }else if(show_metadata){
        // Toggle back to file list
        show_metadata = false;
        demo_state = 0;
      }else{
        // Cycle through demo states
        switch(demo_state){
          case 0:
            // Show metadata view
            show_metadata = true;
            ESP_LOGI(TAG, "Switched to metadata view");
            demo_state++;
            break;
          case 1:
            // Back to file list
            show_metadata = false;
            ESP_LOGI(TAG, "Switched to file list view");
            demo_state++;
            break;
          default:
            // Auto-navigate in file list
            if(demo_state % 2 == 0){
              navigateDown();
            }else{
              navigateUp();
            }
            demo_state++;
            if(demo_state > 5){
              demo_state = 0;  // Reset cycle
            }
            break;
        }
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

/**
 * @brief Application entry point
 */
extern "C" void app_main(void){
  ESP_LOGI(TAG, "==================================");
  ESP_LOGI(TAG, "ARCOS SD Card File Browser");
  ESP_LOGI(TAG, "==================================");
  
  // Initialize I2C bus for OLED
  ESP_LOGI(TAG, "Initializing I2C for OLED...");
  arcos::abstraction::HalResult i2c_result = arcos::abstraction::ESP32S3_I2C::Initialize(0, 1, 2, 400000);
  if(i2c_result != arcos::abstraction::HalResult::Success){
    ESP_LOGE(TAG, "Failed to initialize I2C bus");
    return;
  }
  
  // Initialize OLED display
  ESP_LOGI(TAG, "Initializing OLED display...");
  arcos::abstraction::OLEDConfig oled_config;
  oled_config.contrast = 0xFF;  // Maximum contrast for testing
  oled_config.flip_horizontal = false;   // Flip left to right
  oled_config.flip_vertical = false;
  
  if(!oled.initialize(oled_config)){
    ESP_LOGE(TAG, "Failed to initialize OLED display");
    // Don't return, keep trying
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  ESP_LOGI(TAG, "OLED display initialized");
  
  // Test display with simple pattern
  ESP_LOGI(TAG, "Testing OLED display...");
  oled.clearBuffer();
  oled.drawRect(0, 0, 128, 128, true, false);  // Border
  oled.drawString(25, 30, "DISPLAY", true);
  oled.drawString(30, 50, "TEST", true);
  oled.drawString(35, 70, "OK!", true);
  oled.updateDisplay();
  ESP_LOGI(TAG, "Display test pattern shown");
  vTaskDelay(pdMS_TO_TICKS(2000));
  
  // Show splash screen
  oled.clearBuffer();
  oled.drawRect(0, 0, 128, 128, false, true);
  oled.drawString(15, 30, "ARCOS", true);
  oled.drawString(5, 50, "SD File Browser", true);
  oled.drawString(20, 80, "Initializing...", true);
  oled.updateDisplay();
  ESP_LOGI(TAG, "Splash screen shown");
  vTaskDelay(pdMS_TO_TICKS(2000));
  
  // Initialize SD card with exFAT support
  ESP_LOGI(TAG, "Initializing SD card (FAT32/exFAT support)...");
  arcos::abstraction::SdCardConfig sd_config;
  sd_config.spi_host = 1;  // SPI2_HOST
  sd_config.pin_mosi = 21;  // MOSI
  sd_config.pin_miso = 48;  // MISO
  sd_config.pin_sck  = 47;  // CLK/SCK
  sd_config.pin_cs   = 14;  // CS
  sd_config.max_frequency_hz = 20000000;
  sd_config.max_open_files = 5;
  sd_config.format_if_failed = false;
  sd_config.mount_point = "/sdcard";
  
  arcos::abstraction::SdCardResult sd_result = sd_card.initialize(sd_config);
  
  if(sd_result != arcos::abstraction::SdCardResult::Success){
    ESP_LOGE(TAG, "Failed to initialize SD card! Error code: %d", (int)sd_result);
    
    // Show detailed error on OLED
    oled.clearBuffer();
    oled.drawRect(0, 0, 128, 128, false, true);
    oled.drawString(20, 20, "SD Card", true);
    oled.drawString(20, 35, "Error!", true);
    
    // Show specific error
    char err_msg[32];
    snprintf(err_msg, sizeof(err_msg), "Code: %d", (int)sd_result);
    oled.drawString(20, 55, err_msg, true);
    
    oled.drawString(5, 75, "Check:", true);
    oled.drawString(5, 90, "-Card inserted", true);
    oled.drawString(5, 105, "-FAT32 format", true);
    oled.updateDisplay();
    
    ESP_LOGE(TAG, "Troubleshooting:");
    ESP_LOGE(TAG, "  1. Ensure SD card is inserted");
    ESP_LOGE(TAG, "  2. Check wiring: MOSI=47, MISO=14, SCK=21, CS=48");
    ESP_LOGE(TAG, "  3. Verify card is formatted as FAT32");
    ESP_LOGE(TAG, "  4. Try lower SPI frequency (reduce max_frequency_hz)");
    ESP_LOGE(TAG, "  5. Check power supply (3.3V stable)");
    
    // Keep display on and continue without SD card
    while(true){
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    return;
  }
  
  ESP_LOGI(TAG, "SD card initialized successfully");
  
  // Display card info
  displayCardInfo();
  
  // Load initial file list
  ESP_LOGI(TAG, "Loading file list from SD card root...");
  loadFileList("");  // Empty string = mount point root (/sdcard/)
  
  // Show loading complete
  oled.clearBuffer();
  oled.drawRect(0, 0, 128, 128, false, true);
  oled.drawString(30, 50, "Ready!", true);
  char file_info[32];
  snprintf(file_info, sizeof(file_info), "%zu files found", file_count);
  oled.drawString(20, 70, file_info, true);
  oled.updateDisplay();
  vTaskDelay(pdMS_TO_TICKS(1500));
  
  // Start browser task
  ESP_LOGI(TAG, "Starting browser task...");
  xTaskCreatePinnedToCore(
    browserTask,
    "sd_browser",
    8192,
    nullptr,
    5,
    nullptr,
    1
  );
  
  ESP_LOGI(TAG, "SD Card File Browser running");
  ESP_LOGI(TAG, "Found %zu files on SD card", file_count);
}
