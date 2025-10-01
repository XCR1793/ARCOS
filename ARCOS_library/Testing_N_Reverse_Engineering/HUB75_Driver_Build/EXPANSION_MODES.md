# HUB75 Panel Expansion Modes

This driver supports multiple modes for expanding the LED matrix display beyond a single 64x32 panel.

## Overview

There are three expansion modes available:

1. **SINGLE** - Standard single panel operation
2. **PARALLEL_OE** - Multiple panels with independent OE control (parallel addressing)
3. **SERIES_CHAIN** - Multiple panels daisy-chained with shared OE control

## Expansion Mode Details

### SINGLE Mode
```
Standard operation for a single 64x32 HUB75 panel.
```

**Configuration:**
```cpp
config.expansion_mode = HUB75Config::ExpansionMode::SINGLE;
config.panel_count = 1;
config.matrix_width = 64;
```

**Hardware:**
- Single panel connected
- Standard HUB75 pinout
- One OE pin controls brightness

**Use Case:** Basic LED matrix applications

---

### PARALLEL_OE Mode
```
Multiple panels side-by-side with independent OE control.
Each panel has its own OE pin for independent brightness control.
```

**Configuration:**
```cpp
config.expansion_mode = HUB75Config::ExpansionMode::PARALLEL_OE;
config.panel_count = 2;  // or more
config.matrix_width = 64;
config.pins.oe_pin = 40;   // Panel 0 OE
config.pins.oe_pin2 = 39;  // Panel 1 OE
```

**Hardware Wiring:**
- Panel 0: Standard HUB75 connection with OE on pin 40
- Panel 1: Shares RGB, address, clock, latch BUT has separate OE on pin 39
- Data flows to both panels simultaneously
- Each panel section (64 pixels) gets latched independently

**Data Flow:**
```
Clock 64 pixels → Latch Panel 0 → OE controls Panel 0 brightness
Clock 64 pixels → Latch Panel 1 → OE2 controls Panel 1 brightness
```

**Memory:**
- Framebuffer: (64 × panel_count) × 32 pixels
- Example for 2 panels: 128 × 32 = 4,096 RGB pixels

**Advantages:**
- Independent brightness control per panel
- Parallel data output (all panels update simultaneously)
- Good for side-by-side arrangements

**Limitations:**
- Requires additional OE pins (one per panel)
- Limited scalability (ESP32-S3 has limited GPIO)
- More complex wiring

---

### SERIES_CHAIN Mode
```
Multiple panels daisy-chained where data flows through panels sequentially.
All panels share a single OE pin.
```

**Configuration:**
```cpp
config.expansion_mode = HUB75Config::ExpansionMode::SERIES_CHAIN;
config.panel_count = 3;  // can chain many panels
config.matrix_width = 64;
config.pins.oe_pin = 40;  // Shared by all panels
```

**Hardware Wiring:**
- Panel 0: Data IN from ESP32
- Panel 1: Data IN from Panel 0 Data OUT
- Panel 2: Data IN from Panel 1 Data OUT
- All panels share OE, LAT, CLK from ESP32
- Address lines (A-E) shared by all panels

**Data Flow:**
```
Clock 192 pixels (64 × 3) through chain → Single latch → OE controls all panels
```

**Memory:**
- Framebuffer: (64 × panel_count) × 32 pixels
- Example for 3 panels: 192 × 32 = 6,144 RGB pixels

**Advantages:**
- Only requires one OE pin (highly scalable)
- Simpler wiring (data flows through panels)
- Can chain many panels (limited by clock speed and signal integrity)
- Lower GPIO requirements

**Limitations:**
- All panels share same brightness (single OE)
- Data must clock through entire chain
- Longer chains may need signal buffering

---

## Choosing the Right Mode

| Factor | SINGLE | PARALLEL_OE | SERIES_CHAIN |
|--------|--------|-------------|--------------|
| **Panel Count** | 1 | 2-4 | 2-8+ |
| **GPIO Usage** | Low | High (1 OE per panel) | Low (1 OE total) |
| **Brightness Control** | Single | Independent per panel | Shared |
| **Wiring Complexity** | Simple | Medium | Simple |
| **Update Speed** | Fast | Fast | Medium (longer chain) |
| **Scalability** | N/A | Limited by GPIO | Excellent |

### Recommended Use Cases:

**PARALLEL_OE:**
- Need independent brightness per section
- Only 2-4 panels
- Have sufficient GPIO pins available
- Side-by-side panel arrangement

**SERIES_CHAIN:**
- Large displays (4+ panels)
- Limited GPIO availability
- Uniform brightness across all panels
- Long horizontal or vertical arrangements

---

## BCM Protocol with Expansion

Both PARALLEL_OE and SERIES_CHAIN modes use Binary Code Modulation (BCM) for brightness control:

