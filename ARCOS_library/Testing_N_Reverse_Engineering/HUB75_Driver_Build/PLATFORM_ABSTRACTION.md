# Platform Abstraction Guide

## Overview

The HUB75 driver uses a **Platform Abstraction Layer (HAL)** to separate hardware-specific code from the driver logic. This makes it easy to port the driver to different microcontrollers.

## Architecture

```
┌──────────────────────────────────┐
│     HUB75Driver (Portable)       │
│  - Business logic                │
│  - BCM algorithm                 │
│  - Color management              │
└────────┬─────────────────────────┘
         │ Uses interfaces
         │
┌────────▼─────────────────────────┐
│    Abstract Interfaces           │
│  - IPlatformHAL                  │
│  - IParallelHardware             │
│  - IDmaBufferManager             │
└────────┬─────────────────────────┘
         │ Implemented by
         │
┌────────▼─────────────────────────┐
│  Platform Implementation         │
│  - ESP32PlatformHAL              │
│  - LcdParallel                   │
│  - ParallelBuffer                │
└────────┬─────────────────────────┘
         │ Uses
         │
┌────────▼─────────────────────────┐
│     Hardware/SDK                 │
│  - ESP-IDF                       │
│  - RP2040 SDK                    │
│  - STM32 HAL                     │
└──────────────────────────────────┘
```

## Interfaces to Implement

### 1. IPlatformHAL

Basic platform services (memory, GPIO, timing).

**Location:** `src/platform_hal.hpp`

```cpp
class IPlatformHAL {
public:
  virtual ~IPlatformHAL() = default;
  
  // Memory management
  virtual void* allocateMemory(size_t size, uint32_t caps) = 0;
  virtual void freeMemory(void* ptr) = 0;
  
  // GPIO control
  virtual void pinMode(PinNumber pin, PinMode mode) = 0;
  virtual void digitalWrite(PinNumber pin, PinLevel level) = 0;
  virtual PinLevel digitalRead(PinNumber pin) = 0;
  
  // Timing
  virtual uint64_t getMicros() = 0;
  virtual void delayMicros(uint32_t us) = 0;
  virtual void delayMillis(uint32_t ms) = 0;
  
  // System info
  virtual uint32_t getCpuFrequency() = 0;
};
```

**Memory Capabilities (caps parameter):**
- `MEM_CAP_DEFAULT` - Standard RAM
- `MEM_CAP_DMA` - DMA-capable RAM (for buffers)
- `MEM_CAP_SPIRAM` - External PSRAM (if available)

---

### 2. IParallelHardware

Parallel data output interface (16-bit wide).

**Location:** `src/parallel_hardware_interface.hpp`

```cpp
class IParallelHardware {
public:
  virtual ~IParallelHardware() = default;
  
  // Initialize hardware
  virtual bool init(PinNumber* data_pins, 
                   const ParallelHardwareConfig& config) = 0;
  
  // Start/stop transmission
  virtual bool start(uint16_t* buffer, size_t length) = 0;
  virtual void stop() = 0;
  
  // Buffer management
  virtual bool swapBuffer(uint16_t* new_buffer_ptr, 
                         size_t buffer_len) = 0;
  virtual uint16_t* getDirectBuffer() const = 0;
};
```

**ParallelHardwareConfig:**
```cpp
struct ParallelHardwareConfig {
  uint32_t clock_freq_hz;      // Output clock frequency
  bool invert_clock;            // Clock polarity
  bool continuous_mode;         // Continuous DMA loop
  int data_width;               // Number of data pins
  PinNumber clock_pin;          // Clock pin
  PinNumber* data_pins;         // Array of data pins
  int data_pin_count;           // Length of array
};
```

---

### 3. IDmaBufferManager

DMA buffer management interface.

**Location:** `src/dma_buffer_manager.hpp`

```cpp
class IDmaBufferManager {
public:
  virtual ~IDmaBufferManager() = default;
  
  // Initialize buffer manager
  virtual bool init(const DmaBufferConfig& config) = 0;
  
  // Buffer access
  virtual uint16_t* getFrontBuffer() = 0;
  virtual uint16_t* getBackBuffer() = 0;
  
  // Swap buffers
  virtual bool swapBuffers() = 0;
};
```

**DmaBufferConfig:**
```cpp
struct DmaBufferConfig {
  int buffer_count;        // Typically 2 (double buffering)
  size_t sample_count;     // Number of uint16_t samples per buffer
  BufferMode mode;         // DOUBLE_BUFFER or TRIPLE_BUFFER
  bool auto_allocate;      // Automatically allocate memory
};
```

