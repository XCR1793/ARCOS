#pragma once

#include <cstdint>
#include <cstddef>
#include "hal_dma_buffer.hpp"  // Renamed from dma_buffer_manager.hpp

/**
 * @brief Parallel Buffer Management Class
 * 
 * This class manages DMA-capable buffers used with the LCD parallel interface 
 * for pattern generation. Implements the IDmaBufferManager interface for
 * flexible buffer management strategies.
 */
class ParallelBuffer : public IDmaBufferManager {
public:
  /**
   * @brief Constructor
   */
  ParallelBuffer();

  /**
   * @brief Destructor - automatically frees allocated buffer
   */
  ~ParallelBuffer();

  /**
   * @brief Allocate DMA-capable buffer for parallel output (legacy)
   * @param sample_count Number of 16-bit samples to allocate
   * @return true if allocation successful, false otherwise
   */
  bool alloc(size_t sample_count);
  
  /**
   * @brief Initialize the buffer manager (IDmaBufferManager interface)
   * @param config Buffer configuration
   * @return true if initialization successful, false otherwise
   */
  bool init(const DmaBufferConfig& config) override;
  
  /**
   * @brief Allocate DMA-capable buffers (IDmaBufferManager interface)
   * @param sample_count Number of 16-bit samples per buffer
   * @return true if allocation successful, false otherwise
   */
  bool allocate(size_t sample_count) override;

  /**
   * @brief Free the allocated DMA buffer
   */
  void free() override;

  /**
   * @brief Get pointer to the allocated buffer
   * @return Pointer to buffer, or nullptr if not allocated
   */
  uint16_t* getBuffer() const;

  /**
   * @brief Get size of the allocated buffer
   * @return Number of samples in buffer, or 0 if not allocated
   */
  size_t getSize() const;

  /**
   * @brief Set external buffer pointer directly (zero-copy, legacy)
   * @param external_buffer Pointer to external DMA-capable buffer
   * @param size Number of samples in the external buffer
   * @return true if pointer set successfully, false if invalid parameters
   * @note This does NOT take ownership - caller must manage memory
   */
  bool setDirectPointer(uint16_t* external_buffer, size_t size);
  
  /**
   * @brief Set external buffer pointer (IDmaBufferManager interface)
   * @param buffer_index Index of buffer slot
   * @param external_buffer Pointer to external DMA-capable buffer
   * @param size Number of samples in the external buffer
   * @return true if pointer set successfully, false if invalid parameters
   */
  bool setExternalBuffer(size_t buffer_index, uint16_t* external_buffer, size_t size) override;

  /**
   * @brief Get direct write access to buffer for fast updates
   * @return Pointer to buffer for direct writing, nullptr if not allocated
   * @note Use with caution - no bounds checking
   */
  uint16_t* getDirectAccess() const;

  /**
   * @brief Fill buffer with a repeating pattern (legacy, single buffer)
   * @param high_samples Number of samples with high_value in each cycle
   * @param low_samples Number of samples with low_value in each cycle
   * @param high_value Value to use for high portion (default: 0x3FFF)
   * @param low_value Value to use for low portion (default: 0x0000)
   * @return true if pattern created successfully, false if parameters invalid
   */
  bool fillPattern(size_t high_samples, size_t low_samples,
                   uint16_t high_value = 0x3FFF, uint16_t low_value = 0x0000);
  
  /**
   * @brief Fill a specific buffer with a pattern (IDmaBufferManager interface)
   * @param buffer_index Index of buffer to fill
   * @param high_samples Number of samples with high_value in each cycle
   * @param low_samples Number of samples with low_value in each cycle
   * @param high_value Value to use for high portion
   * @param low_value Value to use for low portion
   * @return true if pattern created successfully, false if parameters invalid
   */
  bool fillPattern(size_t buffer_index, size_t high_samples, size_t low_samples,
                   uint16_t high_value = 0x3FFF, uint16_t low_value = 0x0000) override;

  /**
   * @brief Fill buffer with solid value (legacy, single buffer)
   * @param value Value to fill buffer with
   */
  void fillSolid(uint16_t value);
  
  /**
   * @brief Fill a specific buffer with a value (IDmaBufferManager interface)
   * @param buffer_index Index of buffer to fill
   * @param value Value to fill buffer with
   * @return true if fill successful, false if index invalid
   */
  bool fillBuffer(size_t buffer_index, uint16_t value) override;

  /**
   * @brief Create timing pattern based on desired duration and frequency
   * @param high_duration_ms Duration in milliseconds for high state
   * @param low_duration_ms Duration in milliseconds for low state
   * @param sample_rate_hz Sample rate in Hz for timing calculations
   * @param high_value Value to use for high portion (default: 0x3FFF)
   * @param low_value Value to use for low portion (default: 0x0000)
   * @return true if timing pattern created successfully, false if parameters invalid
   */
  bool createTiming(uint32_t high_duration_ms, uint32_t low_duration_ms,
                    uint32_t sample_rate_hz, uint16_t high_value = 0x3FFF, 
                    uint16_t low_value = 0x0000);
  
  /** IDmaBufferManager interface implementations */
  uint16_t* getFrontBuffer() const override;
  uint16_t* getBackBuffer() const override;
  uint16_t* getBuffer(size_t index) const override;
  size_t getBufferCount() const override;
  size_t getBufferSize() const override;
  bool swapBuffers() override;
  BufferMode getMode() const override;
  bool isAllocated() const override;

private:
  uint16_t** buffers;      ///< Array of buffer pointers
  size_t num_buffers;      ///< Number of buffers
  size_t buffer_size;      ///< Size of each buffer in samples
  size_t front_index;      ///< Index of front buffer (for double buffering)
  size_t back_index;       ///< Index of back buffer (for double buffering)
  BufferMode mode;         ///< Current buffer mode
  bool* owns_buffer;       ///< Array indicating which buffers we own
  bool initialized;        ///< Whether the manager is initialized
  
  /** Legacy single buffer support */
  uint16_t* buffer;        ///< Pointer to allocated DMA buffer (legacy)
  bool legacy_owns_buffer; ///< True if we own the buffer memory (legacy)
};