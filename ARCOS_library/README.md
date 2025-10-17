# ARCOS Hardware Abstraction Framework

A comprehensive header-only library providing hardware abstraction layers, sensor fusion algorithms, and driver implementations for embedded systems.

## Features

- **Hardware Abstraction Layer (HAL)**: GPIO, timers, I2C, SPI, parallel protocols
- **Sensor Fusion Algorithms**: IMU fusion, quaternion operations, Euler angles
- **Driver Implementations**: HUB75 LED matrices, ICM20948 IMU, BME280, OLED displays, SD cards
- **Platform Support**: ESP32 (S2/S3), Arduino, and extensible to other platforms
- **Header-Only**: No compilation required, just include and use

## Quick Start with PlatformIO

### Method 1: Git Dependency (Recommended)

Add the library to your `platformio.ini` file:

```ini
[env:your_board]
platform = espressif32
board = esp32s3usbotg
framework = espidf

lib_deps = 
    https://github.com/XCR1793/ARCOS.git#main

build_flags = 
    -std=c++17
    -DTARGET_ESP32_Wroom32S3_Module
```

### Method 2: Direct Clone

Clone the repository into your project's `lib` folder:

```bash
cd your_project/lib
git clone https://github.com/XCR1793/ARCOS.git
```

### Basic Usage

Include the main header in your code:

```cpp
#include <arcos.hpp>

// Use any ARCOS functionality
using namespace arcos;

void setup() {
    // Initialize hardware abstraction
    abstraction::HAL::init();
    
    // Use sensor fusion
    algorithms::fusion::IMUFusion imu_fusion;
    
    // Use drivers
    abstraction::drivers::HUB75Driver display_driver;
}
```

### Modular Inclusion

ARCOS supports both complete library inclusion and granular module selection:

#### 1. Complete Library
```cpp
#include <arcos.hpp>  // Everything: HAL + algorithms + drivers
```

#### 2. Core Modules
```cpp
#include <arcos_core.hpp>       // HAL + platform implementations
#include <arcos_algorithms.hpp> // Sensor fusion + math algorithms
#include <arcos_drivers.hpp>    // All hardware drivers (includes core)
```

#### 3. HAL Only
```cpp
#include <arcos_hal.hpp>   // Just HAL interfaces (no platform code)
#include <arcos_core.hpp>  // HAL + platform implementations
```

#### 4. Specific Driver Categories
```cpp
#include <arcos_drivers_display.hpp>  // Display drivers only (HUB75, OLED)
#include <arcos_drivers_sensors.hpp>  // Sensor drivers only (IMU, environmental)
```

#### 5. Mix and Match
```cpp
#include <arcos_core.hpp>              // HAL + platforms
#include <arcos_algorithms.hpp>        // Sensor fusion
#include <arcos_drivers_sensors.hpp>   // Just sensor drivers
// No display drivers included - smaller footprint
```

## Library Structure

```
include/
├── arcos.hpp                    # Complete library - all modules
├── arcos_core.hpp              # HAL + platform implementations  
├── arcos_algorithms.hpp        # Sensor fusion + math algorithms
├── arcos_drivers.hpp           # All hardware drivers
├── arcos_drivers_display.hpp   # Display drivers only
├── arcos_drivers_sensors.hpp   # Sensor drivers only
├── arcos_hal.hpp              # HAL interfaces only
├── abstraction/                # Hardware abstraction layer
│   ├── hal.hpp                # Main HAL interface
│   ├── core/                  # Core HAL interfaces
│   ├── platforms/             # Platform-specific implementations
│   └── drivers/               # Hardware drivers
├── algorithms/                 # Sensor fusion & math algorithms
│   ├── fusion/                # IMU and sensor fusion
│   └── orientation/           # Quaternion, Euler angles, gravity
```

## Include Strategy Guide

Choose the right include based on your needs:

| Use Case | Include | What You Get | Memory Impact |
|----------|---------|--------------|---------------|
| **Complete Project** | `arcos.hpp` | Everything | Largest |
| **HAL + Algorithms** | `arcos_core.hpp` + `arcos_algorithms.hpp` | No drivers | Medium |
| **Just Sensors** | `arcos_core.hpp` + `arcos_drivers_sensors.hpp` | No displays | Medium |
| **Just Displays** | `arcos_core.hpp` + `arcos_drivers_display.hpp` | No sensors | Medium |
| **Minimal HAL** | `arcos_hal.hpp` | Interfaces only | Smallest |
| **Custom Platform** | `arcos_hal.hpp` + your implementations | Custom build | Variable |

## Supported Platforms

