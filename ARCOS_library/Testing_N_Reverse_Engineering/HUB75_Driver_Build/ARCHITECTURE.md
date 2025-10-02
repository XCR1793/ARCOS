# Architecture Overview

## System Architecture

The HUB75 driver uses a layered architecture with clear separation of concerns:

```
┌─────────────────────────────────────────────────┐
│              Application Layer                  │
│         (main.cpp - User Code)                  │
└────────────────┬────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────┐
│           HUB75Driver (hub75_driver.cpp)        │
│  - High-level API (setPixel, fillScreen, etc)   │
│  - RGB framebuffer management                   │
│  - BCM conversion & brightness control          │
│  - Panel inversion logic                        │
└────┬────────────────────┬───────────────────────┘
     │                    │
     │              ┌─────▼──────────────────────┐
     │              │ IDmaBufferManager          │
     │              │  (parallel_buffer.cpp)     │
     │              │  - Double buffering        │
     │              │  - Buffer swapping         │
     │              └─────┬──────────────────────┘
     │                    │
┌────▼────────────────────▼───────────────────────┐
│    IParallelHardware (lcd_parallel.cpp)         │
│  - ESP32 LCD_CAM peripheral control             │
│  - GDMA management                              │
│  - Hardware timing                              │
└────────────────┬────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────┐
│     Platform HAL (esp32_platform_impl.cpp)      │
│  - Memory allocation                            │
│  - GPIO control                                 │
│  - Timing/delays                                │
│  - CPU frequency                                │
└────────────────┬────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────┐
│              ESP-IDF HAL                        │
│  (ESP32-S3 Hardware Abstraction Layer)          │
└─────────────────────────────────────────────────┘
```

## Key Components

### 1. HUB75Driver (hub75_driver.cpp/.hpp)

**Purpose:** Main driver class providing high-level LED matrix control

**Responsibilities:**
- RGB framebuffer management (RGB888 format)
- Pixel operations (setPixel, drawLine, fillScreen)
- BCM (Binary Code Modulation) conversion
- Brightness control (0-255 → 64 levels)
- Panel inversion (flip horizontal/vertical)
- Gamma correction (2.2)
- Buffer coordination

**Key Methods:**
```cpp
bool init(const HUB75Config& config);
bool start();
void stop();
void setPixel(int x, int y, RGB color);
void fillScreen(RGB color);
void show();
void setBrightness(uint8_t brightness);  // 0-255
```

**Data Flow:**
```
User calls setPixel() 
  → Writes to RGB framebuffer
  → User calls show()
  → convertFramebufferToHUB75()
    → Converts RGB → 5-bit BCM format
    → Applies brightness (fill-up strategy)
    → Writes to back buffer
  → swapBuffers()
    → Swaps front/back buffers
    → Updates DMA descriptors
```

### 2. BCM Conversion Engine

**Binary Code Modulation** splits each color channel into 5 bit planes with exponential timing:

```
Bit Plane 0 (LSB): Display for 1 clock cycle
Bit Plane 1:       Display for 2 clock cycles
Bit Plane 2:       Display for 4 clock cycles
Bit Plane 3:       Display for 8 clock cycles
Bit Plane 4 (MSB): Display for 16 clock cycles

Total: 31 cycles (1+2+4+8+16) per full BCM refresh
```

**Brightness Control (Fill-Up Strategy):**
- Buffer allocated for maximum brightness (64x base BCM)
- Base: 31 cycles → Max: 31 × 64 = 1,984 cycles
- Active cycles = base_length × (brightness >> 2)
- Inactive cycles filled with OE disabled
- Example: brightness 128 → scale 32 → 50% of cycles active

### 3. Buffer Management

**Double Buffering:**
- Front buffer: DMA actively reads
- Back buffer: CPU writes next frame
- Atomic swap on show()

**Memory Layout:**
```
For 2x 64x32 panels with 5-bit color:
- Framebuffer: 128 × 32 × 3 bytes = 12,288 bytes (RGB888)
- DMA Buffers: ~130KB each × 2 = ~260KB (BCM format)
```

### 4. Platform Abstraction Layer (HAL)

**Interface:** `platform_hal.hpp` - Abstract interface
**Implementation:** `esp32_platform_impl.cpp` - ESP32-specific

**Abstracted Functions:**
```cpp
class IPlatformHAL {
  virtual void* allocateMemory(size_t size, uint32_t caps);
  virtual void freeMemory(void* ptr);
  virtual void pinMode(PinNumber pin, PinMode mode);
  virtual void digitalWrite(PinNumber pin, PinLevel level);
  virtual uint64_t getMicros();
  virtual void delayMicros(uint32_t us);
  virtual void delayMillis(uint32_t ms);
  virtual uint32_t getCpuFrequency();
};
```

**Benefits:**
- Easy porting to other platforms (RP2040, STM32, etc.)
- No direct ESP-IDF calls in driver core
- Testable in isolation

### 5. Hardware Interface

**ESP32 LCD_CAM Peripheral:**
- 16-bit parallel output
- GDMA-backed transfers
- Automatic continuous refresh
- 20MHz clock rate

