/*****************************************************************
 * File:      driver_hub75_i2s.hpp
 * Category:  abstraction/drivers/communication/HUB75
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    I2S protocol implementation for HUB75 display using parallel
 *    hardware interface with DMA buffer management.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_DRIVER_HUB75_I2S_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_DRIVER_HUB75_I2S_HPP_

#include "driver_hub75_protocol.hpp"
#include "driver_hub75.hpp"
#include "../../../core/hal_protocal_parallel.hpp"
#include "../../../core/hal_protocal_dma.hpp"

namespace arcos::abstraction::drivers{

// Use types from core namespaces
using parallel::IParallelHardware;
using parallel::ParallelHardwareConfig;
using dma::IDmaBufferManager;
using dma::DmaBufferConfig;
using dma::BufferMode;

/** I2S-based protocol implementation for HUB75
 * 
 * This implementation uses I2S parallel hardware (LCD_CAM on ESP32) to
 * transmit the HUB75 protocol buffer to the display. It manages the
 * hardware interface and DMA buffer manager.
 */
class HUB75_I2S_Protocol : public IHUB75Protocol{
public:
  HUB75_I2S_Protocol();
  ~HUB75_I2S_Protocol() override;
  
  /** Initialize with injected hardware and buffer dependencies
   * @param config Display configuration
   * @param buffer_size Size of the HUB75 buffer in samples
   * @param hardware Pointer to IParallelHardware implementation (REQUIRED)
   * @param buffer_manager Pointer to IDmaBufferManager implementation (REQUIRED)
   * @return true if initialization successful
   */
  bool init(const HUB75Config& config, int buffer_size, 
            IParallelHardware* hardware, IDmaBufferManager* buffer_manager);
  
  /** IHUB75Protocol interface implementation */
  bool init(const HUB75Config& config, int buffer_size) override;
  bool start() override;
  void stop() override;
  bool isInitialized() const override { return initialized; }
  bool isRunning() const override { return running; }
  bool setBuffer(const uint16_t* buffer, int size) override;
  bool swapBuffer(const uint16_t* buffer, int size) override;
  uint16_t* getWritableBuffer() override;
  const char* getBackendName() const override;
  
private:
  /** Hardware interfaces (abstraction layer) */
  IParallelHardware* hwInterface;      // Abstract hardware interface (LCD_CAM, I2S, etc.)
  IDmaBufferManager* bufferManager;    // Abstract buffer manager
  
  /** NOTE: Protocol does NOT own injected dependencies
   *  Application is responsible for lifecycle management */
  
  /** Buffer pointers */
  uint16_t* frontBuffer;
  uint16_t* backBuffer;
  
  /** Configuration */
  HUB75Config config;
  int buffer_size;
  
  /** State */
  bool initialized;
  bool running;
  bool external_init;  // Track if init() was called with external dependencies
};

} // namespace arcos::abstraction::drivers

// Include implementation
#include "driver_hub75_i2s_impl.hpp"

#endif // ARCOS_ABSTRACTION_DRIVERS_DRIVER_HUB75_I2S_HPP_

