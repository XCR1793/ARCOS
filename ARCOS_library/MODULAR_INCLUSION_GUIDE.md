# ARCOS Modular Inclusion Guide

This guide explains how to use ARCOS's modular header system to include only the functionality you need, optimizing memory usage and compilation time.

## Available Headers

### Complete Library
- **`arcos.hpp`** - Everything included (largest footprint)

### Core Modules
- **`arcos_core.hpp`** - HAL + platform implementations
- **`arcos_algorithms.hpp`** - Sensor fusion + math algorithms
- **`arcos_drivers.hpp`** - All hardware drivers (includes core)

### Specialized Modules
- **`arcos_hal.hpp`** - HAL interfaces only (no platform code)
- **`arcos_drivers_display.hpp`** - Display drivers only
- **`arcos_drivers_sensors.hpp`** - Sensor drivers only

## Usage Patterns

### 1. Complete Functionality
```cpp
#include <arcos.hpp>
// Gets: HAL + platforms + algorithms + all drivers
// Use when: You need everything
```

### 2. HAL + Specific Features
```cpp
#include <arcos_core.hpp>       // HAL + platforms
#include <arcos_algorithms.hpp> // Sensor fusion
// Gets: HAL + platforms + algorithms (no drivers)
// Use when: You need math/fusion but implement your own drivers
```

### 3. Sensor-Focused Application
```cpp
#include <arcos_algorithms.hpp>       // Sensor fusion
#include <arcos_drivers_sensors.hpp>  // Sensor drivers (includes core)
// Gets: HAL + platforms + algorithms + sensor drivers
// Use when: IMU/environmental sensing without displays
```

### 4. Display-Focused Application
```cpp
#include <arcos_core.hpp>              // HAL + platforms
#include <arcos_drivers_display.hpp>   // Display drivers
// Gets: HAL + platforms + display drivers (no algorithms)
// Use when: LED matrices/OLED displays without sensor fusion
```

### 5. Minimal HAL
```cpp
#include <arcos_core.hpp>  // HAL + platforms
// Gets: Just hardware abstraction
// Use when: Minimal footprint, basic GPIO/timer operations
```

### 6. Custom Platform Development
```cpp
#include <arcos_hal.hpp>  // HAL interfaces only
// Gets: Just HAL interface definitions
// Use when: Implementing support for new platforms
```

## Memory Impact Comparison

| Include Pattern | Flash Usage | RAM Usage | Compile Time |
|----------------|-------------|-----------|--------------|
| `arcos.hpp` | ~50KB | ~5KB | Longest |
| Core + Algorithms | ~30KB | ~3KB | Medium |
| Core + Sensors | ~35KB | ~4KB | Medium |
| Core + Display | ~40KB | ~4KB | Medium |
| Core only | ~15KB | ~1KB | Fast |
| HAL only | ~5KB | ~0.5KB | Fastest |

*Estimates vary based on actual usage and compiler optimization*

## Dependency Relationships

```
arcos.hpp
├── arcos_core.hpp
├── arcos_algorithms.hpp
└── arcos_drivers.hpp
    ├── arcos_core.hpp (auto-included)
    ├── arcos_drivers_display.hpp
    └── arcos_drivers_sensors.hpp

arcos_drivers_display.hpp
└── arcos_core.hpp (auto-included)

arcos_drivers_sensors.hpp
└── arcos_core.hpp (auto-included)

arcos_core.hpp
└── arcos_hal.hpp + platform implementations

arcos_hal.hpp
└── Core HAL interfaces only
```

## Platform-Specific Considerations

### ESP32 Projects
```cpp
// Typical ESP32 project with sensors and display
#include <arcos_drivers.hpp>  // All drivers + core
#include <arcos_algorithms.hpp>  // For sensor fusion

// Build flags required:
// -DTARGET_ESP32_Wroom32S3_Module (or appropriate variant)
```

### Arduino Projects
```cpp
// Memory-constrained Arduino
#include <arcos_core.hpp>  // Minimal HAL only

// Build flags:
// -DTARGET_AVR_Atmega328p_Uno
```

### Custom Platforms
```cpp
// When porting to new hardware
#include <arcos_hal.hpp>  // Interface definitions only
// Then implement your own platform-specific code
```

## Best Practices

### 1. Start Minimal, Add as Needed
```cpp
// Start with minimal
#include <arcos_core.hpp>

// Add features incrementally
#include <arcos_algorithms.hpp>  // When you need sensor fusion
#include <arcos_drivers_sensors.hpp>  // When you add sensor hardware
```

### 2. Use Specific Driver Headers
```cpp
// Instead of all drivers
#include <arcos_drivers.hpp>

// Use specific categories
#include <arcos_drivers_sensors.hpp>  // Only if you have sensors
#include <arcos_drivers_display.hpp>  // Only if you have displays
```

### 3. Optimize for Your Use Case
```cpp
// Sensor data logger (no display)
#include <arcos_algorithms.hpp>
#include <arcos_drivers_sensors.hpp>

// LED matrix controller (no sensors)
#include <arcos_core.hpp>
#include <arcos_drivers_display.hpp>

// Sensor fusion research (no hardware drivers)
#include <arcos_core.hpp>
#include <arcos_algorithms.hpp>
```

## Common Patterns by Application Type

### IoT Sensor Node
```cpp
#include <arcos_algorithms.hpp>       // For data processing
#include <arcos_drivers_sensors.hpp>  // For sensor hardware
// Small footprint, sensor-focused
```

### LED Display Controller
```cpp
#include <arcos_core.hpp>              // For GPIO/timing
#include <arcos_drivers_display.hpp>   // For HUB75/OLED
// Display-optimized, no sensor processing
```

### IMU Development Board
```cpp
#include <arcos_algorithms.hpp>       // For fusion algorithms
#include <arcos_drivers_sensors.hpp>  // For IMU hardware
// Sensor fusion focused
```

### Custom HAL Development
```cpp
#include <arcos_hal.hpp>  // Interface definitions only
// Implement your own platform support
```

### Complete Development Platform
```cpp
#include <arcos.hpp>  // Everything available
// Maximum functionality, largest footprint
```

## Migration Guide

### From Monolithic to Modular
```cpp
// Old way (still works)
#include <arcos.hpp>

// New way (optimized)
#include <arcos_core.hpp>        // Always needed
#include <arcos_algorithms.hpp>  // If using sensor fusion
#include <arcos_drivers_sensors.hpp>  // If using sensors
// Exclude display drivers if not needed
```

### Gradual Adoption
1. Start with `arcos.hpp` (everything)
2. Identify unused features in your application
3. Switch to specific module headers
4. Measure memory savings
5. Optimize further as needed

## Troubleshooting

### Common Issues

**"Function not found" errors:**
- Check if you included the right module header
- Some functions require specific drivers to be included

**"Header not found" errors:**
- Verify the header name spelling
- Ensure your PlatformIO setup includes the ARCOS library

**Large memory usage:**
- Check if you're including more than needed
- Use specific driver headers instead of `arcos_drivers.hpp`

**Compilation takes too long:**
- Reduce included headers to only what you use
- Consider using `arcos_hal.hpp` for minimal builds

### Module Dependencies
- Drivers always require core HAL (`arcos_core.hpp` auto-included)
- Algorithms are independent and don't require drivers
- Platform implementations are auto-included based on build flags

---

This modular system gives you complete control over what ARCOS functionality is included in your project, allowing you to optimize for memory, compilation time, and specific use cases.