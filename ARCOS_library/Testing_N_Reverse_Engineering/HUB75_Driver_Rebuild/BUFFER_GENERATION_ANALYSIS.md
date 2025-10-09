# Buffer Generation Analysis

## Issues Found in Testing_Example (Working but Wrong)

### 1. Buffer Size Calculation
```cpp
// Testing_Example (WRONG):
base_buffer_size = config.matrix_width * (config.matrix_height / 2);
buffer_size = base_buffer_size * config.colour_depth;
// Result: 64 * 16 * 5 = 5,120 samples (2.5KB per buffer)
```

**Problem**: This calculates only ONE sample per pixel per color plane. No BCM timing!

### 2. OE (Output Enable) Control
```cpp
// Testing_Example (WRONG):
for(int col = 0; col < config.matrix_width; col++){
  uint16_t sample = 0;
  // ... set RGB data ...
  
  if(col == config.matrix_width - 1){  // ONLY at last column!
    sample |= (1 << LAT_BIT);
    sample |= (1 << OE_BIT);  // OE HIGH at last pixel only
  }
  
  backBuffer[buffer_index++] = sample;
}
```

**Problem**: 
- OE is LOW (LEDs ON) for 63 pixels
- OE is HIGH (LEDs OFF) for only 1 pixel
- This means LEDs are always on at full brightness!
- No proper blanking during latch/address transitions

### 3. No BCM (Binary Code Modulation) Timing
```cpp
// Testing_Example generates ONE sample per pixel:
// Bit 0: 64 samples
// Bit 1: 64 samples
// Bit 2: 64 samples
// Bit 3: 64 samples
// Bit 4: 64 samples
// Total: 320 samples per row, 5,120 for whole display
```

**Problem**: Each bit plane should have duration proportional to its weight!

**Correct BCM timing**:
```
Bit 0 (LSB): 1x duration (64 samples)
Bit 1:       2x duration (128 samples)
Bit 2:       4x duration (256 samples)
Bit 3:       8x duration (512 samples)
Bit 4 (MSB): 16x duration (1024 samples)
Total: 1,984 samples per row
```

## What Our Implementation Should Do

### Correct Buffer Size Calculation
```cpp
// For 64x32 display with 5-bit color depth:
int samples_per_row = 0;
for(int bit = 0; bit < color_depth; bit++){
  int weight = (1 << bit);  // 1, 2, 4, 8, 16
  samples_per_row += pixels_per_row * weight;  // 64, 128, 256, 512, 1024
}
// samples_per_row = 1,984 samples

int total_samples = samples_per_row * rows_per_frame;  // 16 rows
// total_samples = 31,744 samples (63KB per buffer)
```

### Correct OE Control
```cpp
// OE should follow this pattern for each row:
// 1. Set OE HIGH during address transition (blank display)
// 2. Set LAT HIGH to latch data
// 3. Set OE LOW to enable display
// 4. Clock out pixel data with OE LOW (LEDs visible)
// 5. Repeat based on bit weight

for(int bit = 0; bit < color_depth; bit++){
  int repetitions = (1 << bit);  // 1, 2, 4, 8, 16
  
  for(int rep = 0; rep < repetitions; rep++){
    for(int col = 0; col < pixels_per_row; col++){
      uint16_t sample = address_bits | rgb_bits[col];
      
      if(col == 0 && rep == 0){
        // First sample: blank during address setup
        sample |= OE_BIT;  // OE HIGH (LEDs OFF)
        sample |= LAT_BIT; // LAT HIGH
      } else {
        // Normal samples: display visible
        sample &= ~OE_BIT; // OE LOW (LEDs ON)
      }
      
      buffer[index++] = sample;
    }
  }
}
```

### Correct Brightness Control via OE
Instead of Testing_Example's wrong approach, use OE pulse width:

```cpp
// For each bit plane, control how many samples have OE enabled:
int visible_samples = (brightness * samples_this_plane) / 256;

for(int i = 0; i < samples_this_plane; i++){
  if(i < visible_samples){
    sample &= ~OE_BIT;  // OE LOW (visible)
  } else {
    sample |= OE_BIT;   // OE HIGH (blanked for dimming)
  }
}
```

## Why Testing_Example "Works"

Despite these issues, Testing_Example displays something because:
1. **Continuous DMA** keeps refreshing the display rapidly
2. **All RGB data is present** in the buffer
3. **Address lines are correct** so rows are scanned properly
4. **OE is mostly LOW** so LEDs are mostly visible
5. **The LCD parallel interface** handles the actual clocking

But it has these problems:
- **No grayscale**: All pixels are full brightness or off
- **Poor color depth**: Effectively 1-bit per channel instead of 5-bit
- **Huge memory waste**: Could achieve proper 5-bit color in same buffer size
- **Wrong buffer size**: Should be ~32KB not 2.5KB for proper BCM

## What We Need to Fix in Our Implementation

1. **Implement proper BCM timing** with weighted bit planes
2. **Calculate correct buffer size** based on BCM repetitions
3. **Fix OE control** - should be LOW most of the time, HIGH during transitions
4. **Add blanking samples** at the start of each row for address/latch setup
5. **Test with much smaller buffer** initially (maybe 8-bit color) to verify logic

## Memory Comparison

| Configuration | Testing_Example | Correct BCM | Ours (8-bit) |
|--------------|----------------|-------------|--------------|
| Color Depth | 5-bit | 5-bit | 8-bit |
| Samples/Row | 320 | 1,984 | ~32,704 |
| Total Samples | 5,120 | 31,744 | ~523,264 |
| Memory/Buffer | 10KB | 63KB | ~1MB |
| Grayscale Levels | 2 (on/off) | 32 levels | 256 levels |

**Our 8-bit implementation is using 200KB+ because we're calculating based on wrong assumptions!**