---

## Porting to New Platform

### Step 1: Implement IPlatformHAL

**Example for RP2040:**

```cpp
// rp2040_platform_impl.cpp
#include "platform_hal.hpp"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

class RP2040PlatformHAL : public IPlatformHAL {
public:
  void* allocateMemory(size_t size, uint32_t caps) override {
    // RP2040 doesn't distinguish DMA memory
    return malloc(size);
  }
  
  void freeMemory(void* ptr) override {
    free(ptr);
  }
  
  void pinMode(PinNumber pin, PinMode mode) override {
    gpio_init(pin);
    if(mode == PIN_MODE_OUTPUT){
      gpio_set_dir(pin, GPIO_OUT);
    }else{
      gpio_set_dir(pin, GPIO_IN);
    }
  }
  
  void digitalWrite(PinNumber pin, PinLevel level) override {
    gpio_put(pin, level == PIN_LEVEL_HIGH ? 1 : 0);
  }
  
  PinLevel digitalRead(PinNumber pin) override {
    return gpio_get(pin) ? PIN_LEVEL_HIGH : PIN_LEVEL_LOW;
  }
  
  uint64_t getMicros() override {
    return time_us_64();
  }
  
  void delayMicros(uint32_t us) override {
    sleep_us(us);
  }
  
  void delayMillis(uint32_t ms) override {
    sleep_ms(ms);
  }
  
  uint32_t getCpuFrequency() override {
    return clock_get_hz(clk_sys);
  }
};

// Global instance
static RP2040PlatformHAL rp2040_hal;

IPlatformHAL* getPlatformHAL(){
  return &rp2040_hal;
}
```

---

### Step 2: Implement IParallelHardware

**RP2040 using PIO:**

```cpp
// rp2040_parallel_pio.cpp
#include "parallel_hardware_interface.hpp"
#include "hardware/pio.h"
#include "hardware/dma.h"

class RP2040ParallelPIO : public IParallelHardware {
private:
  PIO pio;
  uint sm;
  int dma_channel;
  uint16_t* buffer;
  size_t buffer_len;
  
public:
  bool init(PinNumber* data_pins, 
           const ParallelHardwareConfig& config) override {
    // Claim PIO resources
    pio = pio0;
    sm = pio_claim_unused_sm(pio, true);
    
    // Load PIO program for parallel output
    // (PIO program assembly not shown - see RP2040 docs)
    
    // Configure pins
    for(int i = 0; i < config.data_pin_count; i++){
      pio_gpio_init(pio, data_pins[i]);
    }
    pio_gpio_init(pio, config.clock_pin);
    
    // Set up DMA
    dma_channel = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));
    
    return true;
  }
  
  bool start(uint16_t* buffer_ptr, size_t length) override {
    buffer = buffer_ptr;
    buffer_len = length;
    
    // Configure DMA transfer
    dma_channel_configure(
      dma_channel,
      &c,
      &pio->txf[sm],      // Write to PIO TX FIFO
      buffer,             // Read from buffer
      buffer_len,         // Transfer count
      true                // Start immediately
    );
    
    // Start PIO state machine
    pio_sm_set_enabled(pio, sm, true);
    
    return true;
  }
  
  void stop() override {
    pio_sm_set_enabled(pio, sm, false);
    dma_channel_abort(dma_channel);
  }
  
  bool swapBuffer(uint16_t* new_buffer_ptr, size_t new_len) override {
    // Update DMA source address
    buffer = new_buffer_ptr;
    buffer_len = new_len;
    
    // Reconfigure DMA
    dma_channel_set_read_addr(dma_channel, buffer, true);
    
    return true;
  }
  
  uint16_t* getDirectBuffer() const override {
    return buffer;
  }
};
```

---

### Step 3: Implement IDmaBufferManager

**Simple double buffering:**

```cpp
// simple_buffer_manager.cpp
#include "dma_buffer_manager.hpp"

class SimpleBufferManager : public IDmaBufferManager {
private:
  uint16_t* buffers[2];
  size_t buffer_size;
  int front_index;
  int back_index;
  
public:
  bool init(const DmaBufferConfig& config) override {
    buffer_size = config.sample_count;
    
    // Allocate buffers
    for(int i = 0; i < 2; i++){
      buffers[i] = (uint16_t*)malloc(buffer_size * sizeof(uint16_t));
      if(!buffers[i]) return false;
    }
    
    front_index = 0;
    back_index = 1;
    return true;
  }
  
  uint16_t* getFrontBuffer() override {
    return buffers[front_index];
  }
  
  uint16_t* getBackBuffer() override {
    return buffers[back_index];
  }
  
  bool swapBuffers() override {
    // Swap indices
    int temp = front_index;
    front_index = back_index;
    back_index = temp;
    return true;
  }
};
```

