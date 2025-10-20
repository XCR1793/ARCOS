# ARCOS Fast Trigonometric Functions

This module provides extremely fast trigonometric functions (sin, cos, tan, sinh, cosh, tanh) and their inverse functions (asin, acos, atan, asinh, acosh, atanh) using pre-computed lookup tables with linear interpolation.

## Features

- **High Performance**: Up to 10-20x faster than standard library functions
- **Complete Function Set**: All major trigonometric, hyperbolic, and inverse functions
- **Selective Initialization**: Choose only the functions you need to optimize memory usage
- **Configurable Precision**: Choose from 5 precision levels (10° down to 0.001° angular precision)
- **Linear Interpolation**: Smooth results between table entries using templated lerp utilities
- **Memory Efficient**: Uses smart pointers for automatic memory management
- **Header-Only**: No separate compilation required, just include headers
- **Thread Safe**: Read-only operations after initialization

## Quick Start

### Method 1: Convenience Functions (Recommended)

```cpp
#include "core/maths.hpp"

using namespace arcos::core::maths;

int main(){
  // Initialize with desired precision and only basic trig functions
  initializeFastTrig(FastTrig::Precision::DEG_0_01, FastTrig::BASIC_TRIG);
  
  // Use fast trigonometric functions
  float angle = 1.5708f; // π/2
  float sin_val = fastSin(angle);
  float cos_val = fastCos(angle);
  float tan_val = fastTan(angle);
  
  // Hyperbolic functions
  float x = 1.0f;
  float sinh_val = fastSinh(x);
  float cosh_val = fastCosh(x);
  float tanh_val = fastTanh(x);
  
  // Inverse trigonometric functions
  float val = 0.5f;
  float asin_val = fastAsin(val);
  float acos_val = fastAcos(val);
  float atan_val = fastAtan(val);
  
  // Inverse hyperbolic functions
  float asinh_val = fastAsinh(val);
  float acosh_val = fastAcosh(1.5f);  // acosh requires x >= 1
  float atanh_val = fastAtanh(val);   // atanh requires -1 < x < 1
  
  return 0;
}
```

### Method 2: Direct Class Usage

```cpp
#include "core/maths/fast_trig.hpp"

using namespace arcos::core::maths;

int main(){
  // Create instance with custom precision and selected functions
  FastTrig fast_trig(FastTrig::Precision::DEG_0_001, 
                     FastTrig::BASIC_TRIG | FastTrig::HYPERBOLIC);
  
  float angle = 0.7854f; // π/4
  float result = fast_trig.sin(angle);
  
  return 0;
}
```

## Precision Levels

| Precision | Angular Resolution | Table Entries | Memory Usage | Use Case |
|-----------|-------------------|---------------|--------------|----------|
| DEG_10    | ~10°              | 36            | ~1.7 KB      | Ultra-fast, low precision |
| DEG_1     | ~1°               | 360           | ~17 KB       | Real-time applications |
| DEG_0_1   | ~0.1°             | 3,600         | ~172 KB      | General purpose |
| DEG_0_01  | ~0.01°            | 36,000        | ~1.7 MB      | High accuracy needed |
| DEG_0_001 | ~0.001°           | 360,000       | ~17 MB       | Maximum accuracy |

*Note: Memory usage shown is for ALL functions. Use selective initialization to reduce memory usage.*

## Function Groups

The module provides predefined function groups for easy selective initialization:

- **BASIC_TRIG**: sin, cos, tan (~1/4 of total memory)
- **HYPERBOLIC**: sinh, cosh, tanh (~1/4 of total memory)  
- **INVERSE_TRIG**: asin, acos, atan (~1/4 of total memory)
- **INVERSE_HYPERBOLIC**: asinh, acosh, atanh (~1/4 of total memory)
- **ALL_FUNCTIONS**: All 12 functions (full memory usage)

## Performance

Typical performance improvements over standard library functions:
- **DEG_10 precision**: 20-25x faster
- **DEG_1 precision**: 15-20x faster  
- **DEG_0_1 precision**: 10-15x faster
- **DEG_0_01 precision**: 8-12x faster
- **DEG_0_001 precision**: 5-8x faster

## API Reference

### FastTrig Class

