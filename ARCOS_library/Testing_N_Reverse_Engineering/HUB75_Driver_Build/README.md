# HUB75 LED Matrix Driver - ARCOS Abstraction# HUB75 LED Matrix Driver for ESP32-S3



## OverviewA high-performance, platform-abstracted HUB75 LED matrix driver with advanced features including BCM brightness control, dual panel support, and gamma correction.



This project implements a modular, protocol-agnostic HUB75 LED matrix driver using the ARCOS hardware abstraction framework. The driver architecture separates buffer composition logic from protocol transmission, enabling multiple backend implementations (I2S, GPIO, SPI, etc.) through dependency injection.## Features



## Features- ✅ **Platform Abstraction Layer** - Easy to port to different MCUs

- ✅ **BCM (Binary Code Modulation)** - Smooth 5-bit color depth with 64-level brightness control

- **Protocol Abstraction**: Swap between I2S, GPIO, or custom transmission protocols- ✅ **Dual Panel Support** - Independent control via dual OE pins (PARALLEL_OE mode)

- **Dual Panel Support**: Parallel OE mode for independent panel control- ✅ **Panel Inversion** - Hardware orientation correction (flip horizontal/vertical)

- **Binary Code Modulation (BCM)**: 5-bit color depth with 64-level brightness control- ✅ **Gamma Correction** - Built-in 2.2 gamma correction for accurate colors

- **DMA Double Buffering**: Flicker-free updates with zero-copy direct buffer writes- ✅ **DMA-based** - High-speed transfers using ESP32-S3 LCD_CAM peripheral

- **Flexible Configuration**: Support for multiple expansion modes and panel inversions- ✅ **Double Buffering** - Smooth, flicker-free updates

- **ARCOS Compliant**: Clean abstraction layers with dependency injection- ✅ **Multiple Expansion Modes** - SINGLE, PARALLEL_OE, SERIES_CHAIN



## Hardware Support## Quick Start



- **Platform**: ESP32-S3 (ESP-IDF 5.5.0)### Hardware Setup

- **Display**: HUB75 RGB LED matrix panels

- **Tested Configuration**: Dual 64x32 panels with dual OE pins**Supported Hardware:**

- **Transmission**: I2S/LCD_CAM parallel interface at 10MHz- ESP32-S3 (using LCD_CAM peripheral)

- HUB75 RGB LED matrix panels (64x32 tested)

## Architecture- Dual panel configuration (128x32 total)



```**Pin Configuration:**

Application (main.cpp)```cpp

    ↓config.pins.r0_pin = 7;        // Red 0

HUB75Driver (driver_hub75.cpp)config.pins.g0_pin = 15;       // Green 0  

    ├── Manages framebuffer (RGB888)config.pins.b0_pin = 16;       // Blue 0

    ├── Pixel operations (setPixel, fill, clear)config.pins.r1_pin = 17;       // Red 1

    ├── BCM composition & conversionconfig.pins.g1_pin = 18;       // Green 1

    └── Protocol abstraction pointerconfig.pins.b1_pin = 8;        // Blue 1

        ↓config.pins.a_pin = 41;        // Address A

IHUB75Protocol Interface (driver_hub75_protocol.hpp)config.pins.b_pin = 40;        // Address B

    ↓config.pins.c_pin = 39;        // Address C

HUB75_I2S_Protocol (driver_hub75_i2s.cpp)config.pins.d_pin = 38;        // Address D

    ├── IParallelHardware (LCD_CAM interface)config.pins.e_pin = 42;        // Address E

    └── IDmaBufferManager (Double buffering)config.pins.lat_pin = 36;      // Latch

```config.pins.oe_pin = 35;       // Primary Output Enable

config.pins.oe_pin2 = 6;       // Secondary Output Enable (dual panel)

### Key Principlesconfig.pins.clock_pin = 37;    // Clock

```

1. **Separation of Concerns**

   - Driver: Manages framebuffer and composition logic### Basic Usage

   - Protocol: Handles hardware transmission

   ```cpp

2. **Dependency Injection**#include "hub75_driver.hpp"

   - Protocol implementation injected into driver

   - Hardware interfaces injected into protocolHUB75Driver display;

   

3. **Zero-Copy Design**void app_main(){

   - Driver writes directly to protocol's DMA back buffer  // Configure display

   - No intermediate buffer copies  HUB75Config config = HUB75Config::getDefault();

  config.enable_gamma_correction = true;

## Quick Start  config.gamma_value = 2.2f;

  config.dual_display_mode = true;

### 1. Initialize Hardware and Protocol  config.effective_width = 128;  // 2x 64x32 panels

  

```cpp  // Initialize and start

