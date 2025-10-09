# Quick Fix Strategy

## Problem Summary
Our implementation is trying to be too clever with BCM timing, but we haven't actually implemented the timing repetitions. We're calculating buffer sizes correctly for "1 sample per pixel per bit" but that's not enough for true BCM.

## Testing_Example's Approach (Simple, Works)
```
Buffer = pixels_per_row × rows × color_depth
       = 64 × 16 × 5 = 5,120 samples (10KB)
```

Each sample contains:
- RGB bits for upper/lower half
- Address bits (A,B,C,D,E)
- Control bits (LAT, OE)

## Our Approach (Currently)
```
Buffer = pixels_per_row × rows × color_depth  
       = 64 × 16 × 8 = 8,192 samples (16KB)
```

We're doing the same thing but with 8-bit color instead of 5-bit.

## The Real Problem: OE Control

Testing_Example sets OE=1 only at last pixel:
```cpp
for(col = 0; col < 64; col++){
  if(col == 63){
    sample |= OE_BIT | LAT_BIT;  // Latch and blank at end
  }
  buffer[i++] = sample;
}
```

This means:
- OE=0 (LEDs ON) for 63 samples
- OE=1 (LEDs OFF) for 1 sample
- Result: LEDs always on, no brightness control, no grayscale

## Quick Fix for Our Code

1. **Match Testing_Example's OE pattern initially**:
   - Keep OE=1 (OFF) most of the time
   - Only set OE=0 (ON) for visible pixels

2. **Fix clearFrameBuffer** - DONE (added OE bit)

3. **Fix brightness control**:
   ```cpp
   // Current (backwards):
   if(x > brightness_pixels){
     p[x] |= BitMasks::OE;   // OFF after brightness threshold
   } else {
     p[x] &= ~BitMasks::OE;  // ON up to brightness threshold  
   }
   
   // This is actually CORRECT for HUB75!
   // OE=0 means LEDs are ON (enabled)
   // OE=1 means LEDs are OFF (disabled)
   ```

4. **The actual issue**: We need to INVERT our thinking:
   - **OE LOW (0) = Output ENABLED = LEDs visible**
   - **OE HIGH (1) = Output DISABLED = LEDs blank**

## Test Strategy

1. Start with all pixels having OE=1 (blank)
2. Set specific pixels to have OE=0 (visible)  
3. Put actual RGB data in those pixels
4. Should see colored pixels on black background

## Modified clearFrameBuffer

```cpp
void clearFrameBuffer(int buffer_id){
  for(uint8_t row = 0; row < rows_per_frame_; row++){
    uint16_t row_address = calculateAddress(row);
    
    // Base value: address bits + LAT + OE (all LEDs OFF)
    uint16_t base = row_address | LAT_BIT | OE_BIT;
    
    for(uint8_t depth = 0; depth < color_depth; depth++){
      DMAStorageType* p = frame->row_bits[row]->getDataPtr(depth);
      for(uint16_t x = 0; x < pixels_per_row_; x++){
        p[x] = base;  // Everything blanked initially
      }
    }
  }
}
```

## Modified setPixel to Enable Visibility

```cpp
void updateDMABuffer(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b){
  // ... existing bit extraction code ...
  
  for(uint8_t depth = 0; depth < color_depth; depth++){
    uint16_t rgb_bits = extractBits(r, g, b, depth);
    
    p[x] &= CLEAR_RGB;  // Clear RGB bits
    p[x] |= rgb_bits;   // Set new RGB
    p[x] &= ~OE_BIT;    // ENABLE output for this pixel (OE=0)
  }
}
```

This way:
- Cleared pixels have OE=1 (blank/black)
- Set pixels have OE=0 (visible with their RGB values)
