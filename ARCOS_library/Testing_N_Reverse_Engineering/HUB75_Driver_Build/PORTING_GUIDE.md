# Porting Guide: ARCOS-Compatible Structure

This document explains the pseudo-compatible folder structure and how to complete the full ARCOS library integration in the future.

---

## Current Structure (Pseudo-Compatible)

```
src/
├── core/                            # Platform-agnostic abstractions
│   ├── platform_hal.hpp            # Base HAL (PinNumber, timing, memory)
│   ├── hal_parallel_interface.hpp  # Abstract parallel hardware interface
│   ├── hal_dma_buffer.hpp          # Abstract DMA buffer management
│   └── hal_parallel_buffer.hpp     # Parallel buffer implementation
│
├── platform/                        # Platform-specific implementations
│   └── esp32_s3/                   # ESP32-S3 WROOM32/Module
│       ├── platform_connector.hpp  # Maps HAL to platform implementations
│       ├── lcd_parallel.cpp/.hpp   # LCD_CAM peripheral driver
│       ├── i2s_parallel_driver.*   # I2S parallel driver (alternative)
│       ├── parallel_buffer.cpp     # DMA buffer manager (ESP32-specific)
│       └── esp32_platform_impl.*   # Platform HAL implementation
│
├── driver/                          # Device drivers (use only HAL APIs)
│   └── HUB75/                      # HUB75 LED matrix driver
│       ├── hub75_driver.hpp
│       └── hub75_driver.cpp
│
└── main.cpp                         # Application entry point
```

---

## Design Principles

### 1. **Three-Layer Architecture**

#### **Core Layer** (`core/`)
- **Purpose**: Platform-agnostic abstract interfaces
- **Dependencies**: Only C++ standard library
- **Files**:
  - `platform_hal.hpp` - Base types (PinNumber, timing, memory allocation)
  - `hal_parallel_interface.hpp` - IParallelHardware interface
  - `hal_dma_buffer.hpp` - IDmaBufferManager interface
  - `hal_parallel_buffer.hpp` - Buffer management interface

**Key Rule**: Core files must NEVER include platform-specific headers (no ESP32 SDK, no RP2040 SDK, etc.)

#### **Platform Layer** (`platform/`)
- **Purpose**: Platform-specific implementations of core interfaces
- **Dependencies**: Platform SDK (ESP-IDF, Pico SDK, STM32 HAL, etc.)
- **Structure**: `platform/{manufacturer}/{chip_family}/`
  - Example: `platform/esp32_s3/` (ESP32-S3 WROOM32/Module)
  - Future: `platform/rp2040/` (Raspberry Pi Pico)
  - Future: `platform/stm32/` (STM32 families)

**Key Files**:
- `platform_connector.hpp` - Type aliases mapping HAL to implementations
- Implementation files (`.cpp/.hpp`) - Concrete drivers

#### **Driver Layer** (`driver/`)
- **Purpose**: Device drivers using only HAL abstractions
- **Dependencies**: Only core/ headers (never platform/)
- **Structure**: `driver/{DeviceName}/`
  - Example: `driver/HUB75/`

**Key Rule**: Drivers are **portable** - they work on any platform with compatible HAL.

---

## Separation: LCD_CAM, I2S, and DMA

### Why Separate?

**LCD_CAM** exists on multiple ESP32 variants:
- ESP32-S3 (current)
- ESP32-C6
- ESP32-P4 (future)

**I2S Parallel** exists on:
- ESP32 (original)
- ESP32-S2
- ESP32-S3 (alternative to LCD_CAM)

By separating them, we enable:
1. Platform-specific optimizations (LCD_CAM is faster on S3)
2. Fallback support (use I2S if LCD_CAM unavailable)
3. Easy porting (swap implementation via `platform_connector.hpp`)

---

## Include Path Philosophy

### Core Files
```cpp
// hal_parallel_interface.hpp (in core/)
#pragma once
#include <cstdint>
#include <cstddef>
#include "platform_hal.hpp"  // Same folder
```

### Platform Files
```cpp
// lcd_parallel.hpp (in platform/esp32_s3/)
#pragma once
#include "../../core/platform_hal.hpp"           // Reach up to core
#include "../../core/hal_parallel_interface.hpp" // Reach up to core
#include <esp_lcd_panel_io.h>                    // Platform-specific OK here
```

### Driver Files
```cpp
// hub75_driver.hpp (in driver/HUB75/)
#pragma once
#include "../../core/platform_hal.hpp"           // Reach up to core
#include "../../core/hal_parallel_interface.hpp" // Reach up to core
// NEVER include platform/ files directly!
```

