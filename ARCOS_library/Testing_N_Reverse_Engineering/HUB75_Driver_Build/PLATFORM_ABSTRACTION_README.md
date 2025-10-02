# HUB75 Driver - Platform Abstraction Layer

## Overview

The HUB75 LED matrix driver has been refactored to be **fully platform-agnostic**. The core driver code no longer depends on ESP-IDF or any platform-specific libraries. Instead, it uses an abstract **Platform HAL (Hardware Abstraction Layer)** interface that can be implemented for any microcontroller platform.

## Architecture

### Core Abstraction: `platform_hal.hpp`

The `IPlatformHAL` interface defines all platform-specific operations:

```cpp
class IPlatformHAL {
  // GPIO Operations
  virtual bool pinMode(PinNumber pin, PinMode mode) = 0;
  virtual bool setPinDriveStrength(PinNumber pin, PinDriveStrength strength) = 0;
  virtual bool digitalWrite(PinNumber pin, bool value) = 0;
  
  // Memory Operations
  virtual void* allocateMemory(size_t size, uint32_t caps) = 0;
  virtual void freeMemory(void* ptr) = 0;
  
  // Timing Operations
  virtual uint64_t getMicros() = 0;
  virtual void delayMicros(uint32_t us) = 0;
  
  // Logging Operations  
  virtual void log(LogLevel level, const char* tag, const char* format, ...) = 0;
  
  // Platform Information
  virtual const char* getPlatformName() = 0;
  virtual uint32_t getCpuFrequency() = 0;
};
```

### Platform-Agnostic Types

- **`PinNumber`**: Platform-independent pin identifier (replaces `gpio_num_t`)
- **`PIN_NC`**: "Not Connected" constant (replaces `GPIO_NUM_NC`)
- **Memory Capabilities**: `MEM_CAP_DMA`, `MEM_CAP_INTERNAL`, etc. (replaces ESP-IDF specific caps)
- **Log Macros**: `PLATFORM_LOG_E`, `PLATFORM_LOG_W`, `PLATFORM_LOG_I`, `PLATFORM_LOG_D`

## Platform Implementations

### ESP32 Implementation (`esp32_platform_impl.cpp`)

The ESP32 platform implementation maps the abstract interface to ESP-IDF functions:

- `pinMode()` → `gpio_config()`
- `allocateMemory()` → `heap_caps_malloc()`
- `getMicros()` → `esp_timer_get_time()`
- `log()` → `ESP_LOG*()`

Access via:
```cpp
#include "esp32_platform_impl.hpp"
IPlatformHAL* hal = getPlatformHAL();  // Returns ESP32 implementation
```

### Adding New Platforms

To port to a new platform (e.g., STM32, RP2040):

1. **Create platform implementation**:
   ```cpp
   // stm32_platform_impl.hpp
   class STM32PlatformHAL : public IPlatformHAL {
     // Implement all virtual methods using ST HAL
   };
   ```

2. **Implement `getPlatformHAL()`**:
   ```cpp
   IPlatformHAL* getPlatformHAL() {
     static STM32PlatformHAL instance;
     return &instance;
   }
   ```

3. **Link with HUB75 driver** - No changes to core code needed!

## Refactored Components

### ✅ Fully Platform-Agnostic

These files have **zero** ESP-IDF dependencies:

| File | Changes |
|------|---------|
| `platform_hal.hpp` | Core abstraction interface |
| `parallel_hardware_interface.hpp` | Uses `PinNumber` instead of `gpio_num_t` |
| `dma_buffer_manager.hpp` | Already platform-agnostic |
| `parallel_buffer.cpp` | Uses `getPlatformHAL()->allocateMemory/freeMemory` |
| `i2s_parallel_driver.hpp/cpp` | Uses platform HAL, opaque handles |
| `hub75_driver.hpp` | Uses `PinNumber`, platform HAL |
| `hub75_driver.cpp` | All ESP-IDF calls replaced with platform HAL |

### ⚠️ Platform-Specific (ESP32)

These files still contain ESP-IDF specific code:

| File | Status |
|------|--------|
| `esp32_platform_impl.cpp` | ESP32-specific implementation (expected) |
| `lcd_parallel.cpp` | Contains ESP-IDF LCD_CAM/GDMA code (needs wrapping) |
| `main.cpp` | Demo code using ESP-IDF (user application code) |

**Note**: `lcd_parallel.cpp` contains ESP32-specific peripheral code (LCD_CAM, GDMA). For true cross-platform support, either:
- Keep as ESP32-only implementation (use `i2s_parallel_driver` on other platforms)
- Wrap peripheral code in platform HAL extensions
- Create separate backend implementations per platform

## Usage Example

### Before (ESP32-specific):
```cpp
#include "esp_log.h"
#include "driver/gpio.h"

gpio_num_t pin = GPIO_NUM_5;
ESP_LOGI("TAG", "Setting pin %d", pin);
void* buf = heap_caps_malloc(1024, MALLOC_CAP_DMA);
```

### After (Platform-agnostic):
```cpp
#include "platform_hal.hpp"

PinNumber pin = 5;
PLATFORM_LOG_I("TAG", "Setting pin %d", pin);
void* buf = getPlatformHAL()->allocateMemory(1024, MEM_CAP_DMA);
```

## Memory Capability Flags

Platform-agnostic memory capabilities:

