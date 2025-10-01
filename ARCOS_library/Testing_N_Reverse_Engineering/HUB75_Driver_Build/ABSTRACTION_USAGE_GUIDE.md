# Hardware Abstraction Usage Guide

## Overview

The HUB75 driver uses a clean 3-layer abstraction architecture that completely isolates hardware details from application code.

---

## Quick Start (Automatic Defaults)

### Minimal Example
```cpp
#include "hub75_driver.hpp"

HUB75Driver display;

void setup() {
  // Configure display
  HUB75Config config = HUB75Config::getDefault();
  config.matrix_width = 64;
  config.matrix_height = 32;
  
  // Pin configuration (hardware-specific, but only pins)
  config.pins.r0_pin = 7;
  config.pins.g0_pin = 15;
  // ... other pins ...
  
  // Initialize (automatically creates LcdParallel + ParallelBuffer)
  display.init(config);
  display.start();
}

void loop() {
  // Draw using high-level API only
  display.setPixel(10, 10, RGB(255, 0, 0));  // Red pixel
  display.show();
}
```

**What happens internally:**
1. `display.init(config)` detects no hardware/buffer provided
2. Creates `LcdParallel` (default hardware backend)
3. Creates `ParallelBuffer` (default buffer manager)
4. Stores as opaque pointers in `hub75_driver.cpp`
5. Application never sees concrete types

---

## Advanced Usage (Custom Backends)

### Example: Custom Hardware Backend

```cpp
#include "hub75_driver.hpp"
#include "my_custom_parallel.hpp"  // Your implementation

// Create custom hardware implementation
IParallelHardware* custom_hw = new MyCustomParallel();

HUB75Driver display;

void setup() {
  HUB75Config config = HUB75Config::getDefault();
  
  // Inject custom hardware backend
  display.init(config, custom_hw, nullptr);  // nullptr = use default buffer
  display.start();
}
```

### Example: Custom Buffer Strategy

```cpp
#include "hub75_driver.hpp"
#include "circular_buffer_manager.hpp"  // Your implementation

// Create custom buffer manager (e.g., triple buffering)
IDmaBufferManager* triple_buffer = new CircularBufferManager();

HUB75Driver display;

void setup() {
  HUB75Config config = HUB75Config::getDefault();
  
  // Inject custom buffer manager
  display.init(config, nullptr, triple_buffer);  // nullptr = use default hardware
  display.start();
}
```

### Example: Full Custom Stack

```cpp
#include "hub75_driver.hpp"
#include "i2s_parallel_driver.hpp"    // Alternative hardware
#include "circular_buffer_manager.hpp" // Alternative buffer

// Create custom implementations
IParallelHardware* i2s_hw = new I2sParallelDriver();
IDmaBufferManager* circular_buf = new CircularBufferManager();

HUB75Driver display;

void setup() {
  HUB75Config config = HUB75Config::getDefault();
  
  // Inject both custom implementations
  display.init(config, i2s_hw, circular_buf);
  display.start();
}
```

---

## Creating Custom Implementations

### Step 1: Implement IParallelHardware

```cpp
// my_custom_parallel.hpp
#pragma once
#include "parallel_hardware_interface.hpp"

class MyCustomParallel : public IParallelHardware {
public:
  bool init(const gpio_num_t* pins, const ParallelHardwareConfig& cfg) override {
    // Initialize your hardware (I2S, SPI, GPIO bit-banging, etc.)
    return true;
  }
  
  bool setDirectBuffer(uint16_t* buffer, size_t len) override {
    // Set DMA buffer pointer
    return true;
  }
  
  bool swapBuffer(uint16_t* buffer, size_t len) override {
    // Hot-swap buffer without stopping
    return true;
  }
  
  bool start() override {
    // Start continuous transmission
    return true;
  }
  
  void stop() override {
    // Stop transmission
  }
  
  const char* getBackendName() const override {
    return "MyCustomParallel";
  }
  
  // ... implement other required methods ...
};
```

### Step 2: Implement IDmaBufferManager

```cpp
// my_custom_buffer.hpp
#pragma once
#include "dma_buffer_manager.hpp"

class MyCustomBuffer : public IDmaBufferManager {
public:
  bool init(const DmaBufferConfig& cfg) override {
    // Initialize buffer strategy
    return true;
  }
  
  bool allocate(size_t sample_count) override {
    // Allocate DMA-capable buffers
    return true;
  }
  
  uint16_t* getFrontBuffer() const override {
    return front_buffer;
  }
  
  uint16_t* getBackBuffer() const override {
    return back_buffer;
  }
  
  bool swapBuffers() override {
    // Swap front/back
    std::swap(front_buffer, back_buffer);
    return true;
  }
  
  // ... implement other required methods ...
  
private:
  uint16_t* front_buffer;
  uint16_t* back_buffer;
};
```

### Step 3: Use Your Implementation

```cpp
#include "hub75_driver.hpp"
#include "my_custom_parallel.hpp"
#include "my_custom_buffer.hpp"

IParallelHardware* hw = new MyCustomParallel();
IDmaBufferManager* buf = new MyCustomBuffer();

HUB75Driver display;
display.init(config, hw, buf);
```

---

## Interface Contracts

### IParallelHardware Methods

```cpp
// Initialize hardware with pin mapping
bool init(const gpio_num_t* pins, const ParallelHardwareConfig& cfg);

// Set DMA buffer (initial setup)
bool setDirectBuffer(uint16_t* buffer, size_t len);

// Swap buffer during transmission (hot-swap)
bool swapBuffer(uint16_t* buffer, size_t len);

// Start/stop transmission
bool start();
void stop();

// Identification
const char* getBackendName() const;

// Query capabilities
uint16_t* getDirectBuffer() const;
size_t getBufferSize() const;
```

