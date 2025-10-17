# ARCOS Library Examples

This directory contains example projects demonstrating how to use the ARCOS library in PlatformIO projects.

## Examples Available

### 1. Basic Usage (`basic_usage/`)
- **Purpose**: Shows basic library inclusion and platform-agnostic design
- **Features**: Complete library access, namespace demonstration, cross-platform compatibility
- **Includes**: Complete library (`arcos.hpp`)
- **Hardware**: Any supported platform (ESP32, Arduino, custom)
- **Framework**: Platform-agnostic (ESP-IDF, Arduino, custom)

### 2. IMU Fusion (`imu_fusion/`)
- **Purpose**: Demonstrates complete library inclusion with sensor fusion concepts
- **Features**: Namespace structure, modular access patterns, cross-platform design
- **Includes**: Complete library (`arcos.hpp`)
- **Hardware**: Any supported platform
- **Framework**: Platform-agnostic (ESP-IDF, Arduino, custom)

### 3. Minimal HAL (`minimal_hal/`)
- **Purpose**: Demonstrates smallest possible ARCOS footprint
- **Features**: Core HAL only, platform implementations, minimal memory usage
- **Includes**: Core only (`arcos_core.hpp`)
- **Hardware**: Any supported platform
- **Framework**: Platform-agnostic (ESP-IDF, Arduino, custom)

### 4. Sensors Only (`sensors_only/`)
- **Purpose**: Modular example with sensors and algorithms, no display drivers
- **Features**: Optimized footprint, sensor-focused functionality, modular inclusion
- **Includes**: `arcos_algorithms.hpp` + `arcos_drivers_sensors.hpp`
- **Hardware**: Any supported platform
- **Framework**: Platform-agnostic (ESP-IDF, Arduino, custom)

## Modular Inclusion Examples

The examples demonstrate different ways to include ARCOS modules:

| Example | Includes | Purpose | Memory Usage |
|---------|----------|---------|--------------|
| `basic_usage/` | `arcos.hpp` | Complete functionality | Largest |
| `imu_fusion/` | `arcos.hpp` | All features with IMU demo | Largest |
| `minimal_hal/` | `arcos_core.hpp` | Just HAL + platforms | Smallest |
| `sensors_only/` | `arcos_algorithms.hpp` + `arcos_drivers_sensors.hpp` | Sensors + fusion only | Medium |

Choose the example that best matches your project needs!

### Method 1: Copy Example to New Project
1. Create a new PlatformIO project
2. Copy the contents of any example folder to your project
3. Build and upload

### Method 2: Use Examples Directly
1. Open the example folder in PlatformIO
2. Build and upload directly

## Common Setup Steps

1. **Install PlatformIO**: Make sure you have PlatformIO Core or PlatformIO IDE
2. **Hardware Setup**: Connect your ESP32 and any required sensors
3. **Build Configuration**: The examples use the Git dependency method to include ARCOS
4. **Upload**: Build and upload to your board

## Hardware Connections

### All Examples
- **Basic Usage**: No hardware required - demonstrates library concepts
- **IMU Fusion**: No hardware required - demonstrates modular inclusion
- **Minimal HAL**: No hardware required - shows minimal footprint
- **Sensors Only**: No hardware required - shows sensor-focused inclusion

**Note**: These examples are concept demonstrations focusing on:
- Platform-agnostic design
- Modular library inclusion
- Namespace structure and usage
- Cross-platform compatibility

For actual hardware integration examples, refer to the main library documentation.

## Troubleshooting

### Build Issues
- Ensure you have internet connection (for Git dependency)
- Check that your ESP32 platform is properly installed
- Verify the correct board configuration in `platformio.ini`

### Runtime Issues
- Examples are concept demonstrations and don't require actual hardware
- Check serial monitor for demonstration output
- Verify platform detection in the output logs

### Common Error Messages
- **"Platform not detected"**: Normal for custom platforms
- **"Compilation error"**: Check platform/framework versions
- **"Serial output not visible"**: Check monitor_speed in platformio.ini

## Extending Examples

You can modify these examples to:
- Add additional sensors
- Implement custom algorithms
- Integrate with different hardware platforms
- Add wireless connectivity features

## Support

For issues or questions:
1. Check the main ARCOS README.md
2. Review the CODING_STYLE.md for development guidelines
3. Open an issue on the GitHub repository

---

**Note**: These examples are designed for learning and demonstration. For production use, consider additional error handling, power management, and optimization based on your specific requirements.