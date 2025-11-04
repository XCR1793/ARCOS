# HUB75 LED Matrix Display Driver

## Overview

This directory contains the complete HUB75 LED matrix display driver implementation for the ARCOS framework. The driver supports both low-level manual control and a simplified high-level interface.

**⭐ RECOMMENDED: Use `SimpleHUB75Display` for all applications unless you need advanced features like custom protocols or direct buffer management.**

## Quick Start (Simplified Driver - RECOMMENDED)

### Minimal Example - Get Pixels on Screen

```cpp
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "abstraction/drivers/components/HUB75/driver_hub75_simple.hpp"

using namespace arcos::abstraction::drivers;

extern "C" void app_main(){
  // Create display
  SimpleHUB75Display display;
  
  // Initialize (uses sensible defaults)
  display.begin();
  
  // Draw pixels
  display.setPixel(10, 10, RGB(255, 0, 0));  // Red pixel at (10, 10)
  display.show();  // Update display
  
  // Done! Your pixel is now on screen
}
```

That's it! Just **3 lines** to get pixels on screen:
1. Create display object
2. Call `begin()`
3. Draw and `show()`

## Driver Architecture

### Files in this Directory

| File | Purpose | Recommended For |
|------|---------|-----------------|
| **driver_hub75_simple.hpp** ⭐ | Simplified high-level driver | **99% of applications** |
| **driver_hub75.hpp** | Core display driver with framebuffer management | Advanced control |
| **driver_hub75_impl.hpp** | Implementation details for core driver | Internal use |
| **driver_hub75_protocol.hpp** | Abstract protocol interface (I2S, GPIO, etc.) | Custom protocols |
| **driver_hub75_i2s.hpp** | I2S/LCD_CAM protocol implementation | Platform porting |
| **driver_hub75_i2s_impl.hpp** | Implementation details for I2S protocol | Internal use |

### Architecture Layers

```
┌─────────────────────────────────────┐
│  SimpleHUB75Display (Simplified)    │  ← Use this for easy setup
├─────────────────────────────────────┤
│  HUB75Driver (Core)                 │  ← Framebuffer & pixel management
├─────────────────────────────────────┤
│  IHUB75Protocol (Abstract)          │  ← Protocol abstraction
├─────────────────────────────────────┤
│  HUB75_I2S_Protocol (Concrete)      │  ← I2S/LCD_CAM implementation
├─────────────────────────────────────┤
│  HAL (Hardware Abstraction)         │  ← Platform-specific hardware
└─────────────────────────────────────┘
```

## Simplified Driver (SimpleHUB75Display)

### Why Use the Simplified Driver?

**✅ Recommended for all standard applications**

The `SimpleHUB75Display` class handles all the complexity automatically:
- ✅ **Automatic initialization** - No manual hardware setup
- ✅ **Memory management** - Handles allocation/deallocation
- ✅ **Dual OE support** - Just pass `true`/`false`
- ✅ **Sensible defaults** - Works out of the box
- ✅ **Clean API** - Simple, intuitive methods
- ✅ **RAII pattern** - Automatic cleanup on destruction

**Use the manual driver only if you need:**
- Custom protocol implementations
- Direct buffer access for DMA optimization
- Platform porting to new hardware
- Maximum performance control

### SimpleHUB75Display Implementation

The simplified driver is a wrapper that automatically manages:

```cpp
class SimpleHUB75Display{
private:
  HAL_PARALLEL_DEFAULT* hardware_;        // Hardware platform
  ParallelBuffer* buffer_manager_;         // DMA buffer manager
  HUB75_I2S_Protocol* protocol_;           // I2S/LCD_CAM protocol
  HUB75Driver* driver_;                    // Core display driver
  HUB75Config config_;                     // Configuration
  bool initialized_;                       // State tracking
  
public:
  // Automatic initialization
  bool begin(bool dual_oe = true, HUB75Config config = HUB75Config::getDefault());
  
  // Automatic cleanup
  void end();
  
  // Simple drawing API
  void setPixel(int x, int y, const RGB& color);
  void clear();
  void fill(const RGB& color);
  void show();
  
  // Utility functions
  int getWidth() const;
  int getHeight() const;
  void setBrightness(uint8_t brightness);
  
  // Advanced access if needed
  HUB75Driver* getDriver();
};
```

