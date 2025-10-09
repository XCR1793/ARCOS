#ifndef PANEL_CONFIG_HPP
#define PANEL_CONFIG_HPP

#include <cstdint>

// Panel configuration for ARCOS custom pinout
namespace PanelConfig {
  
  // Panel dimensions
  constexpr uint16_t PANEL_WIDTH = 64;
  constexpr uint16_t PANEL_HEIGHT = 32;
  constexpr uint8_t PANEL_SCAN = 16;  // 1/16 scan
  
  // Color depth configuration
  constexpr uint8_t COLOR_DEPTH_BITS = 8;  // 8 bits per color channel
  
  // Custom ESP32-S3 pin configuration for ARCOS
  struct PinConfig {
    // RGB1 pins (top half)
    static constexpr uint8_t R1 = 7;
    static constexpr uint8_t G1 = 15;
    static constexpr uint8_t B1 = 16;
    
    // RGB2 pins (bottom half)
    static constexpr uint8_t R2 = 17;
    static constexpr uint8_t G2 = 18;
    static constexpr uint8_t B2 = 8;
    
    // Address pins
    static constexpr uint8_t A = 41;
    static constexpr uint8_t B = 40;
    static constexpr uint8_t C = 39;
    static constexpr uint8_t D = 38;
    static constexpr uint8_t E = 42;
    
    // Control pins
    static constexpr uint8_t LAT = 36;
    static constexpr uint8_t OE = 35;
    static constexpr uint8_t CLK = 37;
  };
  
  // DMA buffer configuration
  constexpr uint32_t DMA_MAX_SIZE = 4092;  // Maximum DMA descriptor size
  constexpr uint32_t CLOCK_SPEED_HZ = 20000000;  // 20 MHz
  
  // Brightness configuration
  constexpr uint8_t DEFAULT_BRIGHTNESS = 128;  // 0-255
  constexpr uint8_t MIN_REFRESH_RATE = 60;     // Hz
  
}  // namespace PanelConfig

#endif  // PANEL_CONFIG_HPP
