/*****************************************************************
 * File:      driver_hub75_simple.hpp
 * Category:  abstraction/drivers/components/HUB75
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Simplified HUB75 LED matrix display driver with automatic
 *    initialization - just include and start drawing!
 *    
 *    This wrapper handles all the boilerplate:
 *    - Hardware platform injection
 *    - Buffer manager creation
 *    - Protocol initialization
 *    - Driver setup
 *    
 * Usage:
 *    #include "abstraction/drivers/components/HUB75/driver_hub75_simple.hpp"
 *    
 *    SimpleHUB75Display display;
 *    display.begin();  // That's it!
 *    display.setPixel(10, 10, RGB(255, 0, 0));
 *    display.show();
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_DRIVER_HUB75_SIMPLE_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_DRIVER_HUB75_SIMPLE_HPP_

#include "driver_hub75.hpp"
#include "driver_hub75_i2s.hpp"
#include "abstraction/hal.hpp"

namespace arcos::abstraction::drivers{

using namespace arcos::abstraction;

/** Simple HUB75 display wrapper with automatic initialization
 * 
 * This class wraps all the complexity of HUB75 initialization:
 * - Creates hardware platform objects
 * - Initializes I2S protocol
 * - Sets up the display driver
 * - Manages memory automatically
 * 
 * Just create an instance and call begin()!
 */
class SimpleHUB75Display{
public:
  SimpleHUB75Display() 
    : hardware_(nullptr)
    , buffer_manager_(nullptr)
    , protocol_(nullptr)
    , driver_(nullptr)
    , initialized_(false)
  {}
  
  ~SimpleHUB75Display(){
    end();
  }
  
  /** Initialize display with default or custom configuration
   * @param dual_oe Enable dual OE mode (uses oe_pin and oe_pin2 for parallel panel control)
   * @param config Optional custom configuration (uses default if not provided)
   * @return true if initialization successful
   * @note Dual OE mode controls 2 panels via separate Output Enable pins (parallel addressing)
   *       For chained displays, use config.expansion_mode instead
   */
  bool begin(bool dual_oe = true, HUB75Config config = HUB75Config::getDefault()){
    if(initialized_){
      return true;  // Already initialized
    }
    
    // Apply dual OE mode (parallel panel control via oe_pin and oe_pin2)
    config.dual_display_mode = dual_oe;
    config.effective_width = dual_oe ? 128 : 64;
    
    // Set second OE pin for dual panel control
    if(dual_oe){
      config.pins.oe_pin2 = 6;  // Secondary Output Enable for panel 1
    }else{
      config.pins.oe_pin2 = PIN_NC;  // Disable second OE for single panel
    }
    
    // Apply common sensible defaults
    config.enable_gamma_correction = true;
    config.gamma_value = 2.2f;
    config.panel_inversions[0].flip_vertical = true;
    config.panel_inversions[1].flip_vertical = false;
    
    // Store configuration
    this->config_ = config;
    
    // Create hardware platform objects
    hardware_ = new HAL_PARALLEL_DEFAULT();
    buffer_manager_ = new ParallelBuffer();
    protocol_ = new HUB75_I2S_Protocol();
    driver_ = new HUB75Driver();
    
    // Calculate buffer size
    int buffer_size = HUB75Driver::calculateBufferSize(config);
    
    // Initialize I2S protocol with hardware dependencies
    if(!protocol_->init(config, buffer_size, hardware_, buffer_manager_)){
      end();
      return false;
    }
    
    // Initialize display driver
    if(!driver_->init(config, protocol_)){
      end();
      return false;
    }
    
    // Start display
    if(!driver_->start()){
      end();
      return false;
    }
    
    initialized_ = true;
    return true;
  }
  
  /** Shutdown display and free resources */
  void end(){
    if(driver_){
      driver_->stop();
      delete driver_;
      driver_ = nullptr;
    }
    
    if(protocol_){
      delete protocol_;
      protocol_ = nullptr;
    }
    
    if(buffer_manager_){
      delete buffer_manager_;
      buffer_manager_ = nullptr;
    }
    
    if(hardware_){
      delete hardware_;
      hardware_ = nullptr;
    }
    
    initialized_ = false;
  }
  
  /** Check if display is ready */
  bool isReady() const { return initialized_; }
  
  /** Drawing functions - direct pass-through to driver */
  
  void setPixel(int x, int y, const RGB& color){
    if(driver_) driver_->setPixel(x, y, color);
  }
  
  RGB getPixel(int x, int y) const{
    return driver_ ? driver_->getPixel(x, y) : RGB(0, 0, 0);
  }
  
  void clear(){
    if(driver_) driver_->clear();
  }
  
  void fill(const RGB& color){
    if(driver_) driver_->fill(color);
  }
  
  void show(){
    if(driver_) driver_->show();
  }
  
  /** Brightness control (0-255) */
  void setBrightness(uint8_t brightness){
    if(driver_) driver_->setBrightness(brightness);
  }
  
  uint8_t getBrightness() const{
    return driver_ ? driver_->getBrightness() : 0;
  }
  
  /** Display dimensions */
  int getWidth() const{
    return driver_ ? driver_->getWidth() : 0;
  }
  
  int getHeight() const{
    return driver_ ? driver_->getHeight() : 0;
  }
  
  /** Get current configuration */
  const HUB75Config& getConfig() const{
    return config_;
  }
  
  /** Access to underlying driver for advanced operations */
  HUB75Driver* getDriver() { return driver_; }
  const HUB75Driver* getDriver() const { return driver_; }

private:
  /** Hardware components (managed automatically) */
  HAL_PARALLEL_DEFAULT* hardware_;
  ParallelBuffer* buffer_manager_;
  HUB75_I2S_Protocol* protocol_;
  HUB75Driver* driver_;
  
  /** Configuration */
  HUB75Config config_;
  bool initialized_;
  
  /** Prevent copying */
  SimpleHUB75Display(const SimpleHUB75Display&) = delete;
  SimpleHUB75Display& operator=(const SimpleHUB75Display&) = delete;
};

/** Quick-start configuration templates */
namespace hub75_presets{
  /** Single 64x32 panel */
  inline HUB75Config singlePanel(){
    HUB75Config config = HUB75Config::getDefault();
    config.dual_display_mode = false;
    config.effective_width = 64;
    config.enable_gamma_correction = true;
    config.gamma_value = 2.2f;
    return config;
  }
  
  /** Dual 64x32 panels side-by-side (128x32) */
  inline HUB75Config dualPanelHorizontal(){
    HUB75Config config = HUB75Config::getDefault();
    config.dual_display_mode = true;
    config.effective_width = 128;
    config.enable_gamma_correction = true;
    config.gamma_value = 2.2f;
    config.panel_inversions[0].flip_vertical = true;
    config.panel_inversions[1].flip_vertical = false;
    return config;
  }
  
  /** High brightness preset */
  inline HUB75Config highBrightness(){
    HUB75Config config = dualPanelHorizontal();
    config.colour_depth = 8;  // Maximum color depth
    config.enable_gamma_correction = false;  // Disable for max brightness
    return config;
  }
  
  /** Low power preset */
  inline HUB75Config lowPower(){
    HUB75Config config = dualPanelHorizontal();
    config.colour_depth = 4;  // Reduced color depth
    config.clock_freq_hz = 5000000;  // Lower clock speed
    return config;
  }
}

} // namespace arcos::abstraction::drivers

#endif // ARCOS_ABSTRACTION_DRIVERS_DRIVER_HUB75_SIMPLE_HPP_