**What it does automatically:**
1. Creates hardware abstraction objects (`HAL_PARALLEL_DEFAULT`)
2. Initializes DMA buffer manager (`ParallelBuffer`)
3. Sets up I2S protocol (`HUB75_I2S_Protocol`)
4. Configures display driver (`HUB75Driver`)
5. Calculates buffer sizes
6. Starts display transmission
7. Cleans up on destruction

**Result:** You write **3 lines** instead of **50+ lines** of boilerplate!

### Initialization Methods

#### Method 1: Default (Recommended)
```cpp
SimpleHUB75Display display;
display.begin();  // Dual OE mode, gamma correction enabled
```

#### Method 2: Single/Dual OE Control
```cpp
SimpleHUB75Display display;
display.begin(true);   // Dual OE: 2 panels via oe_pin + oe_pin2 (default)
display.begin(false);  // Single OE: 1 panel via oe_pin only
```

#### Method 3: With Preset Configuration
```cpp
SimpleHUB75Display display;
display.begin(true, hub75_presets::singlePanel());
display.begin(true, hub75_presets::dualPanelHorizontal());
display.begin(true, hub75_presets::highBrightness());
display.begin(true, hub75_presets::lowPower());
```

#### Method 4: Custom Configuration
```cpp
HUB75Config config = HUB75Config::getDefault();
config.colour_depth = 8;
config.enable_gamma_correction = false;
config.matrix_width = 64;
config.matrix_height = 32;

SimpleHUB75Display display;
display.begin(true, config);
```

### Drawing Functions

```cpp
// Set individual pixel
display.setPixel(x, y, RGB(r, g, b));

// Get pixel color
RGB color = display.getPixel(x, y);

// Clear entire display
display.clear();

// Fill with solid color
display.fill(RGB(255, 0, 0));  // Fill red

// Update display (required after drawing)
display.show();
```

### Display Control

```cpp
// Get dimensions
int width = display.getWidth();    // 64 or 128 depending on mode
int height = display.getHeight();  // Typically 32

// Brightness control (0-255)
display.setBrightness(128);  // 50% brightness
uint8_t level = display.getBrightness();

// Check initialization status
bool ready = display.isReady();

// Access configuration
const HUB75Config& config = display.getConfig();

// Shutdown and cleanup
display.end();
```

## Configuration Reference

### HUB75Config Structure

#### Display Dimensions
```cpp
config.matrix_width = 64;   // Panel width in pixels (default: 64)
config.matrix_height = 32;  // Panel height in pixels (default: 32)
```

#### Panel Expansion Modes
```cpp
enum class ExpansionMode{
  SINGLE,        // Single panel (64x32)
  PARALLEL_OE,   // Multiple panels via separate OE pins
  SERIES_CHAIN   // Multiple panels daisy-chained
};

config.expansion_mode = HUB75Config::ExpansionMode::SINGLE;
config.panel_count = 1;  // Number of panels (1-4 typical)
```

#### Dual OE Mode (Parallel Panel Control)
```cpp
config.dual_display_mode = true;  // Enable dual OE control
config.effective_width = 128;     // Total width (64 * 2)
```

**Note:** Dual OE uses `oe_pin` and `oe_pin2` to control 2 panels in parallel.

#### Panel Orientation (Per-Panel Flipping)
```cpp
config.panel_inversions[0].flip_horizontal = false;
config.panel_inversions[0].flip_vertical = true;   // Flip panel 0
config.panel_inversions[1].flip_horizontal = false;
config.panel_inversions[1].flip_vertical = false;  // Panel 1 normal
config.panel_inversions[2].flip_horizontal = false;
config.panel_inversions[2].flip_vertical = false;
config.panel_inversions[3].flip_horizontal = false;
config.panel_inversions[3].flip_vertical = false;
```

#### Color and Visual Quality
```cpp
config.colour_depth = 5;              // Bits per channel (1-8, default: 5)
config.enable_gamma_correction = true; // Apply gamma curve
config.gamma_value = 2.2f;            // Gamma value (1.8, 2.2, or 2.6)
config.enable_anti_aliasing = false;  // 2x2 supersampling (experimental)
```

**Color Depth Guide:**
- `1-3 bits`: Low color, fast refresh (good for text/simple graphics)
- `4-5 bits`: Balanced (default, good for most uses)
- `6-8 bits`: High color depth, slower refresh (photos/gradients)

#### Buffer Configuration
```cpp
config.enable_double_buffering = true;  // Prevent tearing (recommended)
config.colour_buffer_count = 5;         // Number of BCM planes (default: 5)
```