```cpp
class FastTrig {
public:
  enum class Precision {
    DEG_10 = 36,      // ~10 degree precision
    DEG_1 = 360,      // ~1 degree precision
    DEG_0_1 = 3600,   // ~0.1 degree precision
    DEG_0_01 = 36000, // ~0.01 degree precision
    DEG_0_001 = 360000 // ~0.001 degree precision
  };
  
  enum class FunctionType {
    SIN, COS, TAN, SINH, COSH, TANH,
    ASIN, ACOS, ATAN, ASINH, ACOSH, ATANH
  };
  
  // Predefined function groups
  static constexpr uint32_t BASIC_TRIG;
  static constexpr uint32_t HYPERBOLIC;
  static constexpr uint32_t INVERSE_TRIG;
  static constexpr uint32_t INVERSE_HYPERBOLIC;
  static constexpr uint32_t ALL_FUNCTIONS;
  
  explicit FastTrig(Precision precision = Precision::DEG_0_1,
                   uint32_t functions = BASIC_TRIG);
  
  float sin(float angle) const;
  float cos(float angle) const;
  float tan(float angle) const;
  float sinh(float x) const;
  float cosh(float x) const;
  float tanh(float x) const;
  
  // Inverse functions
  float asin(float x) const;
  float acos(float x) const;
  float atan(float x) const;
  float asinh(float x) const;
  float acosh(float x) const;
  float atanh(float x) const;
  
  Precision getPrecision() const;
  size_t getTableSize() const;
  float getAngularPrecisionDegrees() const;
  
  // Function availability checking
  bool isFunctionInitialized(FunctionType function) const;
  uint32_t getInitializedFunctions() const;
};
```

### Convenience Functions

```cpp
// Global initialization with selective functions
void initializeFastTrig(FastTrig::Precision precision,
                       uint32_t functions = FastTrig::BASIC_TRIG);

// Fast trigonometric functions
float fastSin(float angle);
float fastCos(float angle);
float fastTan(float angle);
float fastSinh(float x);
float fastCosh(float x);
float fastTanh(float x);

// Fast inverse functions
float fastAsin(float x);
float fastAcos(float x);
float fastAtan(float x);
float fastAsinh(float x);
float fastAcosh(float x);
float fastAtanh(float x);
```

## Input Ranges

- **Trigonometric functions**: Accept any angle in radians (automatically normalized to [0, 2π))
- **Hyperbolic functions**: Input range is clamped to [-10, 10] for numerical stability
- **Inverse trigonometric functions**: 
  - asin, acos: Input clamped to [-1, 1]
  - atan: Input clamped to [-10, 10] for lookup table efficiency
- **Inverse hyperbolic functions**:
  - asinh: Input clamped to [-10, 10]
  - acosh: Input clamped to [1, 10] (domain requirement)
  - atanh: Input clamped to (-0.99999, 0.99999) to avoid singularities

## Implementation Details

- **Header-Only**: No separate compilation required, all implementation in headers
- **Templated Utilities**: Uses templated lerp functions for type-safe interpolation
- **Lookup Tables**: Pre-computed values stored in contiguous memory for cache efficiency
- **Linear Interpolation**: Smooth transitions between table entries for better accuracy
- **Angle Normalization**: Automatic normalization ensures consistent results for any input
- **Memory Management**: Uses `std::unique_ptr` for automatic cleanup
- **Singularity Handling**: Special handling for tan() near π/2, 3π/2, etc.
- **Domain Validation**: Automatic clamping for inverse functions to valid domains

## Examples

See the following files for complete examples:
- `examples/fast_trig_example.cpp` - Basic usage examples
- `tests/test_fast_trig.cpp` - Performance benchmarks and accuracy tests

## Thread Safety

- **Construction**: Not thread-safe (initialize in single-threaded context)
- **Usage**: Thread-safe for read operations after initialization
- **Global Functions**: Thread-safe after calling `initializeFastTrig()`

## Memory Usage

The module allocates memory for 12 lookup tables (6 forward functions + 6 inverse functions):
- Each table contains `precision` entries of 4 bytes each
- Total memory = `12 * precision * 4` bytes
- Examples: 
  - DEG_1 precision uses ~17 KB of memory
  - DEG_0_1 precision uses ~172 KB of memory
  - DEG_0_01 precision uses ~1.7 MB of memory

## Error Handling

- Invalid precision values are handled by the enum type system
- Hyperbolic function inputs are automatically clamped to safe ranges
- No exceptions are thrown during normal operation