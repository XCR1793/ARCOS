/*****************************************************************
 * File:      driver_sd_card.hpp
 * Category:  abstraction/drivers/components/SD_CARD
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    SD card driver abstraction using SPI interface with VFS FAT
 *    filesystem support. Provides high-level file operations,
 *    directory management, and card metadata retrieval.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_COMPONENTS_SD_CARD_DRIVER_SD_CARD_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_COMPONENTS_SD_CARD_DRIVER_SD_CARD_HPP_

#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace arcos::abstraction{

  /** SD card initialization result codes */
  enum class SdCardResult{
    Success = 0,          ///< Operation completed successfully
    InitError,            ///< Failed to initialize SPI bus
    MountError,           ///< Failed to mount filesystem
    NotInitialized,       ///< SD card not initialized
    FileNotFound,         ///< File does not exist
    FileOpenError,        ///< Failed to open file
    FileReadError,        ///< Failed to read from file
    FileWriteError,       ///< Failed to write to file
    DirectoryError,       ///< Directory operation failed
    InvalidParameter,     ///< Invalid parameter provided
    InsufficientSpace,    ///< Insufficient storage space
    CardNotPresent        ///< SD card not detected
  };

  /** SD card metadata */
  struct SdCardInfo{
    char name[16];        ///< Card name (CID)
    uint64_t capacity_mb; ///< Total capacity in megabytes
    uint64_t free_mb;     ///< Free space in megabytes
    uint32_t sector_size; ///< Sector size in bytes
    uint8_t  type;        ///< Card type (SDSC, SDHC, SDXC)
    bool     is_mounted;  ///< Mount status
  };

  /** File metadata */
  struct FileInfo{
    char name[256];       ///< File name
    uint64_t size;        ///< File size in bytes
    bool is_directory;    ///< True if directory
    uint32_t modified;    ///< Last modification timestamp
  };

  /** SD card driver configuration */
  struct SdCardConfig{
    uint8_t  spi_host;         ///< SPI host (SPI2_HOST, SPI3_HOST)
    uint8_t  pin_mosi;         ///< MOSI GPIO pin
    uint8_t  pin_miso;         ///< MISO GPIO pin
    uint8_t  pin_sck;          ///< SCK GPIO pin
    uint8_t  pin_cs;           ///< Chip select GPIO pin
    uint32_t max_frequency_hz; ///< Maximum SPI frequency in Hz
    uint8_t  max_open_files;   ///< Maximum number of open files
    bool     format_if_failed; ///< Format card if mount fails
    const char* mount_point;   ///< Mount point path (default: "/sdcard")
  };

  /** SD card driver class */
  class DRIVER_SD_CARD{
  public:
    /** Constructor */
    DRIVER_SD_CARD();
    
    /** Destructor */
    ~DRIVER_SD_CARD();
    
    // ============== INITIALIZATION ==============
    
    /**
     * @brief Initialize SD card with configuration
     * @param config SD card configuration
     * @return SdCardResult indicating success or failure
     */
    SdCardResult initialize(const SdCardConfig& config);
    
    /**
     * @brief Initialize SD card with default configuration
     * @return SdCardResult indicating success or failure
     */
    SdCardResult initialize();
    
    /**
     * @brief Deinitialize and unmount SD card
     * @return SdCardResult indicating success or failure
     */
    SdCardResult deinitialize();
    
    /**
     * @brief Check if SD card is initialized and mounted
     * @return True if initialized
     */
    bool isInitialized() const { return initialized_; }
    
    // ============== CARD INFORMATION ==============
    
    /**
     * @brief Get SD card metadata
     * @param info Pointer to store card information
     * @return SdCardResult indicating success or failure
     */
    SdCardResult getCardInfo(SdCardInfo* info);
    
    /**
     * @brief Get free space on SD card
     * @param free_bytes Pointer to store free space in bytes
     * @return SdCardResult indicating success or failure
     */
    SdCardResult getFreeSpace(uint64_t* free_bytes);
    
    /**
     * @brief Get total capacity of SD card
     * @param total_bytes Pointer to store total capacity in bytes
     * @return SdCardResult indicating success or failure
     */
    SdCardResult getTotalSpace(uint64_t* total_bytes);
    
    // ============== FILE OPERATIONS ==============
    
    /**
     * @brief Read entire file into buffer
     * @param filepath Path to file (relative to mount point)
     * @param buffer Buffer to store file contents
     * @param buffer_size Size of buffer
     * @param bytes_read Pointer to store actual bytes read
     * @return SdCardResult indicating success or failure
     */
    SdCardResult readFile(const char* filepath, 
                         uint8_t* buffer, 
                         size_t buffer_size,
                         size_t* bytes_read);
    
    /**
     * @brief Write data to file (overwrites if exists)
     * @param filepath Path to file (relative to mount point)
     * @param data Data to write
     * @param data_size Size of data in bytes
     * @return SdCardResult indicating success or failure
     */
    SdCardResult writeFile(const char* filepath,
                          const uint8_t* data,
                          size_t data_size);
    
    /**
     * @brief Append data to file (creates if doesn't exist)
     * @param filepath Path to file (relative to mount point)
     * @param data Data to append
     * @param data_size Size of data in bytes
     * @return SdCardResult indicating success or failure
     */
    SdCardResult appendFile(const char* filepath,
                           const uint8_t* data,
                           size_t data_size);
    
    /**
     * @brief Delete file
     * @param filepath Path to file (relative to mount point)
     * @return SdCardResult indicating success or failure
     */
    SdCardResult deleteFile(const char* filepath);
    
    /**
     * @brief Check if file exists
     * @param filepath Path to file (relative to mount point)
     * @param exists Pointer to store result
     * @return SdCardResult indicating success or failure
     */
    SdCardResult fileExists(const char* filepath, bool* exists);
    
    /**
     * @brief Get file size
     * @param filepath Path to file (relative to mount point)
     * @param size Pointer to store file size
     * @return SdCardResult indicating success or failure
     */
    SdCardResult getFileSize(const char* filepath, uint64_t* size);
    
    /**
     * @brief Rename or move file
     * @param old_path Current file path
     * @param new_path New file path
     * @return SdCardResult indicating success or failure
     */
    SdCardResult renameFile(const char* old_path, const char* new_path);
    
    // ============== DIRECTORY OPERATIONS ==============
    
    /**
     * @brief Create directory
     * @param dirpath Path to directory
     * @return SdCardResult indicating success or failure
     */
    SdCardResult createDirectory(const char* dirpath);
    
    /**
     * @brief Remove directory (must be empty)
     * @param dirpath Path to directory
     * @return SdCardResult indicating success or failure
     */
    SdCardResult removeDirectory(const char* dirpath);
    
    /**
     * @brief List files in directory
     * @param dirpath Path to directory
     * @param files Array to store file information
     * @param max_files Maximum number of files to list
     * @param file_count Pointer to store actual file count
     * @return SdCardResult indicating success or failure
     */
    SdCardResult listDirectory(const char* dirpath,
                              FileInfo* files,
                              size_t max_files,
                              size_t* file_count);
    
    /**
     * @brief Check if directory exists
     * @param dirpath Path to directory
     * @param exists Pointer to store result
     * @return SdCardResult indicating success or failure
     */
    SdCardResult directoryExists(const char* dirpath, bool* exists);
    
    // ============== UTILITY FUNCTIONS ==============
    
    /**
     * @brief Format SD card (WARNING: Deletes all data)
     * @return SdCardResult indicating success or failure
     */
    SdCardResult formatCard();
    
    /**
     * @brief Get mount point path
     * @return Mount point string
     */
    const char* getMountPoint() const { return mount_point_; }
    
  private:
    bool initialized_;           ///< Initialization status
    void* card_handle_;          ///< Internal card handle (sdmmc_card_t*)
    char mount_point_[32];       ///< Mount point path
    SdCardConfig config_;        ///< Current configuration
    
    /**
     * @brief Build absolute file path from relative path
     * @param filepath Relative file path
     * @param buffer Buffer to store absolute path
     * @param buffer_size Size of buffer
     */
    void buildAbsolutePath(const char* filepath, char* buffer, size_t buffer_size);
  };

} // namespace arcos::abstraction

// Include implementation
#include "driver_sd_card_impl.hpp"

#endif // ARCOS_ABSTRACTION_DRIVERS_COMPONENTS_SD_CARD_DRIVER_SD_CARD_HPP_