#### Hardware Settings
```cpp
config.clock_freq_hz = 10000000;  // Clock frequency (default: 10 MHz)
                                   // Range: 5-20 MHz typical
```

#### GPIO Pin Mapping
```cpp
// Data pins
config.pins.r0_pin = 7;    // Red channel, upper half
config.pins.g0_pin = 15;   // Green channel, upper half
config.pins.b0_pin = 16;   // Blue channel, upper half
config.pins.r1_pin = 17;   // Red channel, lower half
config.pins.g1_pin = 18;   // Green channel, lower half
config.pins.b1_pin = 8;    // Blue channel, lower half

// Control pins
config.pins.lat_pin = 36;   // Latch signal
config.pins.oe_pin = 35;    // Output Enable (primary)
config.pins.oe_pin2 = 6;    // Output Enable (secondary, for dual OE)
config.pins.clock_pin = 37; // Clock signal

// Address pins
config.pins.a_pin = 41;  // Row address A (bit 0)
config.pins.b_pin = 40;  // Row address B (bit 1)
config.pins.c_pin = 39;  // Row address C (bit 2)
config.pins.d_pin = 38;  // Row address D (bit 3)
config.pins.e_pin = 42;  // Row address E (bit 4, for 64-row panels)
```

**Pin Configuration Notes:**
- Set `oe_pin2 = PIN_NC` to disable dual OE mode
- Pin E only needed for panels > 32 rows
- All pins must be valid GPIO numbers for your platform

#### Advanced Timing
```cpp
config.timing.latch_blanking = 1;     // Blanking during latch
config.timing.output_blanking = 1;    // OE blanking time
config.timing.continuous_mode = true; // Continuous looping
```

## Configuration Presets

### Available Presets

#### Single Panel (64x32)
```cpp
HUB75Config config = hub75_presets::singlePanel();
display.begin(false, config);  // Single OE mode
```

**Settings:**
- 64x32 pixels
- Single OE pin
- Gamma correction enabled (2.2)

#### Dual Panel Horizontal (128x32)
```cpp
HUB75Config config = hub75_presets::dualPanelHorizontal();
display.begin(true, config);  // Dual OE mode
```

**Settings:**
- 128x32 pixels (2 panels side-by-side)
- Dual OE pins (oe_pin + oe_pin2)
- Panel 0 flipped vertically
- Gamma correction enabled (2.2)

#### High Brightness
```cpp
HUB75Config config = hub75_presets::highBrightness();
display.begin(true, config);
```

**Settings:**
- Dual panel mode
- 8-bit color depth (maximum)
- Gamma correction **disabled** (for max brightness)

#### Low Power
```cpp
HUB75Config config = hub75_presets::lowPower();
display.begin(true, config);
```

**Settings:**
- Dual panel mode
- 4-bit color depth (reduced)
- Lower clock speed (5 MHz)

## Feature Reference

### 1. Pixel Drawing
```cpp
// Basic drawing
display.setPixel(x, y, RGB(r, g, b));
RGB color = display.getPixel(x, y);
display.clear();
display.fill(RGB(255, 0, 0));
```

### 2. Brightness Control (BCM)
```cpp
// Scales display duration, not pixel values
// 0 = minimum, 255 = maximum
display.setBrightness(200);
```

**Note:** This adjusts the Binary Code Modulation (BCM) timing, affecting overall display brightness without modifying pixel color values.

### 3. Gamma Correction
```cpp
// Enable/disable at runtime
display.getDriver()->setGammaCorrection(true, 2.2f);
display.getDriver()->setGammaCorrection(false);  // Disable

// Check status
bool enabled = display.getDriver()->isGammaCorrectionEnabled();
```

**Gamma values:**
- `1.8`: Brighter, less contrast
- `2.2`: Standard (sRGB)
- `2.6`: Darker, more contrast

### 4. Dual OE Mode (Parallel Panels)
```cpp
// Enable automatically via begin()
display.begin(true);  // Dual OE enabled

// Or via config
HUB75Config config = HUB75Config::getDefault();
config.dual_display_mode = true;
config.effective_width = 128;
config.pins.oe_pin2 = 6;
display.begin(true, config);
```

**How it works:**
- Uses TWO Output Enable pins: `oe_pin` (primary) and `oe_pin2` (secondary)
- Each pin controls one panel independently
- Both panels receive same data, but different OE timing
- Allows 2 panels side-by-side (128x32 total)

