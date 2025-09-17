#pragma once

#include <cstdint>
#include <cstddef>
#include "driver/gpio.h"
#include "hal/dma_types.h"
#include "esp_private/gdma.h"

/**
 * @brief Configuration structure for LCD parallel interface
 */
struct LcdParallelConfig{
  uint32_t clock_freq_hz{20000000};    ///< Target clock frequency in Hz (default: 20MHz)
  bool invert_clock{false};            ///< Invert clock polarity (default: false)
  bool continuous_mode{true};          ///< Enable continuous looping mode (default: true)
  uint8_t data_width{14};              ///< Number of data pins to use (default: 14, max: 16)
};

/**
 * @brief LCD Parallel Interface Class
 * 
 * This class provides an interface for controlling the ESP32-S3 LCD_CAM peripheral
 * in parallel mode with DMA support for high-speed GPIO pattern generation.
 */
class LcdParallel{
public:
  /**
   * @brief Constructor
   */
  LcdParallel();

  /**
   * @brief Destructor
   */
  ~LcdParallel();

  /**
   * @brief Get default configuration for LCD parallel interface
   * @return Default LcdParallelConfig structure with sensible defaults
   */
  static LcdParallelConfig getDefaultConfig();

  /**
   * @brief Initialize LCD parallel interface for 16-bit parallel output
   * @param data_pins Array of GPIO pins for data lines (must have 16 elements)
   * @param config Configuration structure with timing and mode settings
   * @return true if initialization successful, false otherwise
   */
  bool init(const gpio_num_t* data_pins, const LcdParallelConfig& config);

  /**
   * @brief Set buffer for LCD parallel DMA transfer
   * @param buffer Pointer to 16-bit sample buffer
   * @param buffer_len Number of samples in buffer
   * @return true if buffer set successfully, false otherwise
   */
  bool setBuffer(uint16_t* buffer, size_t buffer_len);

  /**
   * @brief Start LCD parallel DMA transfer
   * @return true if started successfully, false otherwise
   */
  bool start();

  /**
   * @brief Stop LCD parallel DMA transfer
   */
  void stop();

  /**
   * @brief Check if LCD parallel interface is running
   * @return true if currently running, false otherwise
   */
  bool isRunning() const;

  /**
   * @brief Get current configuration
   * @return Pointer to current configuration structure, or nullptr if not initialized
   */
  const LcdParallelConfig* getConfig() const;

private:
  /** DMA and LCD peripheral state */
  gdma_channel_handle_t dma_chan;
  dma_descriptor_t* dma_descriptors;
  size_t desc_count;
  bool initialized;
  bool running;
  LcdParallelConfig config;
  uint16_t* buffer;
  size_t buffer_len;
};