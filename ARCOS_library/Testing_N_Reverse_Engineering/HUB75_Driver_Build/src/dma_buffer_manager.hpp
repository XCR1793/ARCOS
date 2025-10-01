#pragma once

#include <cstdint>
#include <cstddef>

/**
 * @brief Buffer mode enumeration
 */
enum class BufferMode {
  SINGLE_BUFFER,      ///< Single buffer (no buffering)
  DOUBLE_BUFFER,      ///< Double buffering (front/back swap)
  CIRCULAR_BUFFER     ///< Circular buffer (continuous loop)
};

/**
 * @brief DMA buffer configuration
 */
struct DmaBufferConfig {
  size_t buffer_count{2};        ///< Number of buffers (1=single, 2=double, etc.)
  size_t sample_count{0};        ///< Number of 16-bit samples per buffer
  BufferMode mode{BufferMode::DOUBLE_BUFFER};
  bool auto_allocate{true};      ///< Automatically allocate DMA memory
};

/**
 * @brief Abstract base class for DMA buffer management
 * 
 * This interface defines the operations needed for managing DMA buffers
 * used in parallel data transmission. Implementations can provide different
 * buffering strategies (single, double, circular, etc.)
 */
class IDmaBufferManager {
public:
  virtual ~IDmaBufferManager() = default;
  
  /**
   * @brief Initialize the buffer manager
   * @param config Buffer configuration
   * @return true if initialization successful, false otherwise
   */
  virtual bool init(const DmaBufferConfig& config) = 0;
  
  /**
   * @brief Allocate DMA-capable buffers
   * @param sample_count Number of 16-bit samples per buffer
   * @return true if allocation successful, false otherwise
   */
  virtual bool allocate(size_t sample_count) = 0;
  
  /**
   * @brief Free all allocated buffers
   */
  virtual void free() = 0;
  
  /**
   * @brief Get pointer to the front buffer (currently being transmitted)
   * @return Pointer to front buffer, nullptr if not allocated
   */
  virtual uint16_t* getFrontBuffer() const = 0;
  
  /**
   * @brief Get pointer to the back buffer (for writing/updating)
   * @return Pointer to back buffer, nullptr if not allocated or in single buffer mode
   */
  virtual uint16_t* getBackBuffer() const = 0;
  
  /**
   * @brief Get pointer to a specific buffer by index
   * @param index Buffer index (0 to buffer_count-1)
   * @return Pointer to buffer, nullptr if index invalid
   */
  virtual uint16_t* getBuffer(size_t index) const = 0;
  
  /**
   * @brief Get the number of buffers
   * @return Number of buffers managed
   */
  virtual size_t getBufferCount() const = 0;
  
  /**
   * @brief Get size of each buffer
   * @return Number of samples per buffer, 0 if not allocated
   */
  virtual size_t getBufferSize() const = 0;
  
  /**
   * @brief Swap front and back buffers (for double buffering)
   * @return true if swap successful, false if not in double buffer mode
   */
  virtual bool swapBuffers() = 0;
  
  /**
   * @brief Get current buffer mode
   * @return Current buffer mode
   */
  virtual BufferMode getMode() const = 0;
  
  /**
   * @brief Fill a specific buffer with a value
   * @param buffer_index Index of buffer to fill
   * @param value Value to fill buffer with
   * @return true if fill successful, false if index invalid
   */
  virtual bool fillBuffer(size_t buffer_index, uint16_t value) = 0;
  
  /**
   * @brief Fill a specific buffer with a pattern
   * @param buffer_index Index of buffer to fill
   * @param high_samples Number of samples with high_value in each cycle
   * @param low_samples Number of samples with low_value in each cycle
   * @param high_value Value to use for high portion
   * @param low_value Value to use for low portion
   * @return true if pattern created successfully, false if parameters invalid
   */
  virtual bool fillPattern(size_t buffer_index, size_t high_samples, size_t low_samples,
                          uint16_t high_value = 0x3FFF, uint16_t low_value = 0x0000) = 0;
  
  /**
   * @brief Check if buffers are allocated
   * @return true if buffers allocated, false otherwise
   */
  virtual bool isAllocated() const = 0;
  
  /**
   * @brief Set external buffer pointer (zero-copy mode)
   * @param buffer_index Index of buffer slot
   * @param external_buffer Pointer to external DMA-capable buffer
   * @param size Number of samples in the external buffer
   * @return true if pointer set successfully, false if invalid parameters
   * @note Does NOT take ownership - caller must manage memory
   */
  virtual bool setExternalBuffer(size_t buffer_index, uint16_t* external_buffer, size_t size) = 0;
};