### 5. Panel Orientation/Inversion
```cpp
HUB75Config config = HUB75Config::getDefault();
config.panel_inversions[0].flip_vertical = true;    // Flip panel 0 upside down
config.panel_inversions[0].flip_horizontal = false;
config.panel_inversions[1].flip_vertical = false;   // Panel 1 normal
display.begin(true, config);
```

**Use cases:**
- Physical panel mounting orientation
- Hardware-specific quirks
- Matching panel directions

### 6. Color Depth Control
```cpp
HUB75Config config = HUB75Config::getDefault();
config.colour_depth = 8;  // 1-8 bits per channel
display.begin(true, config);
```

**Trade-offs:**
- **Higher depth**: Better colors, slower refresh
- **Lower depth**: Faster refresh, fewer colors

### 7. Advanced: Direct Framebuffer Access
```cpp
// Get underlying driver for advanced operations
HUB75Driver* driver = display.getDriver();

// Get framebuffer info
FrameBuffer fb = driver->getFrameBuffer();
// fb.pixels - pointer to RGB array
// fb.width, fb.height - dimensions
// fb.size_bytes - total size

// Bulk upload
RGB* my_buffer = /* ... */;
driver->uploadFrameBuffer(my_buffer, width, height);

// Copy framebuffer out
RGB* dest = new RGB[width * height];
driver->copyFrameBuffer(dest);
delete[] dest;
```

### 8. Runtime Configuration Updates
```cpp
// Get current config
const HUB75Config& current = display.getConfig();

// Modify and apply
HUB75Config new_config = current;
new_config.colour_depth = 6;

// Update (may require restart)
display.getDriver()->updateConfig(new_config);
```

## Complete Setup Examples

### Example 1: Ultra Minimal (Color Cycle)
```cpp
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "abstraction/drivers/components/HUB75/driver_hub75_simple.hpp"

using namespace arcos::abstraction::drivers;

extern "C" void app_main(){
  SimpleHUB75Display display;
  display.begin();
  
  while(true){
    display.fill(RGB(255, 0, 0)); display.show(); vTaskDelay(pdMS_TO_TICKS(1000));
    display.fill(RGB(0, 255, 0)); display.show(); vTaskDelay(pdMS_TO_TICKS(1000));
    display.fill(RGB(0, 0, 255)); display.show(); vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
```

### Example 2: Moving Pixel
```cpp
extern "C" void app_main(){
  SimpleHUB75Display display;
  display.begin();
  
  int x = 0;
  while(true){
    display.clear();
    display.setPixel(x, 16, RGB(255, 255, 0));
    display.show();
    
    x = (x + 1) % display.getWidth();
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
```

### Example 3: Custom Configuration
```cpp
extern "C" void app_main(){
  // Create custom config
  HUB75Config config = HUB75Config::getDefault();
  config.colour_depth = 6;                    // Higher color depth
  config.enable_gamma_correction = true;
  config.gamma_value = 2.2f;
  config.clock_freq_hz = 15000000;            // Faster clock
  config.dual_display_mode = true;
  config.effective_width = 128;
  config.pins.oe_pin2 = 6;                    // Enable second OE
  config.panel_inversions[0].flip_vertical = true;
  
  // Initialize with custom config
  SimpleHUB75Display display;
  if(!display.begin(true, config)){
    // Handle error
    return;
  }
  
  // Draw...
  display.fill(RGB(128, 128, 128));
  display.show();
}
```

### Example 4: Single Panel (No Dual OE)
```cpp
extern "C" void app_main(){
  SimpleHUB75Display display;
  display.begin(false);  // Single OE mode (64x32)
  
  // Width is now 64, not 128
  for(int x = 0; x < display.getWidth(); x++){
    for(int y = 0; y < display.getHeight(); y++){
      display.setPixel(x, y, RGB(x * 4, y * 8, 128));
    }
  }
  display.show();
}
```

### Example 5: High Brightness Mode
```cpp
extern "C" void app_main(){
  SimpleHUB75Display display;
  display.begin(true, hub75_presets::highBrightness());
  
  // Maximum brightness and color depth
  display.setBrightness(255);
  display.fill(RGB(255, 255, 255));  // Full white
  display.show();
}
```

### Example 6: Animation with Brightness Control
```cpp
extern "C" void app_main(){
  SimpleHUB75Display display;
  display.begin();
  
  uint8_t brightness = 0;
  bool increasing = true;
  
  while(true){
    display.fill(RGB(255, 0, 0));
    display.setBrightness(brightness);
    display.show();
    
    if(increasing){
      brightness += 5;
      if(brightness >= 250) increasing = false;
    }else{
      brightness -= 5;
      if(brightness <= 5) increasing = true;
    }
    
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
```

