# Dual Display Fix - HUB75 Driver

## Problem Identified

Both panels were displaying red instead of red on panel 0 and blue on panel 1.

## Root Cause

The original code only clocked **64 pixels** per row, even in dual display mode. This meant:
- Both panels received the SAME data (columns 0-63 from framebuffer)
- Panel 1 never received its intended data (columns 64-127 from framebuffer)
- Both panels showed identical content

## Solution

### 1. Extended Column Loop for Dual Display

**Before:**
```cpp
for(int col = 0; col < config.matrix_width; col++){  // Only 64 pixels
```

**After:**
```cpp
int total_columns = config.dual_display_mode ? 128 : config.matrix_width;
for(int col = 0; col < total_columns; col++){  // 128 pixels for dual mode
```

### 2. Updated Framebuffer Mapping

The framebuffer indexing now correctly reads:
- **Columns 0-63**: Data for Panel 0 (framebuffer columns 0-63)
- **Columns 64-127**: Data for Panel 1 (framebuffer columns 64-127)

```cpp
int upper_index = upper_row * fb_width + col;  // fb_width = 128 in dual mode
int lower_index = lower_row * fb_width + col;
```

### 3. Fixed OE Pin Control

**Panel Selection Logic:**

```cpp
if(col < 64){
  // Panel 0 section (columns 0-63)
  - OE (pin 35): BCM controlled for Panel 0
  - OE2 (pin 6): Always HIGH (disabled)
} else {
  // Panel 1 section (columns 64-127)
  - OE (pin 35): Always HIGH (disabled)
  - OE2 (pin 6): BCM controlled for Panel 1
}
```

**BCM Pattern per Panel:**
```cpp
int local_col = col % 64;  // 0-63 within each panel section

if(local_col >= bcm_length){
  // Disable active panel's OE after BCM period
}
```

### 4. Dual Latch Points

Latch is asserted at TWO points:
- **Column 63**: Latches Panel 0 data into shift registers
- **Column 127**: Latches Panel 1 data into shift registers

```cpp
if(col == 63 || col == 127){
  sample |= (1 << LAT_BIT);
}
```

### 5. Updated Buffer Size

Buffer size now accounts for 128 pixels in dual display mode:

```cpp
int pixels_per_row = config.dual_display_mode ? 128 : 64;
buffer_size = rows × color_depth × (pixels_per_row + 1);

// Dual mode: 16 rows × 5 planes × 129 samples = 10,320 samples
// Single mode: 16 rows × 5 planes × 65 samples = 5,200 samples
```

## Data Flow Visualization

### Dual Display Mode - Per Row, Per Color Plane:

```
Clock Cycle:  0    1    2   ...  62   63   64   65  ...  126  127  128
             ┌────┬────┬────────┬────┬────┬────┬────────┬────┬────┬────┐
Data:        │P0  │P0  │...     │P0  │P0  │P1  │P1  ... │P1  │P1  │DLY │
             │Col0│Col1│        │Col62│Col63│Col0│Col1   │Col62│Col63│    │
             └────┴────┴────────┴────┴────┴────┴────────┴────┴────┴────┘
                        Panel 0 (64px)      Panel 1 (64px)        Delay

OE (Pin 35): [BCM Pattern Panel 0........] [HIGH-Disabled........]
OE2 (Pin 6): [HIGH-Disabled...............]  [BCM Pattern Panel 1.]

Latch:       ..............................↑.......................↑
                                        Col 63               Col 127
```

### BCM Pattern Example (Bit Plane 2, bcm_length = 4):

```
Panel 0 Section (columns 0-63):
Col:  0  1  2  3  4  5  6 ... 63
OE:   0  0  0  0  1  1  1 ... 1   (0=ON, 1=OFF)
      └──────┘  └──────────────┘
      4 cycles   60 cycles OFF
      ON (BCM)

Panel 1 Section (columns 64-127):
Col:  64 65 66 67 68 69 70 ... 127
OE2:  0  0  0  0  1  1  1  ... 1
      └──────┘  └───────────────┘
      4 cycles   60 cycles OFF
      ON (BCM)
```

## Key Changes Summary

1. ✅ **Loop Extension**: 64 → 128 pixels per row in dual mode
2. ✅ **Framebuffer Mapping**: Correctly reads columns 0-127
3. ✅ **OE Control**: Independent BCM per panel
4. ✅ **Latch Points**: Two latches (col 63 and 127)
5. ✅ **Buffer Size**: Doubled for dual mode (10,320 samples)

## Expected Result

- **Panel 0 (left)**: Solid RED (columns 0-63 of framebuffer)
- **Panel 1 (right)**: Solid BLUE (columns 64-127 of framebuffer)
- Both panels controlled independently via separate OE pins
- Each panel gets proper BCM brightness control

## Memory Impact

**Dual Display Mode:**
- Front buffer: 10,320 samples × 2 bytes = 20,640 bytes
- Back buffer: 10,320 samples × 2 bytes = 20,640 bytes
- **Total DMA buffers**: ~41 KB (vs ~20 KB in single mode)

**Framebuffer:**
- 128 × 32 × 3 bytes (RGB) = 12,288 bytes (unchanged)

## Testing

The test pattern (`drawTestRectangles()`) should now correctly display:
```
┌─────────────────┬─────────────────┐
│                 │                 │
│   RED PANEL     │   BLUE PANEL    │
│   (Panel 0)     │   (Panel 1)     │
│   64×32 px      │   64×32 px      │
│                 │                 │
└─────────────────┴─────────────────┘
 Columns 0-63      Columns 64-127
```

## Technical Notes

- Both panels share RGB data pins (R0, G0, B0, R1, G1, B1)
- Panels differentiated by independent OE control (OE vs OE2)
- Data clocked sequentially: Panel 0 data, then Panel 1 data
- Each panel latches its own data at appropriate clock cycle
- BCM pattern applied independently to each panel's OE pin
