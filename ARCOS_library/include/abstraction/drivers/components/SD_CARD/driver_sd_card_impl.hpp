/*****************************************************************
 * File:      driver_sd_card_impl.hpp
 * Category:  abstraction/drivers/components/SD_CARD
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Implementation of SD card driver for ESP32-S3 using SPI
 *    interface and ESP-IDF VFS FAT filesystem.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_COMPONENTS_SD_CARD_DRIVER_SD_CARD_IMPL_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_COMPONENTS_SD_CARD_DRIVER_SD_CARD_IMPL_HPP_

#include "esp_vfs_fat.h"
#include "esp_err.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include <sys/stat.h>
#include <sys/unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#include "abstraction/hal.hpp"

namespace arcos::abstraction{

static const char* SD_CARD_TAG = "DRIVER_SD_CARD";

// ============== CONSTRUCTOR / DESTRUCTOR ==============

inline DRIVER_SD_CARD::DRIVER_SD_CARD()
  : initialized_(false)
  , card_handle_(nullptr)
{
  strncpy(mount_point_, "/sdcard", sizeof(mount_point_));
  memset(&config_, 0, sizeof(config_));
}

inline DRIVER_SD_CARD::~DRIVER_SD_CARD(){
  if(initialized_){
    deinitialize();
  }
}

// ============== INITIALIZATION ==============

inline SdCardResult DRIVER_SD_CARD::initialize(){
  SdCardConfig default_config;
  default_config.spi_host = 1; // SPI2_HOST
  default_config.pin_mosi = 47;
  default_config.pin_miso = 14;
  default_config.pin_sck  = 21;
  default_config.pin_cs   = 48;
  default_config.max_frequency_hz = 20000000; // 20MHz
  default_config.max_open_files = 5;
  default_config.format_if_failed = false;
  default_config.mount_point = "/sdcard";
  
  return initialize(default_config);
}

inline SdCardResult DRIVER_SD_CARD::initialize(const SdCardConfig& config){
  if(initialized_){
    ESP_LOGW(SD_CARD_TAG, "SD card already initialized");
    return SdCardResult::Success;
  }
  
  config_ = config;
  strncpy(mount_point_, config.mount_point, sizeof(mount_point_) - 1);
  mount_point_[sizeof(mount_point_) - 1] = '\0';
  
  // Initialize SPI bus using HAL
  esp_err_t ret = HAL_PROTOCAL_SPI::Initialise(
    (spi_host_device_t)config.spi_host,
    (gpio_num_t)config.pin_mosi,
    (gpio_num_t)config.pin_miso,
    (gpio_num_t)config.pin_sck,
    SPI_DMA_CH_AUTO
  );
  
  if(ret != ESP_OK){
    ESP_LOGE(SD_CARD_TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
    return SdCardResult::InitError;
  }
  
  // Configure SDSPI host
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = (spi_host_device_t)config.spi_host;
  
  // Configure device slot
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = (gpio_num_t)config.pin_cs;
  slot_config.host_id = (spi_host_device_t)config.spi_host;
  
  // Configure VFS FAT mount options
  esp_vfs_fat_mount_config_t mount_cfg = {
    .format_if_mount_failed = config.format_if_failed,
    .max_files = config.max_open_files,
    .allocation_unit_size = 16 * 1024,
    .disk_status_check_enable = false,
    .use_one_fat = false,
  };
  
  // Mount filesystem
  sdmmc_card_t* card;
  ret = esp_vfs_fat_sdspi_mount(
    mount_point_,
    &host,
    &slot_config,
    &mount_cfg,
    &card
  );
  
  if(ret != ESP_OK){
    ESP_LOGE(SD_CARD_TAG, "Failed to mount filesystem: %s", esp_err_to_name(ret));
    
    if(ret == ESP_FAIL){
      ESP_LOGE(SD_CARD_TAG, "Card not present or failed to initialize");
      return SdCardResult::CardNotPresent;
    }
    return SdCardResult::MountError;
  }
  
  card_handle_ = (void*)card;
  initialized_ = true;
  
  ESP_LOGI(SD_CARD_TAG, "SD card mounted successfully at %s", mount_point_);
  
  // Log card info
  SdCardInfo info;
  if(getCardInfo(&info) == SdCardResult::Success){
    ESP_LOGI(SD_CARD_TAG, "Card: %s, Capacity: %llu MB, Free: %llu MB",
             info.name, info.capacity_mb, info.free_mb);
  }
  
  return SdCardResult::Success;
}

inline SdCardResult DRIVER_SD_CARD::deinitialize(){
  if(!initialized_){
    return SdCardResult::NotInitialized;
  }
  
  // Unmount filesystem
  esp_err_t ret = esp_vfs_fat_sdcard_unmount(mount_point_, (sdmmc_card_t*)card_handle_);
  if(ret != ESP_OK){
    ESP_LOGE(SD_CARD_TAG, "Failed to unmount filesystem: %s", esp_err_to_name(ret));
  }
  
  // Free SPI bus
  ret = spi_bus_free((spi_host_device_t)config_.spi_host);
  if(ret != ESP_OK){
    ESP_LOGW(SD_CARD_TAG, "Failed to free SPI bus: %s", esp_err_to_name(ret));
  }
  
  initialized_ = false;
  card_handle_ = nullptr;
  
  ESP_LOGI(SD_CARD_TAG, "SD card unmounted");
  
  return SdCardResult::Success;
}

// ============== CARD INFORMATION ==============

inline SdCardResult DRIVER_SD_CARD::getCardInfo(SdCardInfo* info){
  if(!initialized_){
    return SdCardResult::NotInitialized;
  }
  
  if(!info){
    return SdCardResult::InvalidParameter;
  }
  
  sdmmc_card_t* card = (sdmmc_card_t*)card_handle_;
  
  // Copy card name
  strncpy(info->name, card->cid.name, sizeof(info->name) - 1);
  info->name[sizeof(info->name) - 1] = '\0';
  
  // Calculate capacity in MB
  info->capacity_mb = ((uint64_t)card->csd.capacity * card->csd.sector_size) / (1024 * 1024);
  info->sector_size = card->csd.sector_size;
  
  // Determine card type based on OCR register CCS bit
  // CCS (Card Capacity Status) bit 30 in OCR register indicates SDHC/SDXC
  // If CCS=1: SDHC or SDXC (High/Extended Capacity)
  // If CCS=0: SDSC (Standard Capacity, up to 2GB)
  info->type = (card->ocr & (1 << 30)) ? 2 : 1; // Check CCS bit
  info->is_mounted = initialized_;
  
  // Get free space
  uint64_t free_bytes;
  if(getFreeSpace(&free_bytes) == SdCardResult::Success){
    info->free_mb = free_bytes / (1024 * 1024);
  }else{
    info->free_mb = 0;
  }
  
  return SdCardResult::Success;
}

inline SdCardResult DRIVER_SD_CARD::getFreeSpace(uint64_t* free_bytes){
  if(!initialized_){
    return SdCardResult::NotInitialized;
  }
  
  if(!free_bytes){
    return SdCardResult::InvalidParameter;
  }
  
  FATFS* fs;
  DWORD free_clusters;
  
  char drive_path[64];  // Increased buffer size to accommodate mount point path
  snprintf(drive_path, sizeof(drive_path), "%s", mount_point_);
  
  if(f_getfree(drive_path, &free_clusters, &fs) != FR_OK){
    return SdCardResult::DirectoryError;
  }
  
  *free_bytes = (uint64_t)free_clusters * fs->csize * 512;
  
  return SdCardResult::Success;
}

inline SdCardResult DRIVER_SD_CARD::getTotalSpace(uint64_t* total_bytes){
  if(!initialized_){
    return SdCardResult::NotInitialized;
  }
  
  if(!total_bytes){
    return SdCardResult::InvalidParameter;
  }
  
  sdmmc_card_t* card = (sdmmc_card_t*)card_handle_;
  *total_bytes = (uint64_t)card->csd.capacity * card->csd.sector_size;
  
  return SdCardResult::Success;
}

// ============== FILE OPERATIONS ==============

inline void DRIVER_SD_CARD::buildAbsolutePath(const char* filepath, char* buffer, size_t buffer_size){
  if(filepath[0] == '/'){
    // Absolute path provided
    snprintf(buffer, buffer_size, "%s", filepath);
  }else{
    // Relative path - prepend mount point
    snprintf(buffer, buffer_size, "%s/%s", mount_point_, filepath);
  }
}

inline SdCardResult DRIVER_SD_CARD::readFile(const char* filepath,
                                             uint8_t* buffer,
                                             size_t buffer_size,
                                             size_t* bytes_read){
  if(!initialized_){
    return SdCardResult::NotInitialized;
  }
  
  if(!filepath || !buffer || !bytes_read){
    return SdCardResult::InvalidParameter;
  }
  
  char abs_path[256];
  buildAbsolutePath(filepath, abs_path, sizeof(abs_path));
  
  FILE* f = fopen(abs_path, "rb");
  if(!f){
    ESP_LOGE(SD_CARD_TAG, "Failed to open file for reading: %s", abs_path);
    return SdCardResult::FileNotFound;
  }
  
  *bytes_read = fread(buffer, 1, buffer_size, f);
  
  if(ferror(f)){
    fclose(f);
    return SdCardResult::FileReadError;
  }
  
  fclose(f);
  return SdCardResult::Success;
}

inline SdCardResult DRIVER_SD_CARD::writeFile(const char* filepath,
                                              const uint8_t* data,
                                              size_t data_size){
  if(!initialized_){
    return SdCardResult::NotInitialized;
  }
  
  if(!filepath || !data){
    return SdCardResult::InvalidParameter;
  }
  
  char abs_path[256];
  buildAbsolutePath(filepath, abs_path, sizeof(abs_path));
  
  FILE* f = fopen(abs_path, "wb");
  if(!f){
    ESP_LOGE(SD_CARD_TAG, "Failed to open file for writing: %s", abs_path);
    return SdCardResult::FileOpenError;
  }
  
  size_t written = fwrite(data, 1, data_size, f);
  fclose(f);
  
  if(written != data_size){
    ESP_LOGE(SD_CARD_TAG, "Failed to write complete data to file");
    return SdCardResult::FileWriteError;
  }
  
  return SdCardResult::Success;
}

inline SdCardResult DRIVER_SD_CARD::appendFile(const char* filepath,
                                               const uint8_t* data,
                                               size_t data_size){
  if(!initialized_){
    return SdCardResult::NotInitialized;
  }
  
  if(!filepath || !data){
    return SdCardResult::InvalidParameter;
  }
  
  char abs_path[256];
  buildAbsolutePath(filepath, abs_path, sizeof(abs_path));
  
  FILE* f = fopen(abs_path, "ab");
  if(!f){
    ESP_LOGE(SD_CARD_TAG, "Failed to open file for appending: %s", abs_path);
    return SdCardResult::FileOpenError;
  }
  
  size_t written = fwrite(data, 1, data_size, f);
  fclose(f);
  
  if(written != data_size){
    ESP_LOGE(SD_CARD_TAG, "Failed to append complete data to file");
    return SdCardResult::FileWriteError;
  }
  
  return SdCardResult::Success;
}

inline SdCardResult DRIVER_SD_CARD::deleteFile(const char* filepath){
  if(!initialized_){
    return SdCardResult::NotInitialized;
  }
  
  if(!filepath){
    return SdCardResult::InvalidParameter;
  }
  
  char abs_path[256];
  buildAbsolutePath(filepath, abs_path, sizeof(abs_path));
  
  if(unlink(abs_path) != 0){
    ESP_LOGE(SD_CARD_TAG, "Failed to delete file: %s", abs_path);
    return SdCardResult::FileNotFound;
  }
  
  return SdCardResult::Success;
}

inline SdCardResult DRIVER_SD_CARD::fileExists(const char* filepath, bool* exists){
  if(!initialized_){
    return SdCardResult::NotInitialized;
  }
  
  if(!filepath || !exists){
    return SdCardResult::InvalidParameter;
  }
  
  char abs_path[256];
  buildAbsolutePath(filepath, abs_path, sizeof(abs_path));
  
  struct stat st;
  *exists = (stat(abs_path, &st) == 0 && S_ISREG(st.st_mode));
  
  return SdCardResult::Success;
}

inline SdCardResult DRIVER_SD_CARD::getFileSize(const char* filepath, uint64_t* size){
  if(!initialized_){
    return SdCardResult::NotInitialized;
  }
  
  if(!filepath || !size){
    return SdCardResult::InvalidParameter;
  }
  
  char abs_path[256];
  buildAbsolutePath(filepath, abs_path, sizeof(abs_path));
  
  struct stat st;
  if(stat(abs_path, &st) != 0){
    return SdCardResult::FileNotFound;
  }
  
  *size = st.st_size;
  return SdCardResult::Success;
}

inline SdCardResult DRIVER_SD_CARD::renameFile(const char* old_path, const char* new_path){
  if(!initialized_){
    return SdCardResult::NotInitialized;
  }
  
  if(!old_path || !new_path){
    return SdCardResult::InvalidParameter;
  }
  
  char abs_old[256], abs_new[256];
  buildAbsolutePath(old_path, abs_old, sizeof(abs_old));
  buildAbsolutePath(new_path, abs_new, sizeof(abs_new));
  
  if(rename(abs_old, abs_new) != 0){
    ESP_LOGE(SD_CARD_TAG, "Failed to rename file from %s to %s", abs_old, abs_new);
    return SdCardResult::FileNotFound;
  }
  
  return SdCardResult::Success;
}

// ============== DIRECTORY OPERATIONS ==============

inline SdCardResult DRIVER_SD_CARD::createDirectory(const char* dirpath){
  if(!initialized_){
    return SdCardResult::NotInitialized;
  }
  
  if(!dirpath){
    return SdCardResult::InvalidParameter;
  }
  
  char abs_path[256];
  buildAbsolutePath(dirpath, abs_path, sizeof(abs_path));
  
  if(mkdir(abs_path, 0775) != 0){
    ESP_LOGE(SD_CARD_TAG, "Failed to create directory: %s", abs_path);
    return SdCardResult::DirectoryError;
  }
  
  return SdCardResult::Success;
}

inline SdCardResult DRIVER_SD_CARD::removeDirectory(const char* dirpath){
  if(!initialized_){
    return SdCardResult::NotInitialized;
  }
  
  if(!dirpath){
    return SdCardResult::InvalidParameter;
  }
  
  char abs_path[256];
  buildAbsolutePath(dirpath, abs_path, sizeof(abs_path));
  
  if(rmdir(abs_path) != 0){
    ESP_LOGE(SD_CARD_TAG, "Failed to remove directory: %s", abs_path);
    return SdCardResult::DirectoryError;
  }
  
  return SdCardResult::Success;
}

inline SdCardResult DRIVER_SD_CARD::listDirectory(const char* dirpath,
                                                  FileInfo* files,
                                                  size_t max_files,
                                                  size_t* file_count){
  if(!initialized_){
    return SdCardResult::NotInitialized;
  }
  
  if(!dirpath || !files || !file_count){
    return SdCardResult::InvalidParameter;
  }
  
  char abs_path[256];
  buildAbsolutePath(dirpath, abs_path, sizeof(abs_path));
  
  DIR* dir = opendir(abs_path);
  if(!dir){
    ESP_LOGE(SD_CARD_TAG, "Failed to open directory: %s", abs_path);
    return SdCardResult::DirectoryError;
  }
  
  *file_count = 0;
  struct dirent* entry;
  
  while((entry = readdir(dir)) != NULL && *file_count < max_files){
    // Skip "." and ".."
    if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
      continue;
    }
    
    FileInfo* info = &files[*file_count];
    strncpy(info->name, entry->d_name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    info->is_directory = (entry->d_type == DT_DIR);
    
    // Get file size and modification time
    char full_path[512];  // Increased to accommodate max path + filename
    snprintf(full_path, sizeof(full_path), "%s/%s", abs_path, entry->d_name);
    
    struct stat st;
    if(stat(full_path, &st) == 0){
      info->size = st.st_size;
      info->modified = st.st_mtime;
    }else{
      info->size = 0;
      info->modified = 0;
    }
    
    (*file_count)++;
  }
  
  closedir(dir);
  return SdCardResult::Success;
}

inline SdCardResult DRIVER_SD_CARD::directoryExists(const char* dirpath, bool* exists){
  if(!initialized_){
    return SdCardResult::NotInitialized;
  }
  
  if(!dirpath || !exists){
    return SdCardResult::InvalidParameter;
  }
  
  char abs_path[256];
  buildAbsolutePath(dirpath, abs_path, sizeof(abs_path));
  
  struct stat st;
  *exists = (stat(abs_path, &st) == 0 && S_ISDIR(st.st_mode));
  
  return SdCardResult::Success;
}

// ============== UTILITY FUNCTIONS ==============

inline SdCardResult DRIVER_SD_CARD::formatCard(){
  if(!initialized_){
    return SdCardResult::NotInitialized;
  }
  
  ESP_LOGW(SD_CARD_TAG, "Formatting SD card - all data will be lost!");
  
  // Unmount first
  deinitialize();
  
  // Remount with format flag
  SdCardConfig format_config = config_;
  format_config.format_if_failed = true;
  
  return initialize(format_config);
}

} // namespace arcos::abstraction

#endif // ARCOS_ABSTRACTION_DRIVERS_COMPONENTS_SD_CARD_DRIVER_SD_CARD_IMPL_HPP_