## Valid Configuration Values

### Constraints and Limits

| Parameter | Valid Range | Default | Notes |
|-----------|-------------|---------|-------|
| `matrix_width` | 16-128 | 64 | Must be multiple of 8 |
| `matrix_height` | 8-64 | 32 | Typically 16, 32, or 64 |
| `colour_depth` | 1-8 | 5 | Higher = better colors, slower |
| `gamma_value` | 1.0-3.0 | 2.2 | Typically 1.8, 2.2, or 2.6 |
| `clock_freq_hz` | 5M-20M | 10M | Platform dependent |
| `panel_count` | 1-4 | 1 | For chained displays |
| Brightness | 0-255 | 255 | BCM scaling |

### Pin Number Constraints
- Must be valid GPIO for your platform
- Cannot conflict with reserved pins
- Use `PIN_NC` (-1) to disable optional pins

### Memory Requirements

**Per Panel (64x32):**
- Framebuffer: ~12 KB (64 * 32 * 3 bytes)
- DMA buffers: ~60 KB (depends on color depth)
- Total: ~72 KB per panel

**Dual Panel (128x32):**
- Framebuffer: ~24 KB
- DMA buffers: ~120 KB
- Total: ~144 KB

## Troubleshooting

### Display Not Initializing
```cpp
if(!display.begin()){
  // Check:
  // 1. Platform flag defined (TARGET_ESP32_Wroom32S3_Module)
  // 2. Pin connections correct
  // 3. Power supply sufficient (5V, 2-4A per panel)
  // 4. Memory available
}
```

### Only One Panel Working (Dual OE)
```cpp
// Ensure oe_pin2 is set
HUB75Config config = HUB75Config::getDefault();
config.pins.oe_pin2 = 6;  // Must be set!
display.begin(true, config);
```

### Colors Look Wrong
```cpp
// Try adjusting gamma
display.getDriver()->setGammaCorrection(true, 2.2f);  // Standard
display.getDriver()->setGammaCorrection(true, 1.8f);  // Brighter
display.getDriver()->setGammaCorrection(true, 2.6f);  // Darker

// Or disable
display.getDriver()->setGammaCorrection(false);
```

### Flickering Display
```cpp
// Increase clock speed
HUB75Config config = HUB75Config::getDefault();
config.clock_freq_hz = 20000000;  // Try faster clock

// Reduce color depth
config.colour_depth = 4;  // Faster refresh

// Enable double buffering
config.enable_double_buffering = true;

display.begin(true, config);
```

### Performance Issues
```cpp
// Optimize:
// 1. Lower color depth (4-5 bits)
config.colour_depth = 4;

// 2. Batch drawing operations
display.clear();
for(...) display.setPixel(...);  // Draw everything
display.show();  // One update

// 3. Don't clear if not needed
// display.clear();  // Skip if drawing over everything

// 4. Target 60 FPS
vTaskDelay(pdMS_TO_TICKS(16));  // ~60 Hz
```

## Advanced: Manual Driver Usage

**⚠️ NOT RECOMMENDED for standard applications - Use `SimpleHUB75Display` instead!**

If you need full control, you can use the low-level driver directly:

### Manual Initialization (50+ lines vs 3 lines with SimpleHUB75Display)

```cpp
#include "abstraction/hal.hpp"
#include "abstraction/drivers/components/HUB75/driver_hub75.hpp"
#include "abstraction/drivers/components/HUB75/driver_hub75_i2s.hpp"

using namespace arcos::abstraction;
using namespace arcos::abstraction::drivers;

// Create hardware objects (you must manage these)
static HAL_PARALLEL_DEFAULT hardware;
static ParallelBuffer buffer_manager;
static HUB75_I2S_Protocol protocol;
static HUB75Driver display;

extern "C" void app_main(){
  // Manual configuration
  HUB75Config config = HUB75Config::getDefault();
  config.enable_gamma_correction = true;
  config.gamma_value = 2.2f;
  config.dual_display_mode = true;
  config.effective_width = 128;
  config.panel_inversions[0].flip_vertical = true;
  config.panel_inversions[1].flip_vertical = false;
  config.pins.r0_pin = 7;
  config.pins.g0_pin = 15;
  config.pins.b0_pin = 16;
  config.pins.r1_pin = 17;
  config.pins.g1_pin = 18;
  config.pins.b1_pin = 8;
  config.pins.lat_pin = 36;
  config.pins.oe_pin = 35;
  config.pins.oe_pin2 = 6;  // Must set manually!
  config.pins.a_pin = 41;
  config.pins.b_pin = 40;
  config.pins.c_pin = 39;
  config.pins.d_pin = 38;
  config.pins.e_pin = 42;
  config.pins.clock_pin = 37;
  
  // Manual initialization steps
  int buffer_size = HUB75Driver::calculateBufferSize(config);
  
  if(!protocol.init(config, buffer_size, &hardware, &buffer_manager)){
    // Handle error
    return;
  }
  
  if(!display.init(config, &protocol)){
    // Handle error
    return;
  }
  
  if(!display.start()){
    // Handle error
    return;
  }
  
  // Finally, draw...
  display.setPixel(10, 10, RGB(255, 0, 0));
  display.show();
  
  // You must also manually cleanup (not shown)
}
```

