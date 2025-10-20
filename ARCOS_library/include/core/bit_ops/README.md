# Bit Operations Module

## Overview
The ARCOS bit operations module provides comprehensive utilities for bit manipulation, conversion, and extraction commonly used in embedded systems, graphics programming, and data processing applications.

## Features

### 🔄 **Bit Depth Conversions**
```cpp
// Convert between different bit depths with gamma correction support
uint8_t value5 = convert8to5(255);  // 8-bit to 5-bit: 255 -> 31
uint8_t value6 = convert8to6(255);  // 8-bit to 6-bit: 255 -> 63
uint8_t expanded = convert5to8(31); // 5-bit to 8-bit: 31 -> 255

// Generic bit depth conversion (compile-time optimized)
uint32_t converted = bit_ops.convertBitDepth<8, 5>(255);
```

### 🎯 **Bit Extraction & Manipulation**
```cpp
// Extract specific bits for bit-plane operations (HUB75 displays, PWM)
uint8_t bit = getBitFromValue(value, 3);  // Extract bit 3
uint32_t bits = extractBits(value, 2, 4); // Extract bits 2-5

// Set, clear, toggle individual bits
setBit(value, 5);     // Set bit 5
clearBit(value, 3);   // Clear bit 3
toggleBit(value, 2);  // Toggle bit 2
bool is_set = isBitSet(value, 4); // Check if bit 4 is set
```

### 📦 **Color Packing & RGB Operations**
```cpp
// Pack RGB components into compact formats
uint16_t rgb565 = packRGB565(r5, g6, b5);   // 5-6-5 RGB (16-bit)
uint16_t rgb555 = packRGB555(r5, g5, b5);   // 5-5-5 RGB (15-bit)

// Unpack back to components
uint8_t r, g, b;
unpackRGB565(rgb565, r, g, b);

// Convert between color formats
uint16_t packed = rgb24ToRgb565(255, 128, 64); // 24-bit to 16-bit
rgb565ToRgb24(packed, r, g, b);                 // 16-bit back to 24-bit
```

### 🎨 **Gamma Correction Support**
```cpp
// Initialize with gamma correction enabled
BitOpsConfig config;
config.enable_gamma_correction = true;
config.gamma_value = 2.2f;
initializeBitOps(config);

// All conversions now apply gamma correction automatically
uint8_t gamma_corrected = convert8to5(128); // Applies gamma curve
```

### 🔄 **Bit Counting & Analysis**
```cpp
// Count set bits and analyze bit patterns
int num_bits = popcount(0b10110101);         // Count of 1s: 5
int leading_zeros = countLeadingZeros(0x0F); // Leading zeros: 4
int trailing_zeros = countTrailingZeros(0xF0); // Trailing zeros: 4
```

### 🌀 **Bit Rotation & Endianness**
```cpp
// Rotate bits left or right
uint8_t rotated = rotateLeft(0b10000001, 2);  // Rotate left 2 positions
uint8_t rotated_r = rotateRight(value, 3);    // Rotate right 3 positions

// Endianness conversion
uint16_t swapped = byteSwap(0x1234);       // 0x1234 -> 0x3412
uint32_t big_endian = toBigEndian(value);  // Convert to big endian
```

### 🖥️ **Display-Specific Functions**
```cpp
// Generate bit planes for PWM displays (HUB75, LED matrices)
uint8_t rgb_data[300]; // 100 RGB pixels
uint8_t r_plane[100], g_plane[100], b_plane[100];

// Extract bit plane 3 from RGB data with gamma correction
rgbToBitPlane(rgb_data, 100, 3, r_plane, g_plane, b_plane);

// Generate all bit planes for PWM control
uint8_t intensity_values[64];
uint8_t* bit_planes[5]; // 5 bit planes for 5-bit values
generateAllBitPlanes(intensity_values, 64, 5, bit_planes, true);
```

## Usage Examples

### Basic Bit Operations
```cpp
#include <arcos_core.hpp>
using namespace arcos::core::bit_ops;

// Initialize bit operations
initializeBitOps();

// Convert 8-bit RGB to 5-bit for display
uint8_t r8 = 255, g8 = 128, b8 = 64;
uint8_t r5 = convert8to5(r8); // 31
uint8_t g6 = convert8to6(g8); // 32  
uint8_t b5 = convert8to5(b8); // 8

// Pack into RGB565 format
uint16_t pixel = packRGB565(r5, g6, b5);
```

