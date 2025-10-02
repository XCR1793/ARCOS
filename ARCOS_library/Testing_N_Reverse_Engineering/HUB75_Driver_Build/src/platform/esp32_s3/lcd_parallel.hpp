#pragma once

#include <cstdint>
#include <cstddef>
#include "../../core/platform_hal.hpp"
#include "../../core/hal_parallel_interface.hpp"

/** Forward declarations for platform-specific types (opaque pointers) */
struct PlatformDmaChannel;
struct PlatformDmaDescriptor;

/**
 * @brief Configuration structure for LCD parallel interface
 * @deprecated Use ParallelHardwareConfig instead
 */
struct LcdParallelConfig{
  uint32_t clock_freq_hz{20000000};    ///< Target clock frequency in Hz (default: 20MHz)
  bool invert_clock{false};            ///< Invert clock polarity (default: false)
  bool continuous_mode{true};          ///< Enable continuous looping mode (default: true)
  uint8_t data_width{14};              ///< Number of data pins to use (default: 14, max: 16)
  PinNumber clock_pin{PIN_NC};         ///< External clock output pin (default: no external clock)
};

/**
 * @brief LCD Parallel Interface Class (LCD_CAM peripheral implementation)
 * 
 * This class provides an interface for controlling the ESP32-S3 LCD_CAM peripheral
 * in parallel mode with DMA support for high-speed GPIO pattern generation.
 * Implements the IParallelHardware interface for use with the HUB75 driver.
 */
class LcdParallel : public IParallelHardware {
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
   * @brief Initialize LCD parallel interface for 16-bit parallel output (legacy)
   * @param data_pins Array of GPIO pins for data lines (must have 16 elements)
   * @param config Configuration structure with timing and mode settings
   * @return true if initialization successful, false otherwise
   */
  bool init(const PinNumber* data_pins, const LcdParallelConfig& config);
  
  /**
   * @brief Initialize LCD parallel interface (IParallelHardware interface)
   * @param data_pins Array of GPIO pins for data lines
   * @param config Configuration structure with timing and mode settings
   * @return true if initialization successful, false otherwise
   */
  bool init(const PinNumber* data_pins, const ParallelHardwareConfig& config) override;

  /**
   * @brief Set buffer for LCD parallel DMA transfer
   * @param buffer Pointer to 16-bit sample buffer
   * @param buffer_len Number of samples in buffer
   * @return true if buffer set successfully, false otherwise
   */
  bool setBuffer(uint16_t* buffer, size_t buffer_len) override;

  /**
   * @brief Set direct buffer pointer (zero-copy, high-speed)
   * @param buffer_ptr Pointer to external DMA-capable buffer
   * @param buffer_len Number of samples in buffer
   * @return true if pointer set successfully, false otherwise
   * @note This is fastest method - no copying, direct DMA access
   */
  bool setDirectBuffer(uint16_t* buffer_ptr, size_t buffer_len) override;

  /**
   * @brief Swap buffer pointer seamlessly without stopping transmission
   * @param new_buffer_ptr Pointer to new DMA-capable buffer
   * @param buffer_len Number of samples in buffer (must match current buffer size)
   * @return true if swap successful, false otherwise
   * @note This updates DMA descriptors on-the-fly for seamless double buffering
   */
  bool swapBuffer(uint16_t* new_buffer_ptr, size_t buffer_len) override;

  /**
   * @brief Get direct access to current buffer for in-place updates
   * @return Pointer to current buffer, nullptr if not set
   * @note Use for fastest possible updates - modify buffer directly
   */
  uint16_t* getDirectBuffer() const override;

  /**
   * @brief Get current buffer size
   * @return Number of samples in current buffer, 0 if not set
   */
  size_t getBufferSize() const override;

  /**
   * @brief Start LCD parallel DMA transfer
   * @return true if started successfully, false otherwise
   */
  bool start() override;

  /**
   * @brief Stop LCD parallel DMA transfer
   */
  void stop() override;

  /**
   * @brief Check if LCD parallel interface is running
   * @return true if currently running, false otherwise
   */
  bool isRunning() const override;

  /**
   * @brief Get current configuration (legacy interface)
   * @return Pointer to current configuration structure, or nullptr if not initialized
   */
  const LcdParallelConfig* getLegacyConfig() const;
  
  /**
   * @brief Get current configuration (IParallelHardware interface)
   * @return Pointer to current configuration structure, or nullptr if not initialized
   */
  const ParallelHardwareConfig* getConfig() const override;
  
  /**
   * @brief Get the hardware backend type name
   * @return String describing the hardware backend
   */
  const char* getBackendName() const override { return "LCD_CAM"; }

private:
  /** DMA and LCD peripheral state (opaque pointers for platform abstraction) */
  PlatformDmaChannel* dma_chan;
  PlatformDmaDescriptor* dma_descriptors;
  size_t desc_count;
  bool initialized;
  bool running;
  LcdParallelConfig config;
  ParallelHardwareConfig hw_config;  // Config for interface compliance
  uint16_t* buffer;
  size_t buffer_len;
  
  /** Platform HAL reference */
  IPlatformHAL* platform;
};