### Compare: SimpleHUB75Display (3 lines)

```cpp
extern "C" void app_main(){
  SimpleHUB75Display display;
  display.begin();  // All the above complexity handled automatically!
  
  display.setPixel(10, 10, RGB(255, 0, 0));
  display.show();
}
```

### When to Use Manual Driver

**Only use manual driver if you absolutely need:**

1. **Custom Protocol Implementation**
   - Implementing GPIO bit-banging instead of I2S
   - Using different hardware peripheral (not LCD_CAM)
   - Platform porting to non-ESP32 systems

2. **Advanced Buffer Management**
   - Custom DMA buffer allocation strategies
   - Zero-copy buffer sharing with other systems
   - Memory-constrained optimization

3. **Protocol-Level Control**
   - Modifying I2S timing parameters at runtime
   - Implementing custom refresh strategies
   - Hardware-specific workarounds

4. **Maximum Performance**
   - Bypassing abstraction layers
   - Direct hardware register access
   - Cycle-accurate timing requirements

**For 99% of applications: Use `SimpleHUB75Display`!**

### Manual Driver Responsibilities

If you use the manual driver, **you must**:
- ✅ Create and manage all hardware objects
- ✅ Configure all pins manually
- ✅ Calculate buffer sizes
- ✅ Initialize protocol and driver in correct order
- ✅ Handle all errors explicitly
- ✅ Manually cleanup on shutdown
- ✅ Ensure object lifetimes are correct

With `SimpleHUB75Display`, all of this is automatic! ✨

## Summary

### ⭐ Driver Selection Guide

| Feature | SimpleHUB75Display | Manual Driver |
|---------|-------------------|---------------|
| **Ease of Use** | ⭐⭐⭐⭐⭐ 3 lines | ⭐ 50+ lines |
| **Memory Management** | ✅ Automatic | ❌ Manual |
| **Error Handling** | ✅ Built-in | ❌ Manual |
| **Cleanup** | ✅ Automatic | ❌ Manual |
| **Recommended For** | **99% of apps** | Custom protocols |
| **Performance** | Identical | Identical |
| **Flexibility** | High | Maximum |

**Recommendation:** Start with `SimpleHUB75Display`. Only use manual driver if you have specific needs that require it.

### Recommended Workflow

1. **Start Simple**: Use `SimpleHUB75Display` with defaults
2. **Test**: Get pixels on screen first
3. **Configure**: Adjust settings as needed (dual OE, brightness, etc.)
4. **Optimize**: Tune color depth and clock speed for your needs
5. **Advanced**: Only switch to manual driver if absolutely necessary

### Key Takeaways

- ⭐ **USE SimpleHUB75Display** - Recommended for all standard applications
- ✅ **3 lines to pixels on screen** - Minimal boilerplate
- ✅ **Automatic everything** - Initialization, memory, cleanup
- ✅ Dual OE mode is **automatic** (just pass `true` to `begin()`)
- ✅ Gamma correction **enabled by default** (looks better)
- ✅ Pin configuration **auto-set** for ESP32-S3
- ✅ Call `show()` after drawing to update display
- ✅ Use presets for common configurations
- ⚠️ **Avoid manual driver** unless you need custom protocols

### Performance Tips

1. Lower color depth = faster refresh
2. Batch drawing operations
3. Enable double buffering
4. Target 60 FPS (16ms per frame)
5. Don't clear if not needed

## License

Part of the ARCOS library by XCR1793 (Feather Forge)
