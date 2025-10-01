# HUB75 BCM Protocol Update

## Overview

The HUB75 driver protocol has been updated to implement a Binary Code Modulation (BCM) pattern for proper brightness control via the Output Enable (OE) pins.

## New Protocol Structure

### Previous Protocol:
- Organized by color plane → row → columns
- OE control was simple on/off at end of row
- No brightness scaling per bit plane

### New Protocol:
```
For each row (16 rows for 32-pixel height):
  For each color buffer/bit plane (5 planes for 5-bit color):
    - Generate 64 pixel samples (one full row width)
    - Apply BCM pattern to OE control
    - Add 1-bit delay with OE disabled
```

## Buffer Organization

### Structure:
```
Row 0:
  Bit Plane 0: [64 pixel samples] + [1 delay bit]
  Bit Plane 1: [64 pixel samples] + [1 delay bit]
  Bit Plane 2: [64 pixel samples] + [1 delay bit]
  Bit Plane 3: [64 pixel samples] + [1 delay bit]
  Bit Plane 4: [64 pixel samples] + [1 delay bit]
Row 1:
  Bit Plane 0: [64 pixel samples] + [1 delay bit]
  ...
Row 15:
  ...
```

### Buffer Size Calculation:
```cpp
rows = matrix_height / 2 = 16
buffer_size = rows × color_depth × (matrix_width + 1)
            = 16 × 5 × (64 + 1)
            = 16 × 5 × 65
            = 5,200 samples
```

## BCM Pattern Implementation

### Brightness Control via OE:

The BCM pattern controls how long the OE (Output Enable) pin stays active for each bit plane:

```cpp
int bcm_length = 1 << plane;  // 2^plane cycles

Bit Plane 0: OE enabled for 1 cycle   (2^0 = 1)
Bit Plane 1: OE enabled for 2 cycles  (2^1 = 2)
Bit Plane 2: OE enabled for 4 cycles  (2^2 = 4)
Bit Plane 3: OE enabled for 8 cycles  (2^3 = 8)
Bit Plane 4: OE enabled for 16 cycles (2^4 = 16)
```

### OE Control Logic:

```cpp
// OE is ACTIVE LOW: 0 = ON, 1 = OFF
if(col >= bcm_length){
  sample |= (1 << OE_BIT);      // Disable OE (high)
  sample |= (1 << OE2_BIT);     // Disable second OE (dual panel)
}
// else: OE stays low (enabled) during BCM period
```

### Visual Representation:

For a 64-pixel row with 5-bit color:

```
Bit Plane 0 (LSB):
[ON][OFF][OFF][OFF][OFF]...[OFF] + [DELAY]
 ^    ^
 |    +-- OE disabled for remaining 63 pixels
 +------- OE enabled for 1 pixel (bcm_length = 1)

Bit Plane 1:
[ON][ON][OFF][OFF][OFF]...[OFF] + [DELAY]
 ^    ^    ^
 +----+----+-- OE enabled for 2 pixels (bcm_length = 2)

Bit Plane 2:
[ON][ON][ON][ON][OFF]...[OFF] + [DELAY]
 ^-----------^
 OE enabled for 4 pixels (bcm_length = 4)

Bit Plane 3:
[ON][ON]...[ON (8x)][OFF]...[OFF (56x)] + [DELAY]

Bit Plane 4 (MSB):
[ON][ON]...[ON (16x)][OFF]...[OFF (48x)] + [DELAY]
```

## Ghosting Prevention

### 1-Bit Delay Between Color Buffers:

After each color buffer (bit plane), a delay sample is inserted with:
- **Address lines**: Maintained (same row)
- **OE pins**: Disabled (HIGH)
- **Latch**: Cleared
- **Data bits**: All zero

This prevents ghosting artifacts when transitioning between bit planes.

```cpp
// Delay sample structure
uint16_t delay_sample = 0;
delay_sample |= address_bits;    // Keep row address
delay_sample |= (1 << OE_BIT);   // OE disabled
delay_sample |= (1 << OE2_BIT);  // OE2 disabled
// No latch, no data
```

## Brightness Scaling

The BCM pattern creates proper brightness scaling:

```
Value    Bit Pattern    Brightness
  0      00000         Dark (0/31)
  1      00001         1/31 brightness
  2      00010         2/31 brightness
  3      00011         3/31 brightness
  ...
  15     01111         15/31 brightness
  31     11111         Full brightness (31/31)
```

Each bit contributes proportionally to total brightness:
- Bit 0 (LSB): +1 unit
- Bit 1: +2 units  
- Bit 2: +4 units
- Bit 3: +8 units
- Bit 4 (MSB): +16 units

Total: 1+2+4+8+16 = 31 units = full brightness

## Dual Panel Support

Both OE pins (OE_BIT and OE2_BIT) follow the same BCM pattern:

```cpp
// Panel 0: columns 0-63 → OE pin 35
// Panel 1: columns 64-127 → OE pin 6 (OE2_BIT)

if(col >= bcm_length){
  sample |= (1 << OE_BIT);   // Disable both panels
  if(config.pins.oe_pin2 >= 0){
    sample |= (1 << OE2_BIT);
  }
}
```

Both panels receive the same BCM timing but display different data (left 64 pixels vs right 64 pixels of framebuffer).

## Test Pattern

The test pattern draws:
- **Panel 0 (columns 0-63)**: Red rectangle (RGB: 255, 0, 0)
- **Panel 1 (columns 64-127)**: Blue rectangle (RGB: 0, 0, 255)

This validates:
1. Dual panel addressing works correctly
2. BCM brightness scaling is correct (full brightness)
3. Color data routing is correct
4. OE control is synchronized

## Benefits of New Protocol

1. **Proper Brightness Control**: BCM provides linear brightness scaling
2. **Reduced Ghosting**: 1-bit delay prevents artifacts between bit planes
3. **Better Color Accuracy**: Each bit plane gets proportional display time
4. **Hardware Efficiency**: OE pins control brightness without PWM flicker
5. **Dual Panel Support**: Both panels receive synchronized BCM timing

## Technical Notes

- **Clock Speed**: 10 MHz default (adjustable via config)
- **Refresh Rate**: Depends on buffer size and clock speed
- **Color Depth**: 5-bit per channel (15-bit total RGB)
- **Gamma Correction**: Applied in software before BCM encoding
- **Memory Usage**: ~10KB for dual-buffered DMA (5200 samples × 2 bytes × 2 buffers)

## Code Changes Summary

### Modified Functions:
1. `convertFramebufferToHUB75()` - Implements new BCM protocol
2. Buffer size calculation - Accounts for delay bits
3. `drawTestRectangles()` - New test pattern function
4. `main()` - Calls test pattern instead of plasma animation

### Key Constants:
- `config.matrix_width = 64` - Pixels per row
- `config.matrix_height = 32` - Total panel height
- `hub75_rows = 16` - Addressable rows (height/2)
- `config.colour_depth = 5` - Bit planes for BCM
- Delay bits: 1 per color buffer

## Expected Results

When running the test pattern:
- Left panel (0-63) should display solid red
- Right panel (64-127) should display solid blue
- No visible flickering due to BCM
- Brightness should be uniform across both panels
- No ghosting artifacts between bit planes

## Future Enhancements

1. **Adaptive BCM**: Adjust BCM lengths based on ambient light
2. **Temporal Dithering**: Add sub-bit brightness levels
3. **HDR Support**: Extended BCM patterns for higher dynamic range
4. **Power Optimization**: Disable OE during blanking for power saving
