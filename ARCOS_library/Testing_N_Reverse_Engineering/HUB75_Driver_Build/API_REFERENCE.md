# API Reference - HUB75 Driver

## Overview

Complete API reference for the ARCOS HUB75 LED matrix driver with protocol abstraction.

---

## HUB75Driver

Main driver class managing framebuffer and display operations.

**Header:** `drivers/components/HUB75/driver_hub75.hpp`  
**Namespace:** `arcos::abstraction::drivers`

### Constructor / Destructor

#### `HUB75Driver()`
```cpp
HUB75Driver()
```
Constructs an uninitialized driver instance.

#### `~HUB75Driver()`
```cpp
~HUB75Driver()
```
Destructor. Stops transmission and frees framebuffer. Does NOT delete the injected protocol (application responsibility).

---

### Static Methods

#### `calculateBufferSize()`
```cpp
static int calculateBufferSize(const HUB75Config& config)
```
Calculates the required buffer size for a given configuration.

**Parameters:**
- `config` - Display configuration

**Returns:** Buffer size in samples (uint16_t)

**Usage:**
```cpp
HUB75Config config = HUB75Config::getDefault();
int buffer_size = HUB75Driver::calculateBufferSize(config);
```

---

### Initialization

#### `init()`
```cpp
bool init(const HUB75Config& config, IHUB75Protocol* protocol)
```
Initializes the driver with configuration and protocol implementation.

**Parameters:**
- `config` - Display configuration
- `protocol` - Pointer to protocol implementation (REQUIRED, cannot be null)

**Returns:** `true` if successful, `false` on error

**Notes:**
- Must be called before any other operations
- Protocol must remain valid for driver lifetime
- Application owns the protocol object

**Example:**
```cpp
HUB75Config config = HUB75Config::getDefault();
HUB75_I2S_Protocol i2sProtocol;
HUB75Driver display;

int buffer_size = HUB75Driver::calculateBufferSize(config);
i2sProtocol.init(config, buffer_size, &hardware, &bufferManager);
display.init(config, &i2sProtocol);
```

#### `start()`
```cpp
bool start()
```
Starts continuous display transmission.

**Returns:** `true` if successful, `false` on error

**Notes:**
- Converts initial framebuffer to HUB75 format
- Starts protocol transmission
- Must be called after `init()`

#### `stop()`
```cpp
void stop()
```
Stops display transmission.

---

### State Queries

#### `isInitialized()`
```cpp
bool isInitialized() const
```
Checks if driver is initialized.

**Returns:** `true` if `init()` was successful

#### `isRunning()`
```cpp
bool isRunning() const
```
Checks if transmission is active.

**Returns:** `true` if `start()` was called and transmission is running

---

### Pixel Operations

#### `setPixel()`
```cpp
void setPixel(int x, int y, const RGB& colour)
```
Sets a single pixel color.

**Parameters:**
- `x` - X coordinate (0 to width-1)
- `y` - Y coordinate (0 to height-1)
- `colour` - RGB color value (0-255 per channel)

**Notes:**
- Coordinates outside bounds are ignored
- Changes are not visible until `show()` is called

**Example:**
```cpp
display.setPixel(10, 15, RGB(255, 0, 0));  // Red pixel
```

#### `getPixel()`
```cpp
RGB getPixel(int x, int y) const
```
Gets the color of a pixel.

**Parameters:**
- `x` - X coordinate
- `y` - Y coordinate

**Returns:** RGB color value, or black (0,0,0) if out of bounds

#### `clear()`
```cpp
void clear()
```
Clears the entire framebuffer to black.

**Example:**
```cpp
display.clear();
display.show();
```

#### `fill()`
```cpp
void fill(const RGB& colour)
```
Fills the entire display with a single color.

**Parameters:**
- `colour` - RGB color to fill

**Example:**
```cpp
display.fill(RGB(0, 255, 0));  // Fill with green
display.show();
```

---

### Display Update

#### `show()`
```cpp
void show()
```
Updates the display with the current framebuffer contents.

**Notes:**
- Converts framebuffer to HUB75 format
- Writes to protocol's back buffer
- Swaps protocol buffers to make changes visible
- This is the only way to make pixel changes visible

**Example:**
```cpp
display.clear();
display.setPixel(0, 0, RGB(255, 0, 0));
display.setPixel(1, 1, RGB(0, 255, 0));
display.show();  // Now visible on display
```

---

### Brightness Control

#### `setBrightness()`
```cpp
void setBrightness(uint8_t brightness)
```
Sets global display brightness using BCM timing.

**Parameters:**
- `brightness` - Brightness level (0-255)
  - 0 = minimum (dim)
  - 255 = maximum (full brightness)

**Notes:**
- Controls BCM sample duration, not pixel values
- Changes take effect on next `show()` call
- Does not modify framebuffer pixel values

**Example:**
```cpp
display.setBrightness(128);  // 50% brightness
display.show();
```

#### `getBrightness()`
```cpp
uint8_t getBrightness() const
```
Gets current brightness level.

**Returns:** Brightness value (0-255)

---

### Gamma Correction

