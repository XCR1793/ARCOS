# API Reference

## HUB75Driver Class

Main driver class for HUB75 LED matrix control.

### Initialization & Control

#### `bool init(const HUB75Config& config)`
Initialize the driver with specified configuration.

**Parameters:**
- `config` - Configuration structure containing pins, display settings, etc.

**Returns:** `true` on success, `false` on failure

**Example:**
```cpp
HUB75Driver display;
HUB75Config config = HUB75Config::getDefault();
config.enable_gamma_correction = true;
if(!display.init(config)){
  ESP_LOGE(TAG, "Init failed");
}
```

---

#### `bool start()`
Start display refresh (begins DMA transfers).

**Returns:** `true` on success, `false` on failure

**Note:** Must call `init()` first

---

#### `void stop()`
Stop display refresh (halts DMA transfers).

**Note:** Pixels remain lit in last state

---

### Pixel Operations

#### `void setPixel(int x, int y, RGB color)`
Set a single pixel color.

**Parameters:**
- `x` - X coordinate (0-based)
- `y` - Y coordinate (0-based)
- `color` - RGB color structure

**Example:**
```cpp
display.setPixel(10, 15, RGB(255, 0, 0));  // Red pixel
```

**Note:** Changes take effect after `show()` is called

---

#### `void fillScreen(RGB color)`
Fill entire display with solid color.

**Parameters:**
- `color` - RGB color structure

**Example:**
```cpp
display.fillScreen(RGB(0, 0, 255));  // Blue screen
```

---

#### `void clearScreen()`
Clear display (set all pixels to black).

**Equivalent to:** `fillScreen(RGB(0, 0, 0))`

---

#### `void drawLine(int x0, int y0, int x1, int y1, RGB color)`
Draw a line between two points.

**Parameters:**
- `x0, y0` - Start coordinates
- `x1, y1` - End coordinates
- `color` - RGB color

**Example:**
```cpp
display.drawLine(0, 0, 127, 31, RGB(255, 255, 0));  // Yellow diagonal
```

---

### Display Control

#### `void show()`
Update the display with current framebuffer contents.

**Process:**
1. Converts RGB framebuffer → BCM format
2. Applies brightness control
3. Swaps DMA buffers
4. Hardware begins displaying new frame

**Example:**
```cpp
display.setPixel(10, 10, RGB(255, 0, 0));
display.setPixel(20, 20, RGB(0, 255, 0));
display.show();  // Now visible on panel
```

---

#### `void setBrightness(uint8_t brightness)`
Set global display brightness.

**Parameters:**
- `brightness` - Brightness level (0-255)
  - 0 = Off
  - 128 = 50%
  - 255 = Maximum

**Implementation:** Uses BCM timing fill-up strategy
- Allocates 64x base BCM cycles
- Fills only needed cycles with OE active
- Brightness mapped: 0-255 → 0-63 scale

**Example:**
```cpp
display.setBrightness(128);  // 50% brightness
display.show();
```

---

#### `uint8_t getBrightness() const`
Get current brightness level.

**Returns:** Current brightness (0-255)

---

### Configuration

#### `void setGammaCorrection(bool enabled, float gamma = 2.2f)`
Enable/disable gamma correction.

**Parameters:**
- `enabled` - Enable gamma correction
- `gamma` - Gamma value (1.8, 2.2, or 2.6)

**Example:**
```cpp
display.setGammaCorrection(true, 2.2f);
```

**Note:** Uses pre-computed lookup tables for performance

---

### Information

#### `int getWidth() const`
Get display width in pixels.

**Returns:** Width (typically 64 or 128 for dual panels)

---

#### `int getHeight() const`
Get display height in pixels.

**Returns:** Height (typically 32)

---

## Configuration Structures

### HUB75Config

Main configuration structure.

```cpp
struct HUB75Config {
  // Display dimensions
  int matrix_width;         // Panel width (64)
  int matrix_height;        // Panel height (32)
  int colour_depth;         // Bit depth (5 for BCM)
  
  // Pin configuration
  HUB75Pins pins;
  
  // Gamma correction
  bool enable_gamma_correction;  // Enable gamma
  float gamma_value;             // 1.8, 2.2, or 2.6
  
  // Expansion mode
  ExpansionMode expansion_mode;  // SINGLE, PARALLEL_OE, SERIES_CHAIN
  int panel_count;               // Number of panels
  
  // Panel inversions
  PanelInversion panel_inversions[MAX_PANELS];
  
  // Legacy dual display
  bool dual_display_mode;
  int effective_width;
  
  // Timing
  uint32_t clock_freq_hz;   // Clock frequency (20MHz)
  
  // Static factory method
  static HUB75Config getDefault();
};
```

**Example:**
```cpp
HUB75Config config = HUB75Config::getDefault();
config.enable_gamma_correction = true;
config.dual_display_mode = true;
config.effective_width = 128;
```

---

### HUB75Pins

Pin configuration structure.

```cpp
struct HUB75Pins {
  PinNumber r0_pin, g0_pin, b0_pin;  // Upper half RGB
  PinNumber r1_pin, g1_pin, b1_pin;  // Lower half RGB
  PinNumber a_pin, b_pin, c_pin;     // Row address
  PinNumber d_pin, e_pin;            // Row address (extended)
  PinNumber lat_pin;                 // Latch
  PinNumber oe_pin;                  // Output Enable 1
  PinNumber oe_pin2;                 // Output Enable 2 (dual panel)
  PinNumber clock_pin;               // Clock
};
```

