# BCM Brightness Control

## Overview

This document explains the **Binary Code Modulation (BCM)** protocol and the **fill-up brightness control** strategy used in the HUB75 driver.

## What is BCM?

**Binary Code Modulation** is a technique for creating grayscale/color depth without traditional PWM. Instead of varying the duty cycle of a single pulse, BCM splits each color value into binary bit planes with exponentially weighted display times.

### 5-Bit BCM Breakdown

For 5-bit color depth (32 levels per channel):

```
Color value: 0-31 (5 bits)

Bit 0 (LSB): Weight = 1  → Display for 1  time unit
Bit 1:       Weight = 2  → Display for 2  time units
Bit 2:       Weight = 4  → Display for 4  time units
Bit 3:       Weight = 8  → Display for 8  time units
Bit 4 (MSB): Weight = 16 → Display for 16 time units

Total: 1 + 2 + 4 + 8 + 16 = 31 time units per complete cycle
```

### Example: RGB(18, 25, 7)

**Red = 18 = 0b10010:**
- Bit 4: ON  (16)
- Bit 3: OFF (0)
- Bit 2: OFF (0)
- Bit 1: ON  (2)
- Bit 0: OFF (0)
- Display: 16 + 2 = 18 ✓

**Green = 25 = 0b11001:**
- Bit 4: ON  (16)
- Bit 3: ON  (8)
- Bit 2: OFF (0)
- Bit 1: OFF (0)
- Bit 0: ON  (1)
- Display: 16 + 8 + 1 = 25 ✓

**Blue = 7 = 0b00111:**
- Bit 4: OFF (0)
- Bit 3: OFF (0)
- Bit 2: ON  (4)
- Bit 1: ON  (2)
- Bit 0: ON  (1)
- Display: 4 + 2 + 1 = 7 ✓

## HUB75 BCM Implementation

### Buffer Structure

For each row (16 rows total in 32-pixel-high panel):

```
Row 0:
  Bit Plane 0: [64 pixels × RGB data] → [Latch] → [Display 1  cycle with OE] → [Delay 3 cycles]
  Bit Plane 1: [64 pixels × RGB data] → [Latch] → [Display 2  cycles with OE] → [Delay 3 cycles]
  Bit Plane 2: [64 pixels × RGB data] → [Latch] → [Display 4  cycles with OE] → [Delay 3 cycles]
  Bit Plane 3: [64 pixels × RGB data] → [Latch] → [Display 8  cycles with OE] → [Delay 3 cycles]
  Bit Plane 4: [64 pixels × RGB data] → [Latch] → [Display 16 cycles with OE] → [Delay 3 cycles]

Row 1:
  (repeat bit planes 0-4)
  
...

Row 15:
  (repeat bit planes 0-4)
```

### Timing Phases

**1. Pixel Data Phase (OE disabled):**
- Clock out RGB bits for 64 pixels
- RGB0 (upper half) and RGB1 (lower half) clocked simultaneously
- Row address set via A/B/C/D/E pins
- OE HIGH (display off)

**2. Latch Phase:**
- Pulse LAT HIGH to load shift registers into output latches
- OE still HIGH

**3. BCM Display Phase (OE enabled):**
- OE LOW (display on)
- Hold for exponential duration:
  - Plane 0: 1 cycle
  - Plane 1: 2 cycles
  - Plane 2: 4 cycles
  - Plane 3: 8 cycles
  - Plane 4: 16 cycles

**4. Delay Phase:**
- OE HIGH (display off)
- 3-cycle delay to prevent ghosting
- Allows panel to fully discharge before next bit plane

## Brightness Control Challenge

### The Problem

Traditional approach: Scale BCM timing directly

```cpp
// WRONG - breaks buffer allocation
int bcm_length = (1 << plane) * brightness_scale;
```

**Why this fails:**
- DMA buffer size pre-allocated for fixed timing (31 cycles)
- Changing `bcm_length` causes buffer overflow/underflow
- Buffer calculation:
  ```cpp
  total_bcm_samples = 31 * panel_count;  // Fixed!
  ```

### The Solution: Fill-Up Strategy

**Concept:** Allocate buffer for maximum brightness, fill only needed cycles with OE active.

**Implementation:**

1. **Allocate for maximum:**
   ```cpp
   int base_bcm_cycles = 31;          // 1+2+4+8+16
   int max_brightness_scale = 64;     // 64 levels
   int total_bcm_samples = base_bcm_cycles * max_brightness_scale * panel_count;
   // = 31 × 64 × 2 = 3,968 cycles per row
   ```

2. **Fill based on brightness:**
   ```cpp
   int base_bcm_length = 1 << plane;  // 1, 2, 4, 8, or 16
   
   // Map 0-255 brightness to 0-63 scale
   int brightness_scale = bcm_brightness >> 2;
   
   // Calculate cycles
   int max_bcm_cycles = base_bcm_length * 64;
   int active_bcm_cycles = base_bcm_length * brightness_scale;
   int inactive_bcm_cycles = max_bcm_cycles - active_bcm_cycles;
   ```

