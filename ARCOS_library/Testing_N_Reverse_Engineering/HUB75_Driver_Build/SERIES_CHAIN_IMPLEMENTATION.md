# Series Chain Expansion Implementation Summary

## Overview
Added series/daisy-chain expansion mode to the HUB75 driver, complementing the existing parallel OE expansion mode. This allows users to chain multiple LED panels together using a single OE pin, making large displays more practical.

## Changes Made

### 1. Configuration Structure (`hub75_driver.hpp`)

**Added ExpansionMode Enum:**
```cpp
enum class ExpansionMode {
  SINGLE,         // Single panel operation
  PARALLEL_OE,    // Multiple panels with independent OE pins
  SERIES_CHAIN    // Multiple panels daisy-chained with shared OE
};
```

**Updated HUB75Config:**
```cpp
struct HUB75Config {
  // New fields
  ExpansionMode expansion_mode = ExpansionMode::SINGLE;
  int panel_count = 1;
  
  // Deprecated but maintained for backward compatibility
  bool dual_display_mode = false;
  
  // Auto-calculated
  int effective_width;
};
```

### 2. Buffer Size Calculation (`hub75_driver.cpp`)

**Dynamic Buffer Sizing:**
```cpp
if(config.expansion_mode == HUB75Config::ExpansionMode::PARALLEL_OE){
  pixels_per_row = config.matrix_width * config.panel_count;
} else if(config.expansion_mode == HUB75Config::ExpansionMode::SERIES_CHAIN){
  pixels_per_row = config.matrix_width * config.panel_count;
} else if(config.dual_display_mode){
  // Legacy backward compatibility
  pixels_per_row = 128;
  config.expansion_mode = HUB75Config::ExpansionMode::PARALLEL_OE;
  config.panel_count = 2;
}

buffer_size = hub75_rows * config.colour_depth * (pixels_per_row + 1);
```

**Result:**
- SINGLE: 5,200 samples (64 pixels × 16 rows × 5 planes + delays)
- PARALLEL_OE (2 panels): 10,320 samples (128 pixels × 16 rows × 5 planes + delays)
- SERIES_CHAIN (3 panels): 15,440 samples (192 pixels × 16 rows × 5 planes + delays)

### 3. Data Generation Logic (`convertFramebufferToHUB75()`)

**Unified Pixel Reading:**
```cpp
// Simplified - works for all modes
int upper_index = upper_row * fb_width + col;
int lower_index = lower_row * fb_width + col;

upper_pixel = framebuffer[upper_index];
lower_pixel = framebuffer[lower_index];
```

**Mode-Specific OE Control:**

**PARALLEL_OE Mode:**
```cpp
// Independent OE per panel section
int panel_index = col / config.matrix_width;
int local_col = col % config.matrix_width;

if(panel_index == 0){
  if(local_col >= bcm_length){
    sample |= (1 << OE_BIT);  // Disable Panel 0 OE
  }
  sample |= (1 << OE2_BIT);  // Panel 1 disabled
} else if(panel_index == 1){
  sample |= (1 << OE_BIT);   // Panel 0 disabled
  if(local_col >= bcm_length){
    sample |= (1 << OE2_BIT);  // Disable Panel 1 OE
  }
}

// Latch at end of each panel section
if(local_col == config.matrix_width - 1){
  sample |= (1 << LAT_BIT);
}
```

**SERIES_CHAIN Mode:**
```cpp
// Shared OE for entire chain
if(col >= bcm_length){
  sample |= (1 << OE_BIT);  // Disable OE after BCM period
}

// Single latch at end of entire chain
if(col == total_columns - 1){
  sample |= (1 << LAT_BIT);
}
```

## Key Differences Between Modes

| Aspect | PARALLEL_OE | SERIES_CHAIN |
|--------|-------------|--------------|
| **OE Pins** | One per panel (OE, OE2, ...) | Single shared OE |
| **Latch Points** | End of each panel section | End of entire chain |
| **Data Flow** | Parallel to all panels | Sequential through chain |
| **BCM Application** | Per-panel independent | Unified for all panels |
| **Brightness Control** | Independent | Shared |
| **Scalability** | Limited by GPIO | Excellent (1 OE for all) |

## Hardware Wiring Differences

### PARALLEL_OE (2 panels):
```
ESP32 GPIO → Both Panels (RGB, CLK, LAT, ADDR)
GPIO 40 → Panel 0 OE
GPIO 39 → Panel 1 OE

Framebuffer Layout: [Panel0_64px][Panel1_64px]
Data clocked: 64 pixels → Latch → 64 pixels → Latch
OE timing: Independent per panel
```