**Rule**: Drivers only see `core/`. Platform selection happens at link time.

---

## Platform Connector Pattern

### Current: `platform/esp32_s3/platform_connector.hpp`

```cpp
#ifndef PLATFORM_ESP32_S3_CONNECTOR_HPP_
#define PLATFORM_ESP32_S3_CONNECTOR_HPP_

#include "lcd_parallel.hpp"
#include "i2s_parallel_driver.hpp"
#include "esp32_platform_impl.hpp"

// Type aliases for this platform
using HAL_PARALLEL_LCD = LcdParallel;
using HAL_PARALLEL_I2S = I2SParallelDriver;
using HAL_PARALLEL_DEFAULT = HAL_PARALLEL_LCD;  // Choose default
using HAL_PLATFORM = ESP32PlatformHAL;

#endif
```

### How It Works

1. **Driver doesn't know about platform**:
   ```cpp
   // hub75_driver.cpp
   IParallelHardware* hw;  // Abstract interface only
   ```

2. **Application wires concrete implementation**:
   ```cpp
   // main.cpp
   #include "platform/esp32_s3/platform_connector.hpp"
   
   HAL_PARALLEL_DEFAULT lcd;  // Resolves to LcdParallel on ESP32-S3
   display.setHardware(&lcd);
   ```

3. **To port to RP2040**:
   ```cpp
   // platform/rp2040/platform_connector.hpp
   using HAL_PARALLEL_DEFAULT = RP2040_PIO_Parallel;  // Different impl
   ```

---

## Future: Full ARCOS Integration

### Step 1: Move to ARCOS Library Structure

```
ARCOS/
├── abstraction/
│   ├── hal.hpp                    # Top-level HAL selector
│   ├── core/                      # Your current core/
│   │   ├── hal_gpio_digital.hpp
│   │   ├── hal_parallel_interface.hpp  ← Your file
│   │   ├── hal_dma_buffer.hpp          ← Your file
│   │   └── ...
│   │
│   ├── platforms/                 # Your current platform/
│   │   ├── esp32/
│   │   │   └── wroom32s3/
│   │   │       └── module/
│   │   │           ├── hal_connector.hpp        ← Your platform_connector.hpp
│   │   │           ├── hal_parallel_lcd_cam.*   ← Your lcd_parallel.*
│   │   │           ├── hal_parallel_i2s.*       ← Your i2s_parallel.*
│   │   │           └── ...
│   │   │
│   │   ├── rp2040/                # Future port
│   │   └── stm32/                 # Future port
│   │
│   └── drivers/                   # Your current driver/
│       ├── HUB75/
│       │   └── hub75_driver.*     ← Your driver
│       ├── BME280/
│       ├── ICM20948/
│       └── ...
│
└── firmware/
    └── your_project/
        └── main.cpp
```

### Step 2: Update `hal.hpp` Selector

```cpp
// abstraction/hal.hpp
#pragma once

#include "core/hal_parallel_interface.hpp"
#include "core/hal_dma_buffer.hpp"

#if defined(TARGET_ESP32_Wroom32S3_Module)
  #include "platforms/esp32/wroom32s3/module/hal_connector.hpp"
#elif defined(TARGET_RP2040_Pico)
  #include "platforms/rp2040/pico/hal_connector.hpp"
#elif defined(TARGET_STM32F407_Discovery)
  #include "platforms/stm32/f407/discovery/hal_connector.hpp"
#else
  #error "No platform selected! Define TARGET_xxx in build flags"
#endif
```

### Step 3: Simplified Application

```cpp
// firmware/your_project/main.cpp
#include "abstraction/hal.hpp"              // Auto-selects platform
#include "abstraction/drivers/HUB75/hub75_driver.hpp"

HUB75Driver display;

extern "C" void app_main(){
  HAL_PARALLEL_DEFAULT hardware;  // Resolves to correct impl
  display.setHardware(&hardware);
  display.init(config);
  // ... rest of your code
}
```

### Step 4: Build Flags

```ini
# platformio.ini (ESP32-S3)
build_flags = -DTARGET_ESP32_Wroom32S3_Module

# platformio.ini (RP2040)
build_flags = -DTARGET_RP2040_Pico
```

---

## Porting to New Platform

### Example: Adding RP2040 Support

#### 1. Create Platform Folder
```
platforms/rp2040/pico/
├── hal_connector.hpp
├── hal_parallel_pio.cpp/.hpp  # PIO state machine implementation
└── hal_platform_rp2040.cpp/.hpp
```