### HUB75 Display Bit Plane Generation
```cpp
#include <arcos_core.hpp>
using namespace arcos::core::bit_ops;

// Initialize with gamma correction for better display quality
BitOpsConfig config;
config.enable_gamma_correction = true;
config.gamma_value = 2.2f;
initializeBitOps(config);

// RGB pixel data (64x32 display = 2048 pixels)
uint8_t rgb_buffer[6144]; // 2048 * 3 (RGB)
const size_t num_pixels = 2048;

// Generate bit planes for 5-bit color depth
for(int bit_plane = 0; bit_plane < 5; ++bit_plane) {
    uint8_t r_plane[num_pixels];
    uint8_t g_plane[num_pixels]; 
    uint8_t b_plane[num_pixels];
    
    // Extract bit plane with gamma correction applied
    rgbToBitPlane(rgb_buffer, num_pixels, bit_plane, 
                  r_plane, g_plane, b_plane);
    
    // Send to HUB75 driver...
    hub75_driver.loadBitPlane(bit_plane, r_plane, g_plane, b_plane);
}
```

### Advanced Bit Manipulation
```cpp
#include <arcos_core.hpp>
using namespace arcos::core::bit_ops;

// Create specialized bit operations instance
BitOps<64> bit_ops_6bit; // For 6-bit operations

// Analyze bit patterns
uint8_t sensor_data = 0b10110101;
int num_active_sensors = popcount(sensor_data);

// Extract specific sensor groups
uint8_t group1 = extractBits(sensor_data, 0, 4); // Lower 4 sensors
uint8_t group2 = extractBits(sensor_data, 4, 4); // Upper 4 sensors

// Check individual sensors
for(int i = 0; i < 8; ++i) {
    if(isBitSet(sensor_data, i)) {
        printf("Sensor %d is active\n", i);
    }
}
```

## Performance Characteristics

### Optimization Features
- **Compile-time optimization**: Template-based bit depth conversions
- **Lookup table gamma correction**: O(1) gamma correction using pre-computed tables
- **Inline functions**: Most operations compile to single instructions
- **Constexpr support**: Many operations can be computed at compile time

### Memory Usage
- **BitOps<32>**: ~32 bytes for 5-bit gamma table + minimal overhead
- **BitOps<64>**: ~64 bytes for 6-bit gamma table + minimal overhead
- **Global instance**: Single shared instance for convenience functions

### Typical Performance
- **Bit conversions**: ~1-2 CPU cycles per operation
- **Bit plane generation**: ~5-10 cycles per pixel
- **RGB packing/unpacking**: ~3-5 cycles per operation
- **Gamma correction**: ~1 cycle (lookup table)

## Integration with ARCOS

The bit operations module integrates seamlessly with other ARCOS components:

### With HUB75 Driver
```cpp
// Use bit operations for display data processing
class MyHUB75Driver : public HUB75Driver {
    void processPixelData(const uint8_t* rgb_data, size_t pixel_count) {
        for(int plane = 0; plane < 5; ++plane) {
            rgbToBitPlane(rgb_data, pixel_count, plane,
                         r_buffer[plane], g_buffer[plane], b_buffer[plane]);
        }
    }
};
```

### With PWM Controllers
```cpp
// Generate PWM bit patterns for LED control
void updateLEDArray(const uint8_t* brightness_values, size_t num_leds) {
    for(int bit_plane = 0; bit_plane < 8; ++bit_plane) {
        uint8_t bit_pattern[num_leds];
        generateBitPlane(brightness_values, num_leds, bit_plane, bit_pattern);
        
        // Send to PWM controller
        pwm_controller.setBitPlane(bit_plane, bit_pattern);
    }
}
```

## Configuration Options

```cpp
struct BitOpsConfig {
    bool enable_gamma_correction = false;  // Enable gamma correction
    bool use_lookup_tables = true;         // Use LUT optimization
    float gamma_value = 2.2f;              // Gamma correction value
};
```

## API Reference

### Core Classes
- **`BitOps<GammaTableSize>`**: Main template class for bit operations
- **`BitOpsConfig`**: Configuration structure
- **Type aliases**: `BitOps5bit`, `BitOps6bit`, `BitOps4bit`

### Convenience Functions
All core operations are available as global convenience functions after calling `initializeBitOps()`.

### Specialized Functions
- **Bit plane generation**: For display and PWM applications
- **Color space conversion**: RGB24 ↔ RGB565/RGB555
- **Gamma correction**: Automatic application during conversions

## Testing

Comprehensive Google Test suite located in `tests/core/test_bit_ops.cpp`:
- ✅ All bit depth conversions
- ✅ Bit extraction and manipulation
- ✅ RGB packing/unpacking
- ✅ Gamma correction accuracy
- ✅ Performance benchmarking
- ✅ Edge case handling

Run tests with:
```bash
cmake --build build --config Debug --target core_test_bit_ops
ctest -C Debug -R core_test_bit_ops -V
```