#### `setGammaCorrection()`
```cpp
void setGammaCorrection(bool enabled, float gamma = 2.2f)
```
Enables or disables gamma correction.

**Parameters:**
- `enabled` - Enable gamma correction
- `gamma` - Gamma value (typically 1.8 to 2.6)

**Notes:**
- Gamma correction improves visual linearity
- Applied during framebuffer conversion
- Default gamma tables: 1.8, 2.2, 2.6

#### `isGammaCorrectionEnabled()`
```cpp
bool isGammaCorrectionEnabled() const
```
Checks if gamma correction is enabled.

**Returns:** `true` if enabled

---

### Configuration

#### `getConfig()`
```cpp
const HUB75Config& getConfig() const
```
Gets the current configuration.

**Returns:** Reference to configuration structure

#### `updateConfig()`
```cpp
bool updateConfig(const HUB75Config& newConfig)
```
Updates the driver configuration.

**Parameters:**
- `newConfig` - New configuration

**Returns:** `true` if successful

**Notes:**
- Some changes may require re-initialization
- Display must be stopped before updating

---

### Display Dimensions

#### `getWidth()`
```cpp
int getWidth() const
```
Gets the effective display width in pixels.

**Returns:** Width (accounts for expansion modes)

#### `getHeight()`
```cpp
int getHeight() const
```
Gets the display height in pixels.

**Returns:** Height in pixels

---

### Framebuffer Operations

#### `getFrameBuffer()`
```cpp
FrameBuffer getFrameBuffer() const
```
Gets the current framebuffer information.

**Returns:** FrameBuffer structure with dimensions and pixel pointer

#### `setFrameBuffer()`
```cpp
bool setFrameBuffer(const FrameBuffer& buffer)
```
Replaces the framebuffer with a new one.

**Parameters:**
- `buffer` - New framebuffer structure

**Returns:** `true` if successful

#### `uploadFrameBuffer()`
```cpp
bool uploadFrameBuffer(const RGB* pixels, int width, int height)
```
Uploads pixel data to the framebuffer.

**Parameters:**
- `pixels` - Array of RGB pixels
- `width` - Buffer width
- `height` - Buffer height

**Returns:** `true` if successful

#### `copyFrameBuffer()`
```cpp
void copyFrameBuffer(RGB* destination) const
```
Copies framebuffer contents to destination array.

**Parameters:**
- `destination` - Destination buffer (must be pre-allocated)

---

## RGB Structure

Simple RGB color structure.

**Header:** `drivers/components/HUB75/driver_hub75.hpp`

### Definition

```cpp
struct RGB {
  uint8_t r, g, b;
  RGB();
  RGB(uint8_t red, uint8_t green, uint8_t blue);
};
```

### Usage

```cpp
RGB red(255, 0, 0);
RGB green(0, 255, 0);
RGB blue(0, 0, 255);
RGB white(255, 255, 255);
RGB black(0, 0, 0);

RGB custom(128, 64, 200);
```

---

## HUB75Config Structure

Display configuration structure.

**Header:** `drivers/components/HUB75/driver_hub75.hpp`

### Display Settings

```cpp
int matrix_width = 64;        // Panel width in pixels
int matrix_height = 32;       // Panel height in pixels
```

### Expansion Modes

```cpp
enum class ExpansionMode {
  SINGLE,           // Single panel (64×32)
  PARALLEL_OE,      // Multiple panels with separate OE pins
  SERIES_CHAIN      // Daisy-chained panels
};

ExpansionMode expansion_mode = ExpansionMode::SINGLE;
int panel_count = 1;          // Number of panels (1-4)
```

### Panel Inversion

```cpp
struct PanelInversion {
  bool flip_horizontal = false;  // Mirror left-right
  bool flip_vertical = false;    // Mirror top-bottom
};

PanelInversion panel_inversions[4];  // Per-panel flip settings
```

### Color Settings

```cpp
int colour_depth = 5;              // Bits per channel (1-8)
bool enable_gamma_correction = true;
float gamma_value = 2.2f;
bool enable_anti_aliasing = false;
```

### Hardware Settings

```cpp
int clock_freq_hz = 10000000;  // 10MHz default

struct PinMapping {
  PinNumber r0_pin = 7;
  PinNumber g0_pin = 15;
  PinNumber b0_pin = 16;
  PinNumber r1_pin = 17;
  PinNumber g1_pin = 18;
  PinNumber b1_pin = 8;
  PinNumber lat_pin = 36;
  PinNumber oe_pin = 35;
  PinNumber oe_pin2 = PIN_NC;  // Optional second OE
  PinNumber a_pin = 41;
  PinNumber b_pin = 40;
  PinNumber c_pin = 39;
  PinNumber d_pin = 38;
  PinNumber e_pin = 42;
  PinNumber clock_pin = 37;
} pins;
```

### Static Factory Method

```cpp
static HUB75Config getDefault();
```
Returns default configuration for 64×32 single panel.

### Example Configurations

#### Single Panel
```cpp
HUB75Config config = HUB75Config::getDefault();
config.matrix_width = 64;
config.matrix_height = 32;
config.expansion_mode = HUB75Config::ExpansionMode::SINGLE;
```