---

### PanelInversion

Panel orientation correction.

```cpp
struct PanelInversion {
  bool flip_horizontal;  // Mirror horizontally
  bool flip_vertical;    // Mirror vertically
};
```

**Example:**
```cpp
config.panel_inversions[0].flip_vertical = true;   // Flip panel 0
config.panel_inversions[1].flip_vertical = false;  // Normal panel 1
```

---

### RGB

Color structure.

```cpp
struct RGB {
  uint8_t r, g, b;
  
  RGB();
  RGB(uint8_t red, uint8_t green, uint8_t blue);
};
```

**Example:**
```cpp
RGB red(255, 0, 0);
RGB green(0, 255, 0);
RGB blue(0, 0, 255);
RGB white(255, 255, 255);
RGB black(0, 0, 0);
RGB purple(128, 0, 128);
```

**Helper Macro:**
```cpp
#define RGB(r,g,b) (::RGB(r,g,b))

display.setPixel(0, 0, RGB(255, 128, 64));
```

---

## Expansion Modes

### ExpansionMode::SINGLE
Single panel operation.

**Configuration:**
```cpp
config.expansion_mode = HUB75Config::ExpansionMode::SINGLE;
config.panel_count = 1;
```

---

### ExpansionMode::PARALLEL_OE
Dual panels with independent OE pins.

**Features:**
- Independent panel control
- Simultaneous display
- Requires two OE pins

**Configuration:**
```cpp
config.expansion_mode = HUB75Config::ExpansionMode::PARALLEL_OE;
config.panel_count = 2;
config.pins.oe_pin = 35;
config.pins.oe_pin2 = 6;
```

---

### ExpansionMode::SERIES_CHAIN
Daisy-chained panels.

**Features:**
- Data flows through panels
- Single OE pin
- More panels possible

**Configuration:**
```cpp
config.expansion_mode = HUB75Config::ExpansionMode::SERIES_CHAIN;
config.panel_count = 2;  // or more
```

---

## Platform Abstraction

### IPlatformHAL

Abstract platform interface for porting.

```cpp
class IPlatformHAL {
public:
  // Memory management
  virtual void* allocateMemory(size_t size, uint32_t caps) = 0;
  virtual void freeMemory(void* ptr) = 0;
  
  // GPIO
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

**To port to new platform:**
1. Implement `IPlatformHAL` interface
2. Implement `IParallelHardware` interface
3. Implement `IDmaBufferManager` interface

---

## Advanced Usage

### Custom Animation Loop

```cpp
void animationLoop(){
  while(true){
    // Update pixel data
    for(int x = 0; x < display.getWidth(); x++){
      for(int y = 0; y < display.getHeight(); y++){
        RGB color = calculateColor(x, y, frame_count);
        display.setPixel(x, y, color);
      }
    }
    
    // Display frame
    display.show();
    
    // Control frame rate
    vTaskDelay(pdMS_TO_TICKS(16));  // ~60 FPS
    frame_count++;
  }
}
```

---

### Brightness Fade

```cpp
void fadeBrightness(){
  for(int brightness = 0; brightness <= 255; brightness++){
    display.setBrightness(brightness);
    display.show();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
```

---

### Drawing Primitives

```cpp
// Horizontal line
void drawHLine(int x, int y, int length, RGB color){
  for(int i = 0; i < length; i++){
    display.setPixel(x + i, y, color);
  }
}

// Vertical line
void drawVLine(int x, int y, int length, RGB color){
  for(int i = 0; i < length; i++){
    display.setPixel(x, y + i, color);
  }
}

// Rectangle
void drawRect(int x, int y, int w, int h, RGB color){
  drawHLine(x, y, w, color);
  drawHLine(x, y + h - 1, w, color);
  drawVLine(x, y, h, color);
  drawVLine(x + w - 1, y, h, color);
}

// Filled rectangle
void fillRect(int x, int y, int w, int h, RGB color){
  for(int i = 0; i < h; i++){
    drawHLine(x, y + i, w, color);
  }
}
```

---

## Error Handling

**Check return values:**
```cpp
if(!display.init(config)){
  ESP_LOGE(TAG, "Display init failed");
  return;
}

if(!display.start()){
  ESP_LOGE(TAG, "Display start failed");
  return;
}
```

**Bounds checking:**
```cpp
void safeSetPixel(int x, int y, RGB color){
  if(x >= 0 && x < display.getWidth() && 
     y >= 0 && y < display.getHeight()){
    display.setPixel(x, y, color);
  }
}
```

---

## Performance Tips

1. **Minimize `show()` calls** - Group updates together
2. **Use `fillScreen()` for solid colors** - Faster than pixel-by-pixel
3. **Pre-calculate colors** - Don't do math in render loop
4. **Target 60 FPS or less** - Higher refresh unnecessary
5. **Use lower brightness for power savings** - Exponential power reduction

---

## Memory Management

**Framebuffer size:**
```
Width × Height × 3 bytes (RGB)
Example: 128 × 32 × 3 = 12,288 bytes
```

**DMA buffer size:**
```
~130KB per buffer × 2 buffers = ~260KB
Allocated from DMA-capable RAM (SRAM)
```

**Total RAM usage:** ~14KB (stack + framebuffer)
