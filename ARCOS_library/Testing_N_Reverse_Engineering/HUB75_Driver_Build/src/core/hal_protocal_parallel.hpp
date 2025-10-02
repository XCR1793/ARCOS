/*****************************************************************
 * File:      hal_protocal_parallel.hpp
 * Category:  abstraction/core
 * 
 * Purpose:    Parallel hardware protocol abstraction interface
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_CORE_HAL_PROTOCAL_PARALLEL_HPP_
#define ARCOS_ABSTRACTION_CORE_HAL_PROTOCAL_PARALLEL_HPP_

#include <cstdint>
#include <cstddef>
#include "platform_hal.hpp"

namespace arcos::abstraction{
  namespace parallel{

/**
 * @brief Configuration structure for parallel hardware interface
 * 
 * This structure contains common configuration parameters that apply
 * to various parallel output peripherals (LCD_CAM, I2S, etc.)
 */
struct ParallelHardwareConfig {
  uint32_t clock_freq_hz{20000000};    ///< Target clock frequency in Hz
  bool invert_clock{false};            ///< Invert clock polarity
  bool continuous_mode{true};          ///< Enable continuous looping mode
  uint8_t data_width{14};              ///< Number of data pins to use
  PinNumber clock_pin{PIN_NC};         ///< External clock output pin
  
  /** Pin mapping for data lines */
  PinNumber* data_pins{nullptr};       ///< Array of GPIO pins for data lines
  size_t data_pin_count{0};            ///< Number of data pins
};

/**
 * @brief Abstract base class for parallel hardware interfaces
 * 
 * This interface defines the common operations that any parallel output
 * peripheral must support. Concrete implementations can use different
 * hardware backends (LCD_CAM, I2S, etc.)
 */
class IParallelHardware {
public:
  virtual ~IParallelHardware() = default;
  
  /**
   * @brief Initialize the hardware interface
   * @param data_pins Array of GPIO pins for data lines
   * @param config Configuration structure
   * @return true if initialization successful, false otherwise
   */
  virtual bool init(const PinNumber* data_pins, const ParallelHardwareConfig& config) = 0;
  
  /**
   * @brief Set buffer for DMA transfer
   * @param buffer Pointer to 16-bit sample buffer
   * @param buffer_len Number of samples in buffer
   * @return true if buffer set successfully, false otherwise
   */
  virtual bool setBuffer(uint16_t* buffer, size_t buffer_len) = 0;
  
  /**
   * @brief Set direct buffer pointer (zero-copy, high-speed)
   * @param buffer_ptr Pointer to external DMA-capable buffer
   * @param buffer_len Number of samples in buffer
   * @return true if pointer set successfully, false otherwise
   */
  virtual bool setDirectBuffer(uint16_t* buffer_ptr, size_t buffer_len) = 0;
  
  /**
   * @brief Swap buffer pointer seamlessly without stopping transmission
   * @param new_buffer_ptr Pointer to new DMA-capable buffer
   * @param buffer_len Number of samples in buffer
   * @return true if swap successful, false otherwise
   */
  virtual bool swapBuffer(uint16_t* new_buffer_ptr, size_t buffer_len) = 0;
  
  /**
   * @brief Get direct access to current buffer
   * @return Pointer to current buffer, nullptr if not set
   */
  virtual uint16_t* getDirectBuffer() const = 0;
  
  /**
   * @brief Get current buffer size
   * @return Number of samples in current buffer, 0 if not set
   */
  virtual size_t getBufferSize() const = 0;
  
  /**
   * @brief Start DMA transfer
   * @return true if started successfully, false otherwise
   */
  virtual bool start() = 0;
  
  /**
   * @brief Stop DMA transfer
   */
  virtual void stop() = 0;
  
  /**
   * @brief Check if interface is running
   * @return true if currently running, false otherwise
   */
  virtual bool isRunning() const = 0;
  
  /**
   * @brief Get current configuration
   * @return Pointer to current configuration, or nullptr if not initialized
   */
  virtual const ParallelHardwareConfig* getConfig() const = 0;
  
  /**
   * @brief Get the hardware backend type name
   * @return String describing the hardware backend (e.g., "LCD_CAM", "I2S")
   */
  virtual const char* getBackendName() const = 0;
};

  } // namespace parallel
} // namespace arcos::abstraction

#endif // ARCOS_ABSTRACTION_CORE_HAL_PROTOCAL_PARALLEL_HPP_
