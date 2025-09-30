/*****************************************************************
 * File:      main.cpp
 * Category:  application
 * Author:    XCR1793 (Feather Forge) - fixed by assistant
 *
 * Purpose:
 *   Interactive SD-card file browser over UART console.
 *   Uses SPI bus HAL abstraction and ESP-IDF VFS FAT mount.
 *****************************************************************/

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "esp_timer.h"

#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"

#include "abstraction/hal.hpp"

using arcos::abstraction::HAL_PROTOCAL_SPI;

static const char *TAG = "SD_SPI_BROWSER";

#define MOUNT_POINT "/sdcard"
#define MAX_FILES   100

// GPIO pin configuration
static constexpr gpio_num_t PIN_NUM_MISO = GPIO_NUM_14;
static constexpr gpio_num_t PIN_NUM_MOSI = GPIO_NUM_47;
static constexpr gpio_num_t PIN_NUM_CLK  = GPIO_NUM_21;
static constexpr gpio_num_t PIN_NUM_CS   = GPIO_NUM_48;

static char *fileNames[MAX_FILES];
static int   fileCount     = 0;
static int   selectedIndex = 0;

static int      escapeState     = 0;
static uint32_t lastActionTime  = 0;
static const uint32_t debounceDelay = 500;  // ms

/**
 * @brief Populate fileNames[] with files in directory.
 */
static void list_files(const char *path){
  DIR *dir = opendir(path);
  if(!dir){
    ESP_LOGE(TAG, "Failed to open dir: %s", path);
    return;
  }

  fileCount = 0;
  struct dirent *entry;
  while((entry = readdir(dir)) != NULL && fileCount < MAX_FILES){
    if(entry->d_type != DT_DIR){
      fileNames[fileCount] = strdup(entry->d_name);
      fileCount++;
    }
  }
  closedir(dir);
}

/**
 * @brief Print file menu with highlight.
 */
static void print_menu(){
  printf("\n===== File Browser =====\n");
  for(int i = 0; i < fileCount; i++){
    if(i == selectedIndex) printf("> %s\n", fileNames[i]);
    else printf("  %s\n", fileNames[i]);
  }
  printf("\nControls: [↑/↓] Navigate  [Enter] Read  [Space] Diagnostics\n");
}

/**
 * @brief Read and print contents of file.
 */
static void read_and_print_file(const char *filename) {
  char filepath[128];
  snprintf(filepath, sizeof(filepath), "%s/%s", MOUNT_POINT, filename);

  FILE *f = fopen(filepath, "r");
  if (!f) {
    printf("❌ Failed to open file: %s\n", filepath);
    return;
  }

  printf("📖 Reading %s:\n", filename);
  char buf[64];
  while (fgets(buf, sizeof(buf), f)) {
    printf("%s", buf);
  }
  fclose(f);
  printf("\n===== End of File =====\n");
}

/**
 * @brief Run SD diagnostics and refresh menu.
 */
static void run_diagnostics(sdmmc_card_t *card) {
  printf("\n===== Running SD Diagnostics =====\n");
  if (!card) {
    printf("❌ SD card not initialized\n");
    return;
  }

  printf("✅ SD card initialized successfully.\n");
  printf("📦 Name: %s\n", card->cid.name);
  printf("💾 Card size: %llu MB\n",
         (uint64_t)card->csd.capacity * card->csd.sector_size /
         (1024 * 1024));

  list_files(MOUNT_POINT);
  selectedIndex = 0;
  print_menu();
}

/**
 * @brief Application entry point.
 */
extern "C" void app_main(void) {
  esp_err_t ret;

  // Initialize SPI bus
  ret = HAL_PROTOCAL_SPI::Initialise(SPI2_HOST,
                                     PIN_NUM_MOSI,
                                     PIN_NUM_MISO,
                                     PIN_NUM_CLK,
                                     SPI_DMA_CH_AUTO);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "HAL SPI bus init failed: %s", esp_err_to_name(ret));
    return;
  }

  // Configure SDSPI host
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SPI2_HOST;

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = PIN_NUM_CS;
  slot_config.host_id = SPI2_HOST;

  esp_vfs_fat_mount_config_t mount_cfg = {
    .format_if_mount_failed     = false,
    .max_files                  = 5,
    .allocation_unit_size       = 16 * 1024,
    .disk_status_check_enable   = false,
    .use_one_fat                = false,
  };

  sdmmc_card_t *card;
  ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT,
                                &host,
                                &slot_config,
                                &mount_cfg,
                                &card);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG,
             "Failed to mount filesystem: %s",
             esp_err_to_name(ret));
    return;
  }

  printf("SD card mounted at %s\n", MOUNT_POINT);
  run_diagnostics(card);

  // Timer for periodic SD info
  uint32_t lastSdPrintTime = (uint32_t)(esp_timer_get_time() / 1000);

  // Interactive loop
  while (1) {
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

    // Periodic SD card info every 3 seconds
    if (now - lastSdPrintTime >= 3000) {
      lastSdPrintTime = now;
      run_diagnostics(card);
    }

    // Handle user input
    int c = getchar();
    if (c == EOF) {
      vTaskDelay(10 / portTICK_PERIOD_MS);
      continue;
    }

    if (escapeState == 0) {
      if (c == 27) escapeState = 1;
      else if (c == '\n' || c == '\r') {
        if (now - lastActionTime >= debounceDelay && fileCount > 0) {
          lastActionTime = now;
          read_and_print_file(fileNames[selectedIndex]);
        }
      } else if (c == ' ') {
        if (now - lastActionTime >= debounceDelay) {
          lastActionTime = now;
          run_diagnostics(card);
        }
      }
    } else if (escapeState == 1) {
      if (c == '[') escapeState = 2;
      else escapeState = 0;
    } else if (escapeState == 2) {
      escapeState = 0;
      if (now - lastActionTime < debounceDelay) continue;
      lastActionTime = now;

      if (c == 'A') {  // up arrow
        selectedIndex = (selectedIndex - 1 + fileCount) % fileCount;
        print_menu();
      } else if (c == 'B') {  // down arrow
        selectedIndex = (selectedIndex + 1) % fileCount;
        print_menu();
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
