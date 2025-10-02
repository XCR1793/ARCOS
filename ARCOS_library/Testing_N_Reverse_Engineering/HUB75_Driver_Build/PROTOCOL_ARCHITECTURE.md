# HUB75 Protocol Architecture

## Overview

The HUB75 driver uses a **protocol abstraction layer** to separate buffer composition from hardware transmission. This enables multiple backend implementations (I2S, GPIO, SPI, etc.) through dependency injection.

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                      Application Layer                       │
│                         (main.cpp)                           │
└────────────────────────────┬────────────────────────────────┘
                             │
                             │ Dependency Injection
                             │
┌────────────────────────────▼────────────────────────────────┐
│                       HUB75Driver                            │
│                  (driver_hub75.cpp)                          │
│                                                              │
│  ┌────────────────────────────────────────────────┐         │
│  │  Framebuffer (RGB888)                          │         │
│  │  - setPixel(), fill(), clear()                 │         │
│  │  - 128×32 pixels = 12KB                        │         │
│  └────────────────────────────────────────────────┘         │
│                          │                                   │
│                          │ Composition                       │
│                          ▼                                   │
│  ┌────────────────────────────────────────────────┐         │
│  │  BCM Conversion & Buffer Composition           │         │
│  │  - RGB888 → 5-bit BCM format                   │         │
│  │  - Direct write to protocol backBuffer         │         │
│  │  - Zero-copy design                            │         │
│  └────────────────────────────────────────────────┘         │
│                          │                                   │
│              protocol->getWritableBuffer()                   │
│              protocol->swapBuffer()                          │
└────────────────────────────┬────────────────────────────────┘
                             │
                             │ Protocol Interface
                             │
┌────────────────────────────▼────────────────────────────────┐
│                   IHUB75Protocol Interface                   │
│              (driver_hub75_protocol.hpp)                     │
│                                                              │
│  Abstract Methods:                                           │
│  ├─ init(config, buffer_size)                               │
│  ├─ start() / stop()                                         │
│  ├─ getWritableBuffer() → uint16_t*                         │
│  ├─ swapBuffer(buffer, size)                                │
│  └─ getBackendName() → const char*                          │
└────────────────────────────┬────────────────────────────────┘
                             │
                             │ Implementation
                             │
┌────────────────────────────▼────────────────────────────────┐
│              HUB75_I2S_Protocol Implementation               │
│                (driver_hub75_i2s.cpp)                        │
│                                                              │
│  ┌────────────────────────────────────────────────┐         │
│  │  Buffer Management                             │         │
│  │  - frontBuffer (active transmission)           │         │
│  │  - backBuffer (composition target)             │         │
│  │  - DMA-capable memory (MEM_CAP_DMA)            │         │
│  │  - Size: 73968 samples (144 KB)               │         │
│  └────────────────────────────────────────────────┘         │
│                          │                                   │
│              Injected Dependencies                           │
│              (via constructor)                               │
│                          │                                   │
│          ┌───────────────┴───────────────┐                  │
│          │                               │                  │
│          ▼                               ▼                  │
│  ┌───────────────────┐         ┌────────────────────┐      │
│  │ IParallelHardware │         │ IDmaBufferManager  │      │
│  │  (LCD_CAM/I2S)    │         │  (Double Buffer)   │      │
│  │  - 14 GPIO pins   │         │  - Allocate/Free   │      │
│  │  - 10MHz clock    │         │  - Swap buffers    │      │
│  │  - DMA transfer   │         │  - Get front/back  │      │
│  └───────────────────┘         └────────────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

## Key Components

### 1. HUB75Driver (driver_hub75.cpp)

**Responsibilities:**
- Manages RGB888 framebuffer
- Provides pixel operations (setPixel, fill, clear)
- Converts RGB888 to 5-bit BCM format
- Composes HUB75 protocol samples
- Writes directly to protocol's back buffer

**Key Methods:**
```cpp
// Buffer size calculation (static helper)
static int calculateBufferSize(const HUB75Config& config);

// Initialization with protocol injection
bool init(const HUB75Config& config, IHUB75Protocol* protocol);

// Start/stop display
bool start();
void stop();

// Pixel operations
void setPixel(int x, int y, const RGB& color);
void fill(const RGB& color);
void clear();

// Update display
void show();  // Converts framebuffer and swaps protocol buffers
```

