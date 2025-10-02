# HUB75 LED Matrix Driver for ESP32-S3

A high-performance, platform-abstracted HUB75 LED matrix driver with advanced features including BCM brightness control, dual panel support, and gamma correction.

## Features

- ✅ **Platform Abstraction Layer** - Easy to port to different MCUs
- ✅ **BCM (Binary Code Modulation)** - Smooth 5-bit color depth with 64-level brightness control
- ✅ **Dual Panel Support** - Independent control via dual OE pins (PARALLEL_OE mode)
- ✅ **Panel Inversion** - Hardware orientation correction (flip horizontal/vertical)
- ✅ **Gamma Correction** - Built-in 2.2 gamma correction for accurate colors
- ✅ **DMA-based** - High-speed transfers using ESP32-S3 LCD_CAM peripheral
- ✅ **Double Buffering** - Smooth, flicker-free updates
- ✅ **Multiple Expansion Modes** - SINGLE, PARALLEL_OE, SERIES_CHAIN

## Quick Start

### Hardware Setup

**Supported Hardware:**
- ESP32-S3 (using LCD_CAM peripheral)
- HUB75 RGB LED matrix panels (64x32 tested)
- Dual panel configuration (128x32 total)

**Pin Configuration:**
```cpp
config.pins.r0_pin = 7;        // Red 0
config.pins.g0_pin = 15;       // Green 0  
config.pins.b0_pin = 16;       // Blue 0
config.pins.r1_pin = 17;       // Red 1
config.pins.g1_pin = 18;       // Green 1
config.pins.b1_pin = 8;        // Blue 1
config.pins.a_pin = 41;        // Address A
config.pins.b_pin = 40;        // Address B
config.pins.c_pin = 39;        // Address C
config.pins.d_pin = 38;        // Address D
config.pins.e_pin = 42;        // Address E
config.pins.lat_pin = 36;      // Latch
config.pins.oe_pin = 35;       // Primary Output Enable
config.pins.oe_pin2 = 6;       // Secondary Output Enable (dual panel)
config.pins.clock_pin = 37;    // Clock
```

### Basic Usage

```cpp
#include "hub75_driver.hpp"

HUB75Driver display;

void app_main(){
  // Configure display
  HUB75Config config = HUB75Config::getDefault();
  config.enable_gamma_correction = true;
  config.gamma_value = 2.2f;
  config.dual_display_mode = true;
  config.effective_width = 128;  // 2x 64x32 panels
  
  // Initialize and start
  display.init(config);
  display.start();
  
  // Draw pixels
  display.setPixel(10, 10, RGB(255, 0, 0));  // Red pixel at (10,10)
  display.fillScreen(RGB(0, 0, 255));        // Fill with blue
  
  // Update display
  display.show();
}
```

## Documentation

- **[ARCHITECTURE.md](ARCHITECTURE.md)** - System architecture and design
- **[API_REFERENCE.md](API_REFERENCE.md)** - Complete API documentation
- **[BCM_PROTOCOL.md](BCM_PROTOCOL.md)** - BCM brightness control explained
- **[PLATFORM_ABSTRACTION.md](PLATFORM_ABSTRACTION.md)** - Porting to other platforms
- **[EXAMPLES.md](EXAMPLES.md)** - Code examples and patterns

## Project Structure

```
HUB75_Driver_Build/
├── src/
│   ├── hub75_driver.cpp/.hpp      # Main driver
│   ├── platform_hal.hpp            # Platform abstraction interface
│   ├── esp32_platform_impl.cpp/.hpp # ESP32 platform implementation
│   ├── lcd_parallel.cpp/.hpp       # ESP32 LCD_CAM driver
│   ├── parallel_buffer.cpp/.hpp    # Buffer management
│   ├── dma_buffer_manager.hpp      # DMA buffer interface
│   └── main.cpp                    # Demo application
├── include/                         # Platform-specific headers
├── README.md                        # This file
└── *.md                            # Additional documentation
```

## Building

**Platform:** ESP-IDF v5.x via PlatformIO

```bash
# Build
platformio run

# Build and upload
platformio run --target upload

# Monitor serial output
platformio device monitor --baud 115200
```

## Performance

- **Refresh Rate:** ~500Hz (flicker-free)
- **Color Depth:** 5-bit per channel (15-bit total RGB)
- **Brightness Levels:** 64 levels (0-255 mapped to 0-63)
- **Memory Usage:** ~260KB Flash, ~14KB RAM
- **BCM Timing:** Exponential (1, 2, 4, 8, 16 cycles per bit plane)

## License

[Your License Here]

## Contributing

Contributions welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Credits

Created for the ARCOS robotics library project.
