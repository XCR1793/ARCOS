#pragma once

#include <cstdint>
#include <cstddef>

/**
 * @brief Parallel Buffer Management Class
 * 
 * This class provides static methods for managing DMA-capable buffers
 * used with the LCD parallel interface for pattern generation.
 */
class ParallelBuffer{
public:
  /**
   * @brief Allocate DMA-capable buffer for parallel output
   * @param sample_count Number of 16-bit samples to allocate
   * @return Pointer to allocated buffer, or nullptr if allocation failed
   */
  static uint16_t* alloc(size_t sample_count);

  /**
   * @brief Free previously allocated DMA buffer
   * @param buffer Pointer to buffer allocated with alloc()
   */
  static void free(uint16_t* buffer);

  /**
   * @brief Fill buffer with a repeating pattern
   * @param buffer Pointer to buffer to fill
   * @param sample_count Total number of samples in buffer
   * @param high_samples Number of samples with high_value in each cycle
   * @param low_samples Number of samples with low_value in each cycle
   * @param high_value Value to use for high portion (default: 0x3FFF)
   * @param low_value Value to use for low portion (default: 0x0000)
   * @return true if pattern created successfully, false if parameters invalid
   */
  static bool fillPattern(uint16_t* buffer, size_t sample_count,
                         size_t high_samples, size_t low_samples,
                         uint16_t high_value = 0x3FFF, uint16_t low_value = 0x0000);

  /**
   * @brief Fill buffer with solid value
   * @param buffer Pointer to buffer to fill
   * @param sample_count Number of samples to fill
   * @param value Value to fill buffer with
   */
  static void fillSolid(uint16_t* buffer, size_t sample_count, uint16_t value);

  /**
   * @brief Create timing pattern based on desired duration and frequency
   * @param buffer Pointer to buffer to fill
   * @param sample_count Total number of samples in buffer
   * @param high_duration_ms Duration in milliseconds for high state
   * @param low_duration_ms Duration in milliseconds for low state
   * @param sample_rate_hz Sample rate in Hz for timing calculations
   * @param high_value Value to use for high portion (default: 0x3FFF)
   * @param low_value Value to use for low portion (default: 0x0000)
   * @return true if timing pattern created successfully, false if parameters invalid
   */
  static bool createTiming(uint16_t* buffer, size_t sample_count,
                          uint32_t high_duration_ms, uint32_t low_duration_ms,
                          uint32_t sample_rate_hz, uint16_t high_value = 0x3FFF, 
                          uint16_t low_value = 0x0000);

private:
  ParallelBuffer() = delete;  // Static class, no instances
};