**Does NOT:**
- Own hardware interfaces
- Manage DMA buffers
- Control transmission timing
- Know about I2S/GPIO implementation details

### 2. IHUB75Protocol Interface (driver_hub75_protocol.hpp)

**Responsibilities:**
- Defines abstract protocol interface
- Enables multiple backend implementations
- Guarantees consistent API across protocols

**Interface Methods:**
```cpp
class IHUB75Protocol{
public:
  // Initialize protocol
  virtual bool init(const HUB75Config& config, int buffer_size) = 0;
  
  // Start/stop transmission
  virtual bool start() = 0;
  virtual void stop() = 0;
  
  // State queries
  virtual bool isInitialized() const = 0;
  virtual bool isRunning() const = 0;
  
  // Buffer management
  virtual bool setBuffer(const uint16_t* buffer, int size) = 0;
  virtual bool swapBuffer(const uint16_t* buffer, int size) = 0;
  virtual uint16_t* getWritableBuffer() = 0;
  
  // Backend identification
  virtual const char* getBackendName() const = 0;
  
  virtual ~IHUB75Protocol() = default;
};
```

### 3. HUB75_I2S_Protocol (driver_hub75_i2s.cpp)

**Responsibilities:**
- Implements IHUB75Protocol interface
- Manages DMA double buffering
- Controls hardware transmission
- Configures GPIO pins for HUB75

**Key Methods:**
```cpp
// Extended initialization with hardware dependencies
bool init(const HUB75Config& config, int buffer_size,
          IParallelHardware* hardware, IDmaBufferManager* buffer_manager);

// Protocol interface implementation
bool start() override;        // Calls hwInterface->start()
void stop() override;         // Calls hwInterface->stop()
uint16_t* getWritableBuffer() override;  // Returns backBuffer
bool swapBuffer(...) override;           // Swaps DMA buffers
```

**Injected Dependencies:**
- `IParallelHardware*` - LCD_CAM/I2S hardware abstraction
- `IDmaBufferManager*` - DMA buffer allocation and swapping

**Does NOT:**
- Know about framebuffer format (RGB888)
- Perform BCM conversion
- Manage pixel data
- Own the hardware interfaces (application responsibility)

## Data Flow

### Initialization Sequence

```
1. Application creates hardware interfaces
   hardware = HAL_PARALLEL_DEFAULT()
   bufferManager = ParallelBuffer()

2. Application calculates buffer size
   buffer_size = HUB75Driver::calculateBufferSize(config)

3. Application initializes protocol
   i2sProtocol.init(config, buffer_size, &hardware, &bufferManager)
   ├─ Configures 14 GPIO pins
   ├─ Allocates DMA buffers (frontBuffer, backBuffer)
   └─ Initializes hardware interface

4. Application initializes driver
   display.init(config, &i2sProtocol)
   ├─ Stores protocol pointer
   └─ Allocates framebuffer (RGB888)

5. Application starts display
   display.start()
   ├─ Converts initial framebuffer to HUB75 format
   ├─ Writes to protocol->getWritableBuffer()
   ├─ Calls protocol->start()  ← Sets buffer size in hardware
   └─ Calls protocol->swapBuffer() ← Makes buffer active
```

### Frame Update Sequence

```
1. Application modifies framebuffer
   display.setPixel(x, y, color)
   display.fill(color)

2. Application calls show()
   display.show()

3. Driver converts framebuffer
   convertFramebufferToHUB75()
   ├─ Gets writable buffer: backBuffer = protocol->getWritableBuffer()
   ├─ For each row/plane:
   │  ├─ Read RGB888 pixels from framebuffer
   │  ├─ Apply gamma correction (if enabled)
   │  ├─ Convert to 5-bit per channel
   │  ├─ Compose HUB75 samples with BCM timing
   │  └─ Write directly to backBuffer
   └─ No intermediate copies!

4. Driver swaps buffers
   protocol->swapBuffer(nullptr, 0)
   ├─ nullptr indicates direct write (no copy)
   ├─ Protocol swaps frontBuffer ↔ backBuffer
   └─ Hardware interface updates DMA pointer

5. Hardware continuously transmits frontBuffer
   (No CPU involvement during transmission)
```

## Buffer Layout

### Framebuffer (RGB888)
```
Size: 128 × 32 × 3 bytes = 12 KB
Format: Linear array of RGB pixels
Layout: framebuffer[y * width + x] = {r, g, b}
```