#### 2. Implement `IParallelHardware`
```cpp
// hal_parallel_pio.hpp
#pragma once
#include "../../../core/hal_parallel_interface.hpp"

class RP2040_PIO_Parallel : public IParallelHardware{
public:
  bool init(const PinNumber* data_pins, const ParallelHardwareConfig& config) override{
    // Configure PIO state machine
    // Set up DMA
    return true;
  }
  
  bool start() override{
    // Start PIO + DMA
    return true;
  }
  
  // ... implement other methods
};
```

#### 3. Create Connector
```cpp
// hal_connector.hpp
#pragma once
#include "hal_parallel_pio.hpp"

using HAL_PARALLEL_DEFAULT = RP2040_PIO_Parallel;
using HAL_PLATFORM = RP2040PlatformHAL;
```

#### 4. **Driver Requires ZERO Changes!**
The HUB75 driver works immediately on RP2040 without modification.

---

## Testing Portability

### Rule: Drivers Must Not Know Platform

**Good** (portable):
```cpp
// hub75_driver.cpp
IParallelHardware* hw;  // Abstract interface
hw->init(pins, config);
hw->start();
```

**Bad** (platform-locked):
```cpp
// hub75_driver.cpp
#include <esp_lcd_panel_io.h>  // ❌ ESP32-specific!
lcd_cam_config_t cfg;           // ❌ Platform-specific type!
```

### Checklist
- ✅ `core/` files have no platform includes
- ✅ `driver/` files only include `core/`
- ✅ `platform/` files can include anything
- ✅ Applications include `platform_connector.hpp`

---

## Migration Roadmap

### Phase 1: Current (Pseudo-Compatible) ✅
- [x] Organized into `core/`, `platform/`, `driver/`
- [x] Abstract interfaces in `core/`
- [x] ESP32-S3 implementation in `platform/esp32_s3/`
- [x] HUB75 driver in `driver/HUB75/`
- [x] Working build system

### Phase 2: ARCOS Integration (Future)
- [ ] Move to ARCOS library folder structure
- [ ] Add top-level `hal.hpp` selector
- [ ] Rename `platform_connector.hpp` → `hal_connector.hpp`
- [ ] Add build flag detection (`TARGET_xxx`)

### Phase 3: Multi-Platform (Future)
- [ ] Add RP2040 port (`platform/rp2040/`)
- [ ] Add STM32 port (`platform/stm32/`)
- [ ] Add Arduino compatibility layer
- [ ] Community contributions for other platforms

---

## Benefits of This Structure

### ✅ **Portability**
- HUB75 driver works on any platform with `IParallelHardware` implementation
- No rewriting driver code when changing platforms

### ✅ **Maintainability**
- Clear separation: core abstractions, platform implementations, device drivers
- Easy to find and modify platform-specific code

### ✅ **Extensibility**
- Adding RP2040: implement `IParallelHardware`, done!
- Adding new driver: use existing HAL, done!

### ✅ **Zero Runtime Overhead**
- Abstract interfaces compile to direct function calls
- Compiler optimizes away abstraction layer

---

## Quick Reference

### Include Paths
| File Location | Include Path |
|---------------|--------------|
| `core/` | `"platform_hal.hpp"` (same folder) |
| `platform/esp32_s3/` | `"../../core/platform_hal.hpp"` |
| `driver/HUB75/` | `"../../core/platform_hal.hpp"` |

### Type Aliases
| Abstract Type | ESP32-S3 Implementation |
|---------------|-------------------------|
| `IParallelHardware` | `LcdParallel` (LCD_CAM) |
| `IParallelHardware` | `I2SParallelDriver` (I2S) |
| `IDmaBufferManager` | `ParallelBuffer` |
| `IPlatformHAL` | `ESP32PlatformHAL` |

### Platform Connector Usage
```cpp
#include "platform/esp32_s3/platform_connector.hpp"

HAL_PARALLEL_DEFAULT hw;  // Resolves to LcdParallel
HAL_PLATFORM::delayMs(100);
```

---

## Conclusion

Your code is now **pseudo-compatible** with ARCOS architecture. The structure allows:
1. **Immediate use** with current ESP32-S3 implementation
2. **Easy future porting** to ARCOS library structure
3. **Clean separation** between core, platform, and driver layers
4. **Multi-platform support** without modifying driver code

When ready for full ARCOS integration, simply move folders and add the top-level HAL selector!