// Platform implementations  display.init(config);

HAL_PARALLEL_DEFAULT hardware;  display.start();

ParallelBuffer bufferManager;  

HUB75_I2S_Protocol i2sProtocol;  // Draw pixels

HUB75Driver display;  display.setPixel(10, 10, RGB(255, 0, 0));  // Red pixel at (10,10)

  display.fillScreen(RGB(0, 0, 255));        // Fill with blue

// Configure display  

HUB75Config config = HUB75Config::getDefault();  // Update display

config.matrix_width = 64;  display.show();

config.matrix_height = 32;}

config.expansion_mode = HUB75Config::ExpansionMode::PARALLEL_OE;```

config.panel_count = 2;

config.pins.oe_pin2 = 6;  // Enable dual OE## Documentation



// Calculate buffer size- **[ARCHITECTURE.md](ARCHITECTURE.md)** - System architecture and design

int buffer_size = HUB75Driver::calculateBufferSize(config);- **[API_REFERENCE.md](API_REFERENCE.md)** - Complete API documentation

- **[BCM_PROTOCOL.md](BCM_PROTOCOL.md)** - BCM brightness control explained

// Initialize protocol with hardware dependencies- **[PLATFORM_ABSTRACTION.md](PLATFORM_ABSTRACTION.md)** - Porting to other platforms

i2sProtocol.init(config, buffer_size, &hardware, &bufferManager);- **[EXAMPLES.md](EXAMPLES.md)** - Code examples and patterns



// Initialize driver with protocol## Project Structure

display.init(config, &i2sProtocol);

display.start();```

```HUB75_Driver_Build/

├── src/

### 2. Draw Graphics│   ├── hub75_driver.cpp/.hpp      # Main driver

│   ├── platform_hal.hpp            # Platform abstraction interface

```cpp│   ├── esp32_platform_impl.cpp/.hpp # ESP32 platform implementation

// Set individual pixels│   ├── lcd_parallel.cpp/.hpp       # ESP32 LCD_CAM driver

display.setPixel(10, 10, RGB(255, 0, 0));  // Red pixel│   ├── parallel_buffer.cpp/.hpp    # Buffer management

│   ├── dma_buffer_manager.hpp      # DMA buffer interface

// Fill entire display│   └── main.cpp                    # Demo application

display.fill(RGB(0, 255, 0));  // Green├── include/                         # Platform-specific headers

├── README.md                        # This file

// Clear to black└── *.md                            # Additional documentation

display.clear();```



// Update display## Building

display.show();

```**Platform:** ESP-IDF v5.x via PlatformIO



### 3. Brightness Control```bash

# Build

```cppplatformio run

// BCM brightness (0-255, affects display duration)

display.setBrightness(128);  // 50% brightness# Build and upload

```platformio run --target upload



## Protocol Implementation# Monitor serial output

platformio device monitor --baud 115200

### I2S Protocol (Included)```



The `HUB75_I2S_Protocol` uses ESP32's LCD_CAM peripheral for high-speed parallel transmission.## Performance



**Features:**- **Refresh Rate:** ~500Hz (flicker-free)

- DMA double buffering- **Color Depth:** 5-bit per channel (15-bit total RGB)

- 10MHz clock rate- **Brightness Levels:** 64 levels (0-255 mapped to 0-63)

- 14 GPIO pins (including dual OE)- **Memory Usage:** ~260KB Flash, ~14KB RAM

- Zero-copy direct buffer access- **BCM Timing:** Exponential (1, 2, 4, 8, 16 cycles per bit plane)



### Adding New Protocols## License



To implement a GPIO or custom protocol:[Your License Here]



```cpp## Contributing

class HUB75_GPIO_Protocol : public IHUB75Protocol {

public:Contributions welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

  bool init(const HUB75Config& config, int buffer_size) override {

    // Setup GPIO pins, allocate buffers## Credits

  }

  Created for the ARCOS robotics library project.

  bool start() override {
    // Start transmission loop
  }
  
  uint16_t* getWritableBuffer() override {
    // Return back buffer pointer
  }
  
  bool swapBuffer(const uint16_t* buffer, int size) override {
    // Swap buffers, update transmission
  }
  
  // Implement remaining interface methods...
};

// Use in application
HUB75_GPIO_Protocol gpioProtocol;
gpioProtocol.init(config, buffer_size);
display.init(config, &gpioProtocol);  // Just swap the protocol!
```