```cpp
enum MemoryCapability : uint32_t {
  MEM_CAP_DEFAULT = 0x00,       // Standard RAM
  MEM_CAP_DMA = 0x01,           // DMA-capable memory
  MEM_CAP_32BIT_ALIGNED = 0x02, // 32-bit aligned
  MEM_CAP_INTERNAL = 0x04,      // Internal RAM (faster)
  MEM_CAP_EXTERNAL = 0x08,      // External RAM/PSRAM
  MEM_CAP_IRAM = 0x10,          // Instruction RAM
  MEM_CAP_CACHE_SAFE = 0x20     // Cache-safe memory
};
```

Each platform implementation maps these to platform-specific memory types.

## Pin Configuration

Platform-agnostic GPIO configuration:

```cpp
IPlatformHAL* hal = getPlatformHAL();

// Configure pin as output
hal->pinMode(LED_PIN, PinMode::OUTPUT);

// Set drive strength
hal->setPinDriveStrength(LED_PIN, PinDriveStrength::STRONG);

// Write value
hal->digitalWrite(LED_PIN, true);
```

## Logging

Platform-agnostic logging macros:

```cpp
PLATFORM_LOG_E(TAG, "Error: %d", error_code);   // Error
PLATFORM_LOG_W(TAG, "Warning: %s", message);    // Warning
PLATFORM_LOG_I(TAG, "Info: %d Hz", frequency);  // Info
PLATFORM_LOG_D(TAG, "Debug: %p", pointer);      // Debug
PLATFORM_LOG_V(TAG, "Verbose: %f", value);      // Verbose
```

## Benefits

1. **Cross-Platform Portability**: Core driver works on any platform with HAL implementation
2. **Clean Separation**: Hardware-specific code isolated to platform implementations
3. **Easy Testing**: Can create mock HAL for unit testing
4. **Maintainability**: Single codebase for all platforms
5. **Backward Compatible**: ESP32 code works through platform implementation

## File Structure

```
src/
├── Platform Abstraction Layer
│   ├── platform_hal.hpp              ← Core interface
│   └── esp32_platform_impl.{hpp,cpp} ← ESP32 implementation
│
├── Core Driver (Platform-Agnostic)
│   ├── hub75_driver.{hpp,cpp}
│   ├── parallel_buffer.{hpp,cpp}
│   ├── i2s_parallel_driver.{hpp,cpp}
│   ├── parallel_hardware_interface.hpp
│   └── dma_buffer_manager.hpp
│
└── Platform-Specific Backends (Optional)
    └── lcd_parallel.{hpp,cpp}        ← ESP32 LCD_CAM backend
```

## Migration Guide

### For Existing ESP32 Code

1. **Include ESP32 platform implementation**:
   ```cpp
   #include "esp32_platform_impl.hpp"  // At top of main.cpp
   ```

2. **Replace ESP-IDF types**:
   - `gpio_num_t` → `PinNumber`
   - `GPIO_NUM_NC` → `PIN_NC`
   - `GPIO_NUM_5` → `5` (or `static_cast<PinNumber>(5)`)

3. **Replace ESP-IDF functions**:
   - `heap_caps_malloc()` → `getPlatformHAL()->allocateMemory()`
   - `heap_caps_free()` → `getPlatformHAL()->freeMemory()`
   - `ESP_LOG*()` → `PLATFORM_LOG_*()`
   - `esp_timer_get_time()` → `getPlatformHAL()->getMicros()`

### For New Platforms

1. Create `platform_impl.{hpp,cpp}` for your platform
2. Implement `IPlatformHAL` interface
3. Implement `getPlatformHAL()` singleton accessor
4. Link with HUB75 driver library
5. Done! No changes to core driver needed

## Example: STM32 Platform Implementation

```cpp
// stm32_platform_impl.hpp
#include "platform_hal.hpp"
#include "stm32f4xx_hal.h"

class STM32PlatformHAL : public IPlatformHAL {
public:
  bool pinMode(PinNumber pin, PinMode mode) override {
    GPIO_TypeDef* port = /* decode pin to port */;
    GPIO_InitTypeDef config = {
      .Pin = /* pin mask */,
      .Mode = (mode == PinMode::OUTPUT) ? GPIO_MODE_OUTPUT_PP : GPIO_MODE_INPUT,
      // ...
    };
    HAL_GPIO_Init(port, &config);
    return true;
  }
  
  void* allocateMemory(size_t size, uint32_t caps) override {
    // Use appropriate STM32 memory region
    if(caps & MEM_CAP_DMA) {
      return /* DMA-capable allocation */;
    }
    return malloc(size);
  }
  
  // ... implement other methods
};

IPlatformHAL* getPlatformHAL() {
  static STM32PlatformHAL instance;
  return &instance;
}
```

## Performance

The platform abstraction adds **minimal overhead**:
- Function calls are virtual (single indirect jump)
- Logging macros are inline
- Memory allocation is direct passthrough
- No runtime type checking or dynamic dispatch beyond initial HAL lookup

For performance-critical code, platform implementations can be optimized per-platform.

## Future Enhancements

Potential additions to platform HAL:

- [ ] DMA abstraction (descriptor setup, channels)
- [ ] Interrupt handling abstraction
- [ ] SPI/I2C abstractions for peripherals
- [ ] Power management interfaces
- [ ] File system abstraction (for configuration storage)
- [ ] Network abstraction (for remote control)

## License

Same as original HUB75 driver project.

## Contributing

To add a new platform implementation:

1. Fork the repository
2. Create `src/<platform>_platform_impl.{hpp,cpp}`
3. Implement `IPlatformHAL` interface
4. Test with HUB75 driver
5. Submit pull request with documentation

---

**Status**: ✅ Core refactoring complete. ESP32 fully functional. Ready for multi-platform expansion.