#### Dual Panels (Parallel OE)
```cpp
HUB75Config config = HUB75Config::getDefault();
config.matrix_width = 64;
config.matrix_height = 32;
config.expansion_mode = HUB75Config::ExpansionMode::PARALLEL_OE;
config.panel_count = 2;
config.pins.oe_pin2 = 6;  // Second OE pin
```

#### Panel Inversion
```cpp
config.panel_inversions[1].flip_horizontal = true;  // Flip panel 1
```

---

## IHUB75Protocol Interface

Abstract protocol interface for hardware transmission.

**Header:** `drivers/components/HUB75/driver_hub75_protocol.hpp`  
**Namespace:** `arcos::abstraction::drivers`

### Pure Virtual Methods

#### `init()`
```cpp
virtual bool init(const HUB75Config& config, int buffer_size) = 0
```
Initialize the protocol with configuration.

#### `start()` / `stop()`
```cpp
virtual bool start() = 0
virtual void stop() = 0
```
Start and stop transmission.

#### `isInitialized()` / `isRunning()`
```cpp
virtual bool isInitialized() const = 0
virtual bool isRunning() const = 0
```
Query protocol state.

#### `setBuffer()`
```cpp
virtual bool setBuffer(const uint16_t* buffer, int size) = 0
```
Copy buffer to protocol's front buffer.

#### `swapBuffer()`
```cpp
virtual bool swapBuffer(const uint16_t* buffer, int size) = 0
```
Swap buffers. If `buffer` is `nullptr`, assumes direct write to back buffer.

#### `getWritableBuffer()`
```cpp
virtual uint16_t* getWritableBuffer() = 0
```
Get pointer to back buffer for direct writes (zero-copy).

#### `getBackendName()`
```cpp
virtual const char* getBackendName() const = 0
```
Get human-readable backend name.

---

## HUB75_I2S_Protocol

I2S protocol implementation using LCD_CAM peripheral.

**Header:** `drivers/components/HUB75/driver_hub75_i2s.hpp`  
**Namespace:** `arcos::abstraction::drivers`

### Extended Initialization

```cpp
bool init(const HUB75Config& config, int buffer_size,
          IParallelHardware* hardware, IDmaBufferManager* buffer_manager)
```

Initializes with injected hardware dependencies.

**Parameters:**
- `config` - Display configuration
- `buffer_size` - Buffer size in samples
- `hardware` - Parallel hardware interface (LCD_CAM)
- `buffer_manager` - DMA buffer manager

**Example:**
```cpp
HAL_PARALLEL_DEFAULT hardware;
ParallelBuffer bufferManager;
HUB75_I2S_Protocol i2sProtocol;

int buffer_size = HUB75Driver::calculateBufferSize(config);
i2sProtocol.init(config, buffer_size, &hardware, &bufferManager);
```

### Interface Implementation

Implements all `IHUB75Protocol` methods:
- `start()` - Starts LCD_CAM transmission
- `stop()` - Stops transmission
- `getWritableBuffer()` - Returns DMA back buffer
- `swapBuffer()` - Swaps DMA buffers and updates hardware

---

## Complete Usage Example

```cpp
#include "hal.hpp"
#include "drivers.hpp"
#include "drivers/components/HUB75/driver_hub75_i2s.hpp"

using namespace arcos::abstraction::drivers;

void app_main() {
  // 1. Create hardware interfaces
  HAL_PARALLEL_DEFAULT hardware;
  ParallelBuffer bufferManager;
  HUB75_I2S_Protocol i2sProtocol;
  HUB75Driver display;
  
  // 2. Configure display
  HUB75Config config = HUB75Config::getDefault();
  config.expansion_mode = HUB75Config::ExpansionMode::PARALLEL_OE;
  config.panel_count = 2;
  config.pins.oe_pin2 = 6;
  
  // 3. Calculate buffer size
  int buffer_size = HUB75Driver::calculateBufferSize(config);
  
  // 4. Initialize protocol
  i2sProtocol.init(config, buffer_size, &hardware, &bufferManager);
  
  // 5. Initialize driver
  display.init(config, &i2sProtocol);
  display.start();
  
  // 6. Draw graphics
  display.clear();
  display.fill(RGB(255, 0, 0));  // Red
  display.show();
  
  // 7. Animation loop
  while(true) {
    for(int x = 0; x < display.getWidth(); x++) {
      display.setPixel(x, 16, RGB(0, 255, 0));
      display.show();
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}
```

---

## Performance Notes

- **Frame composition**: ~2-5ms CPU time
- **Buffer swap**: <1µs
- **DMA transmission**: Continuous, no CPU
- **Refresh rate**: ~240Hz (varies with brightness)
- **Memory**: ~300KB total (framebuffer + DMA buffers)

---

## Thread Safety

- Driver is **NOT thread-safe**
- All operations should be called from the same task
- Protocol transmission happens asynchronously via DMA

---

## Error Handling

Methods return `bool` for success/failure:
- `true` = Success
- `false` = Error (check logs for details)

Errors are logged using `PLATFORM_LOG_E()`.