## Configuration Options

### Display Modes

```cpp
// Single panel (64x32)
config.expansion_mode = HUB75Config::ExpansionMode::SINGLE;
config.panel_count = 1;

// Dual panels with parallel OE (128x32 effective)
config.expansion_mode = HUB75Config::ExpansionMode::PARALLEL_OE;
config.panel_count = 2;
config.pins.oe_pin2 = 6;  // Second OE pin

// Daisy-chained panels (series data flow)
config.expansion_mode = HUB75Config::ExpansionMode::SERIES_CHAIN;
config.panel_count = 2;
```

### Panel Inversion

```cpp
// Flip panel 1 horizontally
config.panel_inversions[1].flip_horizontal = true;

// Flip panel 0 vertically
config.panel_inversions[0].flip_vertical = true;
```

### Color Settings

```cpp
config.colour_depth = 5;  // 5-bit per channel (32 levels)
config.enable_gamma_correction = true;
config.gamma_value = 2.2f;
```

## Pin Mapping (Default)

```cpp
config.pins.r0_pin = 7;    // Upper red
config.pins.g0_pin = 15;   // Upper green
config.pins.b0_pin = 16;   // Upper blue
config.pins.r1_pin = 17;   // Lower red
config.pins.g1_pin = 18;   // Lower green
config.pins.b1_pin = 8;    // Lower blue
config.pins.lat_pin = 36;  // Latch
config.pins.oe_pin = 35;   // Output enable 1
config.pins.oe_pin2 = 6;   // Output enable 2 (optional)
config.pins.a_pin = 41;    // Address A
config.pins.b_pin = 40;    // Address B
config.pins.c_pin = 39;    // Address C
config.pins.d_pin = 38;    // Address D
config.pins.e_pin = 42;    // Address E (64-row panels)
config.pins.clock_pin = 37; // Clock
```

## Build Instructions

### PlatformIO

```bash
# Build
pio run

# Upload
pio run --target upload

# Monitor
pio device monitor
```

### Configuration

Edit `platformio.ini` for your board:

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = espidf
monitor_speed = 115200
```

## Project Structure

```
HUB75_Driver_Build/
├── src/
│   ├── main.cpp                    # Application entry point
│   ├── hal.hpp                     # HAL aggregator
│   ├── drivers.hpp                 # Driver aggregator
│   ├── core/                       # Core abstractions
│   │   ├── platform_hal.hpp
│   │   ├── hal_protocal_parallel.hpp
│   │   ├── hal_protocal_dma.hpp
│   │   └── ...
│   ├── drivers/
│   │   └── components/
│   │       └── HUB75/
│   │           ├── driver_hub75_protocol.hpp  # Protocol interface
│   │           ├── driver_hub75.hpp/cpp       # Main driver
│   │           └── driver_hub75_i2s.hpp/cpp   # I2S implementation
│   └── platforms/
│       └── esp32/
│           └── wroom32s3/
│               └── module/
│                   ├── hal_connector.hpp
│                   └── platform implementations...
├── platformio.ini                  # Build configuration
├── README.md                       # This file
└── CODING_STYLE.md                 # Code style guide
```

## Performance

- **Refresh Rate**: ~240Hz (depends on BCM brightness)
- **Buffer Size**: 73968 samples (144KB) for dual 64x32 panels
- **Memory**: ~12KB framebuffer (RGB888) + DMA buffers
- **Clock Speed**: 10MHz parallel transmission

## Debugging

Enable debug logging in protocol:

```cpp
PLATFORM_LOG_I(TAG, "Buffer first 10 samples: %04X %04X...", 
               buffer[0], buffer[1]);
```

Serial output shows initialization sequence:

```
*** ESP32 BOOTED - APP STARTING ***
I (2386) HSL_DEMO: === HSL Color Scale Demo ===
I (2466) HUB75_I2S: HUB75 I2S protocol initialised
I (2476) HUB75_DRIVER: HUB75 driver initialised
```

## License

Part of the ARCOS hardware abstraction framework.

## Contributing

When contributing, follow the ARCOS coding style:
- 2-space indentation
- Tight braces (no space before `{`)
- camelCase for methods
- snake_case for variables

See `CODING_STYLE.md` for details.