### IDmaBufferManager Methods

```cpp
// Initialize with configuration
bool init(const DmaBufferConfig& cfg);

// Allocate DMA-capable memory
bool allocate(size_t sample_count);

// Get buffer pointers
uint16_t* getFrontBuffer() const;
uint16_t* getBackBuffer() const;
uint16_t* getBuffer(size_t index) const;

// Buffer management
bool swapBuffers();
size_t getBufferCount() const;
size_t getBufferSize() const;

// Configuration query
BufferMode getMode() const;

// Cleanup
void free();
```

---

## Configuration Structures

### ParallelHardwareConfig (Abstract)

```cpp
struct ParallelHardwareConfig {
  int clock_freq_hz;         // Clock frequency (10MHz typical)
  bool invert_clock;         // Clock polarity
  bool continuous_mode;      // Continuous DMA loop
  int data_width;            // Number of data pins (13 or 14)
  gpio_num_t clock_pin;      // Clock pin number
  gpio_num_t* data_pins;     // Array of data pins
  int data_pin_count;        // Number of pins in array
};
```

### DmaBufferConfig (Abstract)

```cpp
struct DmaBufferConfig {
  BufferMode mode;           // SINGLE_BUFFER, DOUBLE_BUFFER, CIRCULAR_BUFFER
  size_t buffer_count;       // Number of buffers (1-N)
  size_t sample_count;       // Samples per buffer
  bool auto_allocate;        // Auto-allocate on init
};
```

### BufferMode Enumeration

```cpp
enum class BufferMode {
  SINGLE_BUFFER,      // Single buffer (no swapping)
  DOUBLE_BUFFER,      // Front/back buffers (flicker-free)
  CIRCULAR_BUFFER     // Continuous circular buffer
};
```

---

## Application-Level API (main.cpp)

### Display Control

```cpp
// Initialize and control
display.init(config);
display.start();
display.stop();

// Query state
bool initialized = display.isInitialized();
bool running = display.isRunning();
```

### Drawing API

```cpp
// Pixel operations
display.setPixel(x, y, RGB(r, g, b));
RGB color = display.getPixel(x, y);
display.clear();

// Fill operations
display.fillRect(x, y, width, height, RGB(r, g, b));
display.fillScreen(RGB(r, g, b));

// Line drawing
display.drawLine(x0, y0, x1, y1, RGB(r, g, b));

// Frame update
display.show();  // Convert framebuffer to HUB75 format and update
```

### Dimensions

```cpp
int width = display.getWidth();
int height = display.getHeight();
```

---

## Platform-Specific Implementations

### ESP32-S3 (Default)
- **Hardware:** `LcdParallel` (LCD_CAM peripheral)
- **Buffer:** `ParallelBuffer` (DMA double-buffering)
- **Status:** ✅ Production-ready

### ESP32 (Classic)
- **Hardware:** `I2sParallelDriver` (I2S parallel mode)
- **Buffer:** `ParallelBuffer`
- **Status:** 🚧 Template skeleton available

### ESP32-C3/C6
- **Hardware:** Custom GPIO bit-banging or SPI
- **Buffer:** `ParallelBuffer`
- **Status:** 📝 Not yet implemented

### Other Platforms (STM32, RP2040, etc.)
- Create platform-specific `IParallelHardware` implementation
- Reuse abstract `IDmaBufferManager` interface
- Application code stays identical

---

## Best Practices

### ✅ DO:
- Keep application code in terms of `HUB75Driver` API only
- Use dependency injection for custom backends
- Implement both interfaces for complete flexibility
- Test with mock implementations

### ❌ DON'T:
- Include `lcd_parallel.hpp` in application code
- Include `parallel_buffer.hpp` in application code
- Access hardware peripherals directly from main.cpp
- Cast interface pointers to concrete types in application

---

## Example Projects

### Minimal Application (Auto Defaults)
```
src/
  main.cpp              # Only includes hub75_driver.hpp
  hub75_driver.hpp      # Pure abstract interface
  hub75_driver.cpp      # Creates LcdParallel + ParallelBuffer
  lcd_parallel.cpp      # Concrete LCD_CAM implementation
  parallel_buffer.cpp   # Concrete buffer implementation
```

### Multi-Platform Application
```
src/
  main.cpp                    # Platform-agnostic
  hub75_driver.hpp           # Pure abstract
  hub75_driver.cpp           # Platform selection logic
  platforms/
    esp32s3/
      lcd_parallel.cpp       # ESP32-S3 LCD_CAM
    esp32/
      i2s_parallel.cpp       # ESP32 I2S
    stm32/
      gpio_parallel.cpp      # STM32 GPIO
```

### Test Environment
```
test/
  test_hub75_driver.cpp      # Unit tests
  mocks/
    mock_parallel_hw.cpp     # Mock hardware
    mock_buffer_mgr.cpp      # Mock buffer manager
```

---

## Troubleshooting

### Issue: "Undefined reference to interface methods"
**Solution:** Ensure concrete implementations are compiled and linked

### Issue: "Cannot see concrete class methods"
**Solution:** Working as designed - use interface methods only

### Issue: "How to access hardware-specific features?"
**Solution:** Extend the abstract interface or use configuration parameters

### Issue: "Performance overhead from abstraction?"
**Solution:** Virtual function overhead is negligible (~1-2 CPU cycles per call)

---

## Summary

The HUB75 driver abstraction provides:

✅ **Clean separation** of concerns  
✅ **Platform independence** for application code  
✅ **Easy testing** with mock implementations  
✅ **Flexible backends** through dependency injection  
✅ **Future-proof** architecture for new hardware  

**Key principle:** Application code should only know about display dimensions, colors, and pixels - never about LCD_CAM, I2S, DMA, or platform-specific details.
