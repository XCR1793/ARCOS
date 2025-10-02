/*****************************************************************
 * File:      driver_hub75_protocol.hpp
 * Category:  abstraction/drivers/components/HUB75
 * 
 * Purpose:    Protocol interface for HUB75 display transmission
 *             Allows different protocol implementations (I2S, GPIO, etc.)
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_DRIVER_HUB75_PROTOCOL_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_DRIVER_HUB75_PROTOCOL_HPP_

#include <stdint.h>

namespace arcos::abstraction::drivers{

/** Forward declaration of HUB75Config */
struct HUB75Config;

/** Abstract interface for HUB75 protocol implementations
 * 
 * This interface separates the protocol-specific transmission logic from
 * the display buffer management and composition. Different implementations
 * can handle transmission via I2S, GPIO bit-banging, LCD_CAM, etc.
 * 
 * The protocol receives a pointer to the composed HUB75 buffer and
 * handles the transmission to the physical display hardware.
 */
class IHUB75Protocol{
public:
  virtual ~IHUB75Protocol() = default;
  
  /** Initialize the protocol handler with configuration
   * @param config Display configuration
   * @param buffer_size Size of the HUB75 buffer in samples
   * @return true if initialization successful
   */
  virtual bool init(const HUB75Config& config, int buffer_size) = 0;
  
  /** Start continuous transmission
   * @return true if transmission started successfully
   */
  virtual bool start() = 0;
  
  /** Stop transmission */
  virtual void stop() = 0;
  
  /** Check if protocol is initialized */
  virtual bool isInitialized() const = 0;
  
  /** Check if transmission is running */
  virtual bool isRunning() const = 0;
  
  /** Set the buffer to transmit (does not block, just updates pointer)
   * @param buffer Pointer to HUB75-formatted buffer
   * @param size Size of buffer in samples
   * @return true if buffer was accepted
   */
  virtual bool setBuffer(const uint16_t* buffer, int size) = 0;
  
  /** Swap to a new buffer for the next transmission cycle
   * @param buffer Pointer to new HUB75-formatted buffer (can be nullptr if using getWritableBuffer)
   * @param size Size of buffer in samples (can be 0 if using getWritableBuffer)
   * @return true if swap was successful
   */
  virtual bool swapBuffer(const uint16_t* buffer, int size) = 0;
  
  /** Get writable buffer for direct writes (back buffer in double buffering)
   * @return Pointer to writable buffer, or nullptr if not supported
   * @note Driver should write directly to this buffer, then call swapBuffer(nullptr, 0)
   */
  virtual uint16_t* getWritableBuffer() = 0;
  
  /** Get protocol backend name for debugging */
  virtual const char* getBackendName() const = 0;
};

} // namespace arcos::abstraction::drivers

#endif // ARCOS_ABSTRACTION_DRIVERS_DRIVER_HUB75_PROTOCOL_HPP_