3. **Write buffer:**
   ```cpp
   // Fill active cycles (OE enabled - display on)
   uint16_t bcm_sample_on = (address_bits) | (OE_BIT = 0);
   for(int i = 0; i < active_bcm_cycles; i++){
     backBuffer[buffer_index++] = bcm_sample_on;
   }
   
   // Fill inactive cycles (OE disabled - display off)
   uint16_t bcm_sample_off = (address_bits) | (OE_BIT = 1);
   for(int i = 0; i < inactive_bcm_cycles; i++){
     backBuffer[buffer_index++] = bcm_sample_off;
   }
   ```

### Brightness Examples

**Brightness 255 (maximum):**
- Scale: 255 >> 2 = 63
- Plane 0: 1 × 63 = 63 active, 1 inactive
- Plane 4: 16 × 63 = 1008 active, 16 inactive
- Result: Near-maximum brightness

**Brightness 128 (50%):**
- Scale: 128 >> 2 = 32
- Plane 0: 1 × 32 = 32 active, 32 inactive
- Plane 4: 16 × 32 = 512 active, 512 inactive
- Result: 50% brightness

**Brightness 64 (25%):**
- Scale: 64 >> 2 = 16
- Plane 0: 1 × 16 = 16 active, 48 inactive
- Plane 4: 16 × 16 = 256 active, 768 inactive
- Result: 25% brightness

**Brightness 0 (off):**
- Scale: 0 >> 2 = 0
- All planes: 0 active, all inactive
- Result: Display off

## Advantages of Fill-Up Strategy

✅ **Fixed Buffer Size:** No reallocation needed
✅ **Smooth Brightness:** 64 distinct levels (0-63)
✅ **No Quantization:** Full color depth maintained at all brightness levels
✅ **DMA-Safe:** Buffer size always matches DMA descriptor setup
✅ **Flicker-Free:** Consistent timing, smooth transitions

## Comparison to Pixel Multiplication

### Pixel Multiplication (Old Method)

```cpp
// Apply brightness to pixel values
uint8_t r_scaled = (pixel.r * brightness) / 255;
uint8_t g_scaled = (pixel.g * brightness) / 255;
uint8_t b_scaled = (pixel.b * brightness) / 255;
```

**Problems:**
- ❌ Loses bit depth at low brightness
- ❌ Causes color quantization (blocky gradients)
- ❌ Integer rounding errors
- ❌ Example: RGB(1,1,1) at 50% → (0,0,0) - disappears!

### Fill-Up Strategy (Current Method)

```cpp
// Keep full pixel values, control OE timing
display.setPixel(x, y, RGB(255, 128, 64));  // Full values
display.setBrightness(128);                 // Control via BCM
```

**Benefits:**
- ✅ Full 5-bit color depth at all brightness levels
- ✅ Smooth gradients
- ✅ No quantization artifacts
- ✅ True linear brightness response

## Why 64 Levels?

The 64-level resolution comes from the relationship between pixel clocking and BCM timing:

- **64 pixels** clocked per row
- **64 clock cycles** to clock out one row
- Natural "bank" of **64 time units** available
- Brightness can utilize up to **64× base BCM timing**

Could we use 255 levels?
- Yes, but requires 255× buffer size (too large)
- 64 levels provides good perceptual resolution
- Human eye can barely distinguish more than 64 brightness levels

## Performance Impact

**Memory:**
- Old (31× base): ~11KB per buffer
- New (64× base): ~130KB per buffer (12× larger)
- Still fits comfortably in ESP32-S3 SRAM

**Speed:**
- No performance impact
- DMA transfers same amount of data
- Buffer generation slightly longer (more cycles to fill)
- Imperceptible to human eye

**Power:**
- Brightness directly affects power consumption
- Lower brightness = less LED on-time = lower power
- Exponential relationship (50% brightness ≠ 50% power)

## Code Reference

**Buffer Allocation:**
`hub75_driver.cpp` lines ~125-147

**Fill-Up Implementation:**
`hub75_driver.cpp` lines ~570-650

**Brightness API:**
`hub75_driver.cpp` lines ~293-295

## Future Improvements

**Possible Enhancements:**
1. **Hardware PWM on OE:** Use ESP32 LEDC for brightness
2. **Adaptive Scaling:** Dynamic brightness based on content
3. **Per-Panel Brightness:** Independent control via dual OE
4. **8-bit BCM:** Full 256-level depth (requires 8 bit planes)

**Power Optimization:**
```cpp
// Calculate actual on-time for power estimation
float on_time_ratio = (float)brightness_scale / 64.0f;
float power_estimate = base_power * on_time_ratio;
```
