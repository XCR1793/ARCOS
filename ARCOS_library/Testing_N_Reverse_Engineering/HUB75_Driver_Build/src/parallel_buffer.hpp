#pragma once

#include <cstdint>
#include <cstddef>

/**
 * @brief Parallel Buffer Management Class
 * 
 * This class manages DMA-capable buffers used with the LCD parallel interface 
 * for pattern generation. Each instance manages its own buffer.
 */
class ParallelBuffer{
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
   * @brief Allocate DMA-capable buffer for parallel output
   * @param sample_count Number of 16-bit samples to allocate
   * @return true if allocation successful, false otherwise
   */
  bool alloc(size_t sample_count);

  /**
   * @brief Free the allocated DMA buffer
   */
  void free();

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
   * @brief Set external buffer pointer directly (zero-copy)
   * @param external_buffer Pointer to external DMA-capable buffer
   * @param size Number of samples in the external buffer
   * @return true if pointer set successfully, false if invalid parameters
   * @note This does NOT take ownership - caller must manage memory
   */
  bool setDirectPointer(uint16_t* external_buffer, size_t size);

  /**
   * @brief Get direct write access to buffer for fast updates
   * @return Pointer to buffer for direct writing, nullptr if not allocated
   * @note Use with caution - no bounds checking
   */
  uint16_t* getDirectAccess() const;

  /**
   * @brief Fill buffer with a repeating pattern
   * @param high_samples Number of samples with high_value in each cycle
   * @param low_samples Number of samples with low_value in each cycle
   * @param high_value Value to use for high portion (default: 0x3FFF)
   * @param low_value Value to use for low portion (default: 0x0000)
   * @return true if pattern created successfully, false if parameters invalid
   */
  bool fillPattern(size_t high_samples, size_t low_samples,
                   uint16_t high_value = 0x3FFF, uint16_t low_value = 0x0000);

  /**
   * @brief Fill buffer with solid value
   * @param value Value to fill buffer with
   */
  void fillSolid(uint16_t value);

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

private:
  uint16_t* buffer;     ///< Pointer to allocated DMA buffer
  size_t buffer_size;   ///< Size of buffer in samples
  bool owns_buffer;     ///< True if we own the buffer memory, false for external pointers
};