### PARALLEL_OE BCM:
- Each panel section gets BCM pattern applied to its OE pin
- `bcm_length = 2^plane` cycles per bit plane
- Independent timing allows panel-specific dimming

### SERIES_CHAIN BCM:
- BCM pattern applied to shared OE pin
- All panels in chain receive same brightness
- `bcm_length = 2^plane` cycles for entire chain
- Single latch pulse after all data clocked through

---

## Example Configurations

### Dual Display (Legacy Compatibility)
```cpp
config.dual_display_mode = true;  // Deprecated but still works
// Automatically maps to:
// config.expansion_mode = PARALLEL_OE
// config.panel_count = 2
```

### Triple Series Chain
```cpp
HUB75Config config;
config.expansion_mode = HUB75Config::ExpansionMode::SERIES_CHAIN;
config.panel_count = 3;
config.matrix_width = 64;
config.matrix_height = 32;
config.colour_depth = 5;

// Framebuffer will be 192 × 32 pixels
driver.init(&hardware, &buffer_manager, config);
```

### Quad Parallel OE
```cpp
HUB75Config config;
config.expansion_mode = HUB75Config::ExpansionMode::PARALLEL_OE;
config.panel_count = 4;
config.matrix_width = 64;

// Would need to extend driver to support oe_pin3, oe_pin4
// Currently supports 2 panels via oe_pin and oe_pin2
```

---

## Performance Considerations

### Series Chain Performance:
- Clock rate: Same as single panel
- Data throughput: `64 × panel_count` pixels per row
- Refresh rate: Inversely proportional to panel_count
- Example: 3-panel chain = 3× data per row

### PARALLEL_OE Performance:
- Clock rate: Same as single panel
- Data throughput: `64 × panel_count` pixels total
- Refresh rate: Similar to single panel
- Multiple latches per row cycle

### Memory Usage:
```
Buffer Size = rows × color_depth × (pixels_per_row + 1)
- rows = matrix_height / 2 (16 for 32px height)
- color_depth = 5 (for 5-bit color)
- pixels_per_row = matrix_width × panel_count
- +1 = delay bit per color plane

Example (3-panel chain):
= 16 × 5 × (64 × 3 + 1)
= 16 × 5 × 193
= 15,440 samples (30,880 bytes for uint16_t)
```

---

## Migration Guide

### From Single to PARALLEL_OE:
1. Add second OE pin to configuration
2. Set `expansion_mode = PARALLEL_OE`
3. Set `panel_count = 2`
4. Increase framebuffer width to 128 pixels
5. Wire Panel 1 with shared data lines, separate OE

### From Single to SERIES_CHAIN:
1. Set `expansion_mode = SERIES_CHAIN`
2. Set `panel_count` to desired number
3. Increase framebuffer width to `64 × panel_count`
4. Daisy-chain panels (Data OUT → Data IN)
5. Share OE, LAT, CLK across all panels

---

## Future Enhancements

Potential improvements for expansion modes:

1. **Dynamic OE Support:** Runtime switching between PARALLEL and SERIES
2. **Mixed Mode:** Some panels parallel, some series
3. **Per-Panel Gamma:** Independent gamma tables for PARALLEL_OE
4. **Automatic Detection:** Detect panel count from hardware
5. **Signal Buffering:** Auto-insert buffers for long chains

---

## Troubleshooting

### PARALLEL_OE Issues:
- **Both panels show same image:** Check OE2 wiring
- **Panel 1 dim:** Verify OE2 pin configured correctly
- **Flickering:** Increase BCM length for bit plane

### SERIES_CHAIN Issues:
- **Last panel dark:** Check data daisy-chaining
- **Color corruption:** Reduce clock speed for signal integrity
- **Ghosting:** Add delay between bit planes
- **Uneven brightness:** All panels share OE - check wiring

### General Debug Steps:
1. Verify `effective_width = matrix_width × panel_count`
2. Check buffer size calculation
3. Confirm framebuffer allocation size
4. Test with simple patterns (solid colors)
5. Use oscilloscope to verify clock and data signals

---

## Hardware Reference

### Standard HUB75 Pinout:
```
R0, G0, B0, R1, G1, B1 - RGB data (2 rows)
A, B, C, D, E - Address lines (row selection)
CLK - Clock signal
LAT - Latch signal
OE - Output Enable (active LOW)
GND - Ground
```

### PARALLEL_OE Additional:
```
OE2 - Output Enable for Panel 1 (active LOW)
```

### SERIES_CHAIN Connections:
```
ESP32 → Panel 0 (Data IN)
Panel 0 (Data OUT) → Panel 1 (Data IN)
Panel 1 (Data OUT) → Panel 2 (Data IN)
...
All panels share: CLK, LAT, OE, A-E, GND
```