### HUB75 Buffer (16-bit samples)
```
Size: 73968 samples = 144 KB
Format: Packed 14-bit GPIO states per sample
Layout per row:
  ├─ For each bit plane (0-4):
  │  ├─ 128 pixel samples (64 per panel)
  │  ├─ BCM delay samples (scaled by brightness)
  │  └─ 3 blanking samples
  └─ Address bits updated per row
```

### Sample Structure (16 bits)
```
Bit 0:  R0  (Upper half red)
Bit 1:  G0  (Upper half green)
Bit 2:  B0  (Upper half blue)
Bit 3:  R1  (Lower half red)
Bit 4:  G1  (Lower half green)
Bit 5:  B1  (Lower half blue)
Bit 6:  LAT (Latch signal)
Bit 7:  OE  (Output enable 1)
Bit 8:  A   (Address bit A)
Bit 9:  B   (Address bit B)
Bit 10: C   (Address bit C)
Bit 11: D   (Address bit D)
Bit 12: E   (Address bit E)
Bit 13: OE2 (Output enable 2)
```

## Protocol Extension

### Adding a GPIO Protocol

```cpp
// 1. Implement the interface
class HUB75_GPIO_Protocol : public IHUB75Protocol{
public:
  bool init(const HUB75Config& config, int buffer_size) override{
    // Allocate buffers
    backBuffer = new uint16_t[buffer_size];
    frontBuffer = new uint16_t[buffer_size];
    
    // Configure GPIO pins
    gpio_set_direction(config.pins.clock_pin, GPIO_MODE_OUTPUT);
    // ... configure all pins
    
    return true;
  }
  
  bool start() override{
    // Start transmission task
    xTaskCreate(transmitTask, "GPIO_TX", 4096, this, 5, &taskHandle);
    return true;
  }
  
  uint16_t* getWritableBuffer() override{
    return backBuffer;
  }
  
  bool swapBuffer(const uint16_t* buffer, int size) override{
    // Swap pointers
    std::swap(frontBuffer, backBuffer);
    return true;
  }
  
private:
  uint16_t* frontBuffer;
  uint16_t* backBuffer;
  TaskHandle_t taskHandle;
  
  static void transmitTask(void* param){
    auto* protocol = static_cast<HUB75_GPIO_Protocol*>(param);
    while(true){
      // Bit-bang frontBuffer samples to GPIO pins
      for(int i = 0; i < protocol->buffer_size; i++){
        uint16_t sample = protocol->frontBuffer[i];
        gpio_set_level(PIN_R0, (sample >> 0) & 1);
        gpio_set_level(PIN_G0, (sample >> 1) & 1);
        // ... set all pins
        gpio_set_level(PIN_CLOCK, 1);
        gpio_set_level(PIN_CLOCK, 0);
      }
    }
  }
};

// 2. Use in application
HUB75_GPIO_Protocol gpioProtocol;
gpioProtocol.init(config, buffer_size);
display.init(config, &gpioProtocol);  // Just change the protocol!
```

## Advantages

1. **Modularity**: Driver and protocol are independent
2. **Testability**: Can mock protocol for unit testing
3. **Flexibility**: Swap protocols at runtime
4. **Performance**: Zero-copy direct buffer writes
5. **Maintainability**: Clear separation of concerns

## Performance Characteristics

- **Zero-Copy Design**: Driver writes directly to DMA buffer
- **Double Buffering**: Composition and transmission happen in parallel
- **DMA Transmission**: No CPU involvement during display refresh
- **Efficient Memory**: No intermediate buffer allocations

### Memory Usage
```
Framebuffer (RGB888):     12 KB
DMA Buffer (front):       144 KB
DMA Buffer (back):        144 KB
Total:                    ~300 KB
```

### Timing
```
Frame composition:        ~2-5 ms (CPU)
Buffer swap:              <1 µs (pointer update)
Hardware transmission:    Continuous (DMA, no CPU)
Refresh rate:             ~240 Hz
```

## Summary

The protocol abstraction provides:
- Clean separation between composition and transmission
- Easy extension for new hardware backends
- Zero-copy performance with direct buffer access
- Type-safe dependency injection
- Clear ownership semantics

This architecture enables the driver to focus on pixel operations while protocols handle hardware-specific transmission details.