| Platform | Status | Build Flag |
|----------|--------|------------|
| ESP32-S3 | ✅ Full Support | `-DTARGET_ESP32_Wroom32S3_Module` |
| ESP32-S2 | ✅ Full Support | `-DTARGET_ESP32_Wroom32S2_Esp32Dev` |
| Arduino Uno | ⚠️ Limited | `-DTARGET_AVR_Atmega328p_Uno` |

## Available Components

### Hardware Abstraction Layer

- **GPIO**: Digital I/O with compile-time optimization
- **Timers**: System timing and delays
- **Protocols**: I2C, SPI, parallel interfaces
- **Logging**: Platform-agnostic logging system

### Sensor Fusion Algorithms

- **IMU Fusion**: Complete inertial measurement processing
- **Quaternion Math**: 3D rotation calculations
- **Euler Angles**: Roll, pitch, yaw conversions
- **Gravity Vector**: Gravity compensation algorithms

### Hardware Drivers

- **HUB75**: RGB LED matrix driver with I2S DMA support
- **ICM20948**: 9-DOF IMU (accelerometer, gyroscope, magnetometer)
- **BME280**: Temperature, humidity, pressure sensor
- **OLED SH1107**: Monochrome OLED display driver
- **SD Card**: File system operations

## Usage Examples

### IMU Sensor Fusion

```cpp
#include <arcos.hpp>

using namespace arcos::algorithms;

void setup() {
    // Initialize IMU fusion
    fusion::IMUFusion imu;
    
    // Configure sensor
    orientation::QuaternionConfig config;
    config.sample_rate = 100.0f; // 100 Hz
    
    if (!imu.init(config)) {
        // Handle initialization error
    }
}

void loop() {
    // Read sensor data (implement your sensor reading)
    float accel[3] = {ax, ay, az};
    float gyro[3] = {gx, gy, gz};
    float mag[3] = {mx, my, mz};
    
    // Update fusion
    imu.update(accel, gyro, mag);
    
    // Get orientation
    orientation::EulerAngles angles = imu.getEulerAngles();
    float roll = angles.roll;
    float pitch = angles.pitch;
    float yaw = angles.yaw;
}
```

### HUB75 LED Matrix

```cpp
#include <arcos.hpp>

using namespace arcos::abstraction::drivers;

HUB75Driver display;

void setup() {
    // Configure display
    HUB75Config config;
    config.width = 64;
    config.height = 32;
    config.panels = 1;
    
    // Initialize with I2S protocol
    if (!display.init(config)) {
        // Handle error
    }
}

void loop() {
    // Set pixel color
    RGB color = {255, 0, 0}; // Red
    display.setPixel(10, 15, color);
    
    // Update display
    display.update();
}
```

### GPIO Operations

```cpp
#include <arcos.hpp>

using namespace arcos::abstraction::core;

void setup() {
    // Configure GPIO pin
    constexpr int LED_PIN = 2;
    
    // Set pin as output
    HALGPIODigital<LED_PIN>::setMode(GPIO_MODE_OUTPUT);
}

void loop() {
    // Toggle LED
    HALGPIODigital<LED_PIN>::write(true);
    delay(500);
    HALGPIODigital<LED_PIN>::write(false);
    delay(500);
}
```

## Configuration

### Build Flags

Add appropriate build flags for your target platform:

```ini
# ESP32-S3
build_flags = 
    -DTARGET_ESP32_Wroom32S3_Module
    -std=c++17

# ESP32-S2  
build_flags = 
    -DTARGET_ESP32_Wroom32S2_Esp32Dev
    -std=c++17

# Arduino Uno
build_flags = 
    -DTARGET_AVR_Atmega328p_Uno
    -std=c++17
```

### Memory Requirements

- **Flash**: ~10-50KB (depending on used components)
- **RAM**: ~1-5KB (depending on active drivers)
- **Stack**: Minimal additional overhead

## Development

### Adding New Platforms

1. Create platform-specific HAL implementations in `include/abstraction/platforms/`
2. Add appropriate conditional compilation flags
3. Update the main `arcos.hpp` header

### Adding New Drivers

1. Implement driver in `include/abstraction/drivers/components/`
2. Follow the existing coding style (see `CODING_STYLE.md`)
3. Add driver include to main header

## Coding Style

This library follows strict coding conventions defined in `CODING_STYLE.md`:

- **2-space indentation**
- **Tight braces** (no space before `{`)
- **camelCase** for methods, **snake_case** for variables
- **PascalCase** for classes
- Header guards format: `ARCOS_PATH_FILENAME_HPP_`

## License

MIT License - see LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Follow the coding style guidelines
4. Add tests if applicable
5. Submit a pull request

## Support

- GitHub Issues: [Report bugs or request features](https://github.com/XCR1793/ARCOS/issues)
- Documentation: See header files for detailed API documentation

---

**Version**: 1.0.0  
**Author**: XCR1793 (Feather Forge)  
**Repository**: https://github.com/XCR1793/ARCOS