### SERIES_CHAIN (3 panels):
```
ESP32 → Panel 0 (Data IN)
Panel 0 (Data OUT) → Panel 1 (Data IN)
Panel 1 (Data OUT) → Panel 2 (Data IN)
All panels share: CLK, LAT, OE, ADDR

Framebuffer Layout: [Panel0_64px][Panel1_64px][Panel2_64px]
Data clocked: 192 pixels → Single Latch
OE timing: Shared for entire chain
```

## Example Configurations

### Single Panel:
```cpp
HUB75Config config;
config.expansion_mode = HUB75Config::ExpansionMode::SINGLE;
config.panel_count = 1;
config.matrix_width = 64;
// Framebuffer: 64 × 32 pixels
```

### Dual Parallel OE:
```cpp
HUB75Config config;
config.expansion_mode = HUB75Config::ExpansionMode::PARALLEL_OE;
config.panel_count = 2;
config.matrix_width = 64;
config.pins.oe_pin = 40;
config.pins.oe_pin2 = 39;
// Framebuffer: 128 × 32 pixels
```

### Triple Series Chain:
```cpp
HUB75Config config;
config.expansion_mode = HUB75Config::ExpansionMode::SERIES_CHAIN;
config.panel_count = 3;
config.matrix_width = 64;
config.pins.oe_pin = 40;  // Shared
// Framebuffer: 192 × 32 pixels
```

## Backward Compatibility

The old `dual_display_mode` flag is maintained:
```cpp
if(config.dual_display_mode){
  // Automatically converts to new system
  pixels_per_row = 128;
  config.expansion_mode = HUB75Config::ExpansionMode::PARALLEL_OE;
  config.panel_count = 2;
}
```

Existing code continues to work without modification.

## Performance Characteristics

### Memory Usage:
```
Buffer = rows × color_depth × (pixels_per_row + 1) × sizeof(uint16_t)

SINGLE (1 panel):
= 16 × 5 × 65 × 2 = 10,400 bytes

PARALLEL_OE (2 panels):
= 16 × 5 × 129 × 2 = 20,640 bytes

SERIES_CHAIN (3 panels):
= 16 × 5 × 193 × 2 = 30,880 bytes
```

### Refresh Rate Impact:
- **PARALLEL_OE:** Minimal impact (multiple latches per row)
- **SERIES_CHAIN:** Proportional to panel_count (more pixels to clock)

### Signal Integrity:
- **PARALLEL_OE:** Short data paths to each panel
- **SERIES_CHAIN:** Data travels through multiple panels (may need buffering)

## Testing Recommendations

### Test Pattern for Series Chain (3 panels):
```cpp
void drawSeriesChainTest(RGBPixel* framebuffer) {
  int panel_width = 64;
  
  // Panel 0: Red rectangle (columns 0-63)
  for(int y = 8; y < 24; y++) {
    for(int x = 8; x < 56; x++) {
      framebuffer[y * 192 + x] = {255, 0, 0};
    }
  }
  
  // Panel 1: Green rectangle (columns 64-127)
  for(int y = 8; y < 24; y++) {
    for(int x = 72; x < 120; x++) {
      framebuffer[y * 192 + x] = {0, 255, 0};
    }
  }
  
  // Panel 2: Blue rectangle (columns 128-191)
  for(int y = 8; y < 24; y++) {
    for(int x = 136; x < 184; x++) {
      framebuffer[y * 192 + x] = {0, 0, 255};
    }
  }
}
```

### Debug Checklist:
1. ✅ Verify `effective_width = matrix_width × panel_count`
2. ✅ Check buffer size calculation matches panel count
3. ✅ Confirm framebuffer allocation matches expected pixels
4. ✅ Test BCM pattern generates correct OE timing
5. ✅ Validate latch points (end of section vs end of chain)

## Documentation Created

1. **EXPANSION_MODES.md** - Comprehensive guide covering:
   - All three expansion modes
   - Hardware wiring diagrams
   - Configuration examples
   - Performance comparisons
   - Troubleshooting guide
   - Migration paths

2. **SERIES_CHAIN_IMPLEMENTATION.md** (this file) - Technical implementation details

## Future Enhancements

Potential improvements:
1. **Dynamic Mode Switching** - Runtime mode changes
2. **Auto-Detection** - Detect panel count from hardware
3. **Mixed Mode** - Combine parallel and series (e.g., 2 series chains in parallel)
4. **Per-Panel Brightness** - Software dimming for series chains
5. **Signal Buffering** - Auto-insert buffers for long chains

## Benefits

✅ **Scalability:** Series chain supports many panels with minimal GPIO  
✅ **Flexibility:** Choose parallel for independent control or series for simplicity  
✅ **Backward Compatible:** Existing code continues to work  
✅ **Memory Efficient:** Dynamic buffer sizing based on configuration  
✅ **Performance:** Optimized for each expansion mode  

## Build Status

✅ Successfully compiled with ESP-IDF 5.5.0  
✅ No compilation errors  
✅ Memory usage within acceptable limits  
✅ Ready for hardware testing  