---

### Step 4: Update Build System

**CMakeLists.txt:**
```cmake
# Platform-specific sources
if(PLATFORM_RP2040)
  set(PLATFORM_SOURCES
    src/rp2040_platform_impl.cpp
    src/rp2040_parallel_pio.cpp
    src/simple_buffer_manager.cpp
  )
elseif(PLATFORM_ESP32)
  set(PLATFORM_SOURCES
    src/esp32_platform_impl.cpp
    src/lcd_parallel.cpp
    src/parallel_buffer.cpp
  )
endif()

# Add executable
add_executable(hub75_demo
  src/hub75_driver.cpp
  src/main.cpp
  ${PLATFORM_SOURCES}
)
```

---

## Platform-Specific Considerations

### ESP32-S3
- **LCD_CAM peripheral** - Hardware parallel interface
- **GDMA** - Automatic continuous refresh
- **Memory:** Distinguish DMA-capable vs non-DMA RAM
- **Clock:** Up to 40MHz parallel output

### RP2040
- **PIO** - Programmable I/O for parallel interface
- **DMA** - Manual DMA chain setup required
- **Memory:** All RAM is DMA-capable
- **Clock:** Up to 50MHz PIO clock

### STM32
- **FMC/FSMC** - Parallel memory interface
- **DMA** - Automatic continuous mode
- **Memory:** Distinguish DMA-capable regions
- **Clock:** Up to 100MHz on some models

### Arduino
- **Bit-banging** - Software parallel output (slow)
- **Alternatives:** Use SPI or I2S peripherals
- **Memory:** Standard malloc
- **Performance:** Limited to ~5MHz

---

## Testing Your Port

### 1. Basic Initialization Test
```cpp
HUB75Driver display;
HUB75Config config = HUB75Config::getDefault();

if(!display.init(config)){
  printf("FAIL: init() returned false\n");
}else{
  printf("PASS: init() succeeded\n");
}
```

### 2. Single Pixel Test
```cpp
display.fillScreen(RGB(0, 0, 0));    // Black
display.setPixel(0, 0, RGB(255, 0, 0));  // Red top-left
display.show();
// Verify: One red pixel visible
```

### 3. Pattern Test
```cpp
// Checkerboard
for(int y = 0; y < 32; y++){
  for(int x = 0; x < 64; x++){
    RGB color = ((x + y) % 2 == 0) ? RGB(255,255,255) : RGB(0,0,0);
    display.setPixel(x, y, color);
  }
}
display.show();
// Verify: Checkerboard pattern
```

### 4. Brightness Test
```cpp
for(int b = 0; b <= 255; b += 32){
  display.setBrightness(b);
  display.show();
  delay(500);
}
// Verify: Smooth brightness ramp
```

### 5. Performance Test
```cpp
uint32_t start = micros();
for(int i = 0; i < 100; i++){
  display.fillScreen(RGB(255, 0, 0));
  display.show();
}
uint32_t elapsed = micros() - start;
printf("Average frame time: %d us\n", elapsed / 100);
// Target: <20ms per frame (50 FPS)
```

---

## Common Porting Issues

### Issue: Display Flickers
- **Cause:** DMA not continuous
- **Fix:** Ensure buffer loops back to start

### Issue: Wrong Colors
- **Cause:** Pin mapping incorrect
- **Fix:** Verify R0,G0,B0,R1,G1,B1 order

### Issue: Garbled Display
- **Cause:** Clock too fast
- **Fix:** Reduce clock frequency

### Issue: Dim Display
- **Cause:** BCM timing too short
- **Fix:** Verify BCM cycle calculation

### Issue: Crashes on init()
- **Cause:** Memory allocation failure
- **Fix:** Check available RAM, use DMA-capable memory

---

## Reference Implementations

**ESP32-S3 (current):**
- `src/esp32_platform_impl.cpp`
- `src/lcd_parallel.cpp`
- `src/parallel_buffer.cpp`

**Example ports:**
- RP2040: [Link to example repo]
- STM32: [Link to example repo]
- Arduino: [Link to example repo]

---

## Need Help?

- Check existing implementations in `src/`
- Review `platform_hal.hpp` interface comments
- Ask in GitHub Issues
- Join Discord: [Link]