**Pin Mapping:**
```cpp
struct HUB75Pins {
  PinNumber r0, g0, b0;      // Upper RGB
  PinNumber r1, g1, b1;      // Lower RGB
  PinNumber a, b, c, d, e;   // Row address (5 bits = 32 rows)
  PinNumber lat;             // Latch
  PinNumber oe;              // Output Enable 1
  PinNumber oe2;             // Output Enable 2 (dual panel)
  PinNumber clock;           // Clock
};
```

## Expansion Modes

### PARALLEL_OE (Dual Panel Mode)
- Two panels with independent OE pins
- Each panel clocks 64 pixels
- BCM timing per panel (31 × 64 cycles × 2 panels)
- Total width: 128 pixels

**Data Flow:**
```
For each row, for each bit plane:
  Clock 64 pixels → Panel 0
  Latch → BCM timing (OE1 active)
  Clock 64 pixels → Panel 1  
  Latch → BCM timing (OE2 active)
```

### SERIES_CHAIN (Daisy Chain)
- Panels connected in series
- Single OE pin
- Data flows through panels
- BCM timing once at end

### SINGLE (Single Panel)
- One 64×32 panel
- Standard operation

## Timing Diagrams

### BCM Refresh Cycle

```
Row 0, Plane 0 (1 cycle):
  [PIXEL DATA: 64 clocks, OE=HIGH] → [LATCH] → [BCM: 1 cycle, OE=LOW] → [DELAY: 3 cycles]

Row 0, Plane 1 (2 cycles):
  [PIXEL DATA: 64 clocks, OE=HIGH] → [LATCH] → [BCM: 2 cycles, OE=LOW] → [DELAY: 3 cycles]

Row 0, Plane 2 (4 cycles):
  [PIXEL DATA: 64 clocks, OE=HIGH] → [LATCH] → [BCM: 4 cycles, OE=LOW] → [DELAY: 3 cycles]

Row 0, Plane 3 (8 cycles):
  [PIXEL DATA: 64 clocks, OE=HIGH] → [LATCH] → [BCM: 8 cycles, OE=LOW] → [DELAY: 3 cycles]

Row 0, Plane 4 (16 cycles):
  [PIXEL DATA: 64 clocks, OE=HIGH] → [LATCH] → [BCM: 16 cycles, OE=LOW] → [DELAY: 3 cycles]

...repeat for rows 1-15 (total 16 rows, scanning upper and lower half simultaneously)
```

### Brightness Fill-Up Strategy

**Traditional (broken):** Scale BCM duration → buffer size mismatch

**Fill-Up (working):** Fill fixed buffer with variable OE active/inactive

```
Brightness 255 (100%):
  [PIXEL DATA] → [LATCH] → [OE=LOW: 1984 cycles] → [OE=HIGH: 0 cycles]

Brightness 128 (50%):
  [PIXEL DATA] → [LATCH] → [OE=LOW: 992 cycles] → [OE=HIGH: 992 cycles]

Brightness 64 (25%):
  [PIXEL DATA] → [LATCH] → [OE=LOW: 496 cycles] → [OE=HIGH: 1488 cycles]

Brightness 0 (0%):
  [PIXEL DATA] → [LATCH] → [OE=LOW: 0 cycles] → [OE=HIGH: 1984 cycles]
```

## Performance Characteristics

**Refresh Rate Calculation:**
```
Per row: (64 pixels × 5 planes) + (31 BCM × 64 × 5 planes) + (3 delay × 5 planes)
       = 320 + 9,920 + 15 = 10,255 clock cycles

16 rows × 2 panels: 10,255 × 16 × 2 = 328,160 cycles per frame

At 20MHz clock: 328,160 / 20,000,000 = ~16.4ms per frame
Refresh rate: 1 / 0.0164s ≈ 61 Hz per full BCM cycle

Apparent refresh: Much higher due to BCM interleaving (~500Hz flicker-free)
```

**Memory Usage:**
- Flash: ~251KB (driver + demo)
- RAM: ~14KB (framebuffer + stack)
- DMA: ~260KB (allocated from DMA-capable RAM)

## Thread Safety

**Current State:** NOT thread-safe

**Considerations:**
- `setPixel()` writes directly to framebuffer
- `show()` triggers conversion and buffer swap
- No mutex protection

**Recommendations for multi-threaded use:**
```cpp
// Add mutex to HUB75Driver
SemaphoreHandle_t framebuffer_mutex;

void setPixel(int x, int y, RGB color){
  xSemaphoreTake(framebuffer_mutex, portMAX_DELAY);
  // ... write to framebuffer ...
  xSemaphoreGive(framebuffer_mutex);
}
```

## Error Handling

**Initialization:**
- Returns `false` on failure
- Logs errors via ESP_LOGE
- Safe to call `init()` multiple times

**Runtime:**
- Out-of-bounds pixel writes are silently ignored
- Buffer swap failures logged but don't crash
- DMA errors trigger ESP32 exception handler

## Future Improvements

1. **Thread Safety:** Add mutex protection
2. **Dirty Rectangles:** Only update changed regions
3. **Hardware PWM:** Use OE pin PWM for brightness
4. **Color Correction:** Per-LED calibration
5. **Power Management:** Dynamic brightness based on content
6. **8-bit Color:** Optional full 8-bit per channel mode
