# ARCOS OLED Driver Example

This project demonstrates the ARCOS Hardware Abstraction Framework with an SH1107G OLED display driver implementation.

## Overview

This implementation showcases the ARCOS design principles:

- **Hardware Abstraction**: Uses ARCOS HAL for I2C communication
- **Modular Design**: Clean separation between interface and implementation
- **Zero Runtime Overhead**: Template-based abstractions compile away
- **Cross-Platform**: Can be easily ported to different microcontrollers
- **Type Safety**: Strong typing with compile-time verification

## Project Structure

```
├── abstraction/                    # ARCOS Hardware Abstraction Framework
│   ├── hal.hpp                    # Main HAL header
│   ├── core/                      # Core HAL interfaces
│   │   ├── hal_protocal_i2c.hpp   # I2C protocol abstraction
│   │   └── ...                    # Other core interfaces
│   ├── platforms/                 # Platform-specific implementations
│   │   └── esp32/wroom32s3/module/
│   │       ├── hal_connector.hpp  # ESP32-S3 platform connector
│   │       ├── hal_interface_i2c_module.hpp
│   │       └── ...
│   └── drivers/                   # Device drivers
│       └── components/
│           ├── OLED/              # OLED driver (NEW)
│           │   ├── driver_oled.hpp
│           │   └── driver_oled_impl.hpp
│           └── BME280/            # BME280 sensor driver
├── src/                           # Application source
│   ├── main.cpp                   # Original ESP-IDF example
│   ├── main_arcos_example.cpp     # ARCOS framework example
│   ├── i2c_driver.cpp/h           # Legacy ESP-IDF I2C driver
│   └── oled_driver.cpp/h          # Legacy ESP-IDF OLED driver
└── README.md                      # This file
```

## Hardware Requirements

- **ESP32-S3 Development Board**
- **1.5" 128x128 OLED Display** (SH1107G controller)
- **I2C Connections**:
  - SDA → GPIO 21
  - SCL → GPIO 22  
  - VCC → 3.3V
  - GND → GND

## ARCOS OLED Driver Features

### Core Functionality
- **Hardware Abstraction**: Uses ARCOS I2C HAL instead of direct ESP-IDF calls
- **Memory Management**: Dynamic buffer allocation with proper cleanup
- **Smart Updates**: Only updates changed display pages for better performance
- **Area Updates**: Update specific rectangular regions
- **Dirty Page Tracking**: Automatic optimization of display updates

### Drawing Operations
- `setPixel()` - Individual pixel control
- `drawLine()` - Bresenham line algorithm
- `drawRect()` - Filled and outline rectangles  
- `drawCircle()` - Filled and outline circles

### Display Control
- `displayOn()/displayOff()` - Power control
- `setContrast()` - Brightness adjustment
- `invertDisplay()` - Color inversion
- `clearBuffer()/fillBuffer()` - Buffer operations

### Update Modes
- `OLEDUpdateMode::Full` - Update entire display
- `OLEDUpdateMode::Smart` - Update only changed pages
- `OLEDUpdateMode::Area` - Update specific rectangular area
- `OLEDUpdateMode::Pages` - Update specific page range

## Usage Example

```cpp
#define TARGET_ESP32_Wroom32S3_Module
#include "abstraction/drivers/components/OLED/driver_oled.hpp"

using namespace arcos::abstraction;

void example_usage(){
  // Create driver instance
  DRIVER_OLED oled;
  
  // Initialize with default settings (0x3C address, bus 0)
  if(!oled.initialize()){
    ESP_LOGE("APP", "OLED init failed!");
    return;
  }
  
  // Draw some graphics
  oled.clearBuffer();
  oled.drawRect(10, 10, 50, 30, false, true);    // Rectangle outline
  oled.drawCircle(64, 64, 20, true, true);       // Filled circle
  oled.setPixel(100, 100, true);                 // Single pixel
  
  // Update display (smart mode - only changed pages)
  oled.updateDisplay(OLEDUpdateMode::Smart);
  
  // Custom configuration example
  OLEDConfig config;
  config.i2c_address = 0x3D;    // Different address
  config.contrast = 200;         // Higher contrast
  config.flip_horizontal = true; // Mirror horizontally
  
  DRIVER_OLED oled2(config);
  oled2.initialize(config);
}
```

## Comparison: Legacy vs ARCOS

### Legacy ESP-IDF Approach
```cpp
// Direct ESP-IDF calls, platform-specific
#include "driver/i2c.h"
esp_err_t ret = i2c_master_write_to_device(
    I2C_NUM_0, 0x3C, data, len, pdMS_TO_TICKS(1000));
```

### ARCOS Abstraction Approach  
```cpp
// Hardware-agnostic, cross-platform
#include "abstraction/hal.hpp"
HalResult result = ESP32S3_I2C::WriteBuffer(0, 0x3C, data, len);
```

## Key Benefits

1. **Portability**: Easy to port to STM32, Arduino, or other platforms
2. **Maintainability**: Clean interfaces and separation of concerns
3. **Testing**: Hardware abstraction enables unit testing
4. **Performance**: Zero runtime overhead through templates
5. **Type Safety**: Compile-time verification of hardware configurations
6. **Modularity**: Drivers can be included independently

## Build Instructions

1. **Set up ESP-IDF** (version 4.4 or later)
2. **Clone/copy this project**
3. **Configure target**: `idf.py set-target esp32s3`
4. **Build**: `idf.py build`
5. **Flash**: `idf.py flash monitor`

## Extending the Framework

To add a new sensor/device driver:

1. Create directory in `abstraction/drivers/components/YOUR_DEVICE/`
2. Create `driver_yourdevice.hpp` with interface
3. Create `driver_yourdevice_impl.hpp` with implementation
4. Follow ARCOS coding style guide
5. Use HAL abstractions (I2C, SPI, GPIO, etc.)

## Files Overview

### New ARCOS Files
- `abstraction/drivers/components/OLED/driver_oled.hpp` - OLED driver interface
- `abstraction/drivers/components/OLED/driver_oled_impl.hpp` - OLED implementation
- `src/main_arcos_example.cpp` - ARCOS framework usage example

### Legacy Files (for comparison)
- `src/i2c_driver.cpp/h` - ESP-IDF specific I2C driver
- `src/oled_driver.cpp/h` - ESP-IDF specific OLED driver  
- `src/main.cpp` - ESP-IDF approach example

## ARCOS Coding Style

The implementation follows the ARCOS coding style guide:

- **Tight braces**: `if(condition){`
- **camelCase**: Method names like `updateDisplay()`
- **snake_case**: Variables like `display_buffer_`
- **PascalCase**: Classes like `DRIVER_OLED`
- **2-space indentation**
- **Const correctness**
- **Clear ownership semantics**

This demonstrates a professional hardware abstraction framework suitable for production embedded systems.