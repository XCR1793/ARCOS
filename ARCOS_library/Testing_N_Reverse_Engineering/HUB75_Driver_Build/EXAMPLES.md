# Examples

## Table of Contents
1. [Basic Setup](#basic-setup)
2. [Drawing Primitives](#drawing-primitives)
3. [Animations](#animations)
4. [Color Gradients](#color-gradients)
5. [Text Display](#text-display)
6. [Advanced Patterns](#advanced-patterns)

---

## Basic Setup

### Minimal Example

```cpp
#include "hub75_driver.hpp"

HUB75Driver display;

extern "C" void app_main(){
  // Get default configuration
  HUB75Config config = HUB75Config::getDefault();
  
  // Initialize display
  display.init(config);
  display.start();
  
  // Draw a red pixel
  display.setPixel(10, 10, RGB(255, 0, 0));
  display.show();
}
```

### Dual Panel Setup

```cpp
#include "hub75_driver.hpp"

HUB75Driver display;

extern "C" void app_main(){
  HUB75Config config = HUB75Config::getDefault();
  
  // Enable dual display mode
  config.dual_display_mode = true;
  config.effective_width = 128;  // 2x 64-wide panels
  
  // Configure dual OE pins
  config.pins.oe_pin = 35;   // Panel 0
  config.pins.oe_pin2 = 6;   // Panel 1
  
  // Enable gamma correction
  config.enable_gamma_correction = true;
  config.gamma_value = 2.2f;
  
  // Panel orientation (if one panel is upside down)
  config.panel_inversions[0].flip_vertical = true;
  
  display.init(config);
  display.start();
  
  // Draw across both panels
  for(int x = 0; x < 128; x++){
    display.setPixel(x, 16, RGB(255, 0, 0));
  }
  display.show();
}
```

---

## Drawing Primitives

### Lines

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

// Example usage
drawHLine(0, 15, 64, RGB(255, 0, 0));    // Red horizontal line
drawVLine(32, 0, 32, RGB(0, 255, 0));    // Green vertical line
display.show();
```

### Rectangles

```cpp
// Outline rectangle
void drawRect(int x, int y, int w, int h, RGB color){
  drawHLine(x, y, w, color);              // Top
  drawHLine(x, y + h - 1, w, color);      // Bottom
  drawVLine(x, y, h, color);              // Left
  drawVLine(x + w - 1, y, h, color);      // Right
}

// Filled rectangle
void fillRect(int x, int y, int w, int h, RGB color){
  for(int dy = 0; dy < h; dy++){
    drawHLine(x, y + dy, w, color);
  }
}

// Example usage
drawRect(5, 5, 20, 15, RGB(255, 255, 0));      // Yellow outline
fillRect(30, 10, 15, 10, RGB(0, 255, 255));    // Cyan filled
display.show();
```

### Circles

```cpp
// Draw circle (midpoint algorithm)
void drawCircle(int x0, int y0, int radius, RGB color){
  int x = radius;
  int y = 0;
  int err = 0;
  
  while(x >= y){
    display.setPixel(x0 + x, y0 + y, color);
    display.setPixel(x0 + y, y0 + x, color);
    display.setPixel(x0 - y, y0 + x, color);
    display.setPixel(x0 - x, y0 + y, color);
    display.setPixel(x0 - x, y0 - y, color);
    display.setPixel(x0 - y, y0 - x, color);
    display.setPixel(x0 + y, y0 - x, color);
    display.setPixel(x0 + x, y0 - y, color);
    
    if(err <= 0){
      y += 1;
      err += 2*y + 1;
    }
    if(err > 0){
      x -= 1;
      err -= 2*x + 1;
    }
  }
}

// Example usage
drawCircle(32, 16, 10, RGB(255, 0, 255));  // Magenta circle
display.show();
```

---

## Animations

### Rainbow Cycle

```cpp
RGB hueToRgb(float hue){
  float r, g, b;
  hue = fmodf(hue, 360.0f) / 60.0f;
  
  if(hue < 1.0f){
    r = 1.0f; g = hue; b = 0.0f;
  }else if(hue < 2.0f){
    r = 2.0f - hue; g = 1.0f; b = 0.0f;
  }else if(hue < 3.0f){
    r = 0.0f; g = 1.0f; b = hue - 2.0f;
  }else if(hue < 4.0f){
    r = 0.0f; g = 4.0f - hue; b = 1.0f;
  }else if(hue < 5.0f){
    r = hue - 4.0f; g = 0.0f; b = 1.0f;
  }else{
    r = 1.0f; g = 0.0f; b = 6.0f - hue;
  }
  
  return RGB((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

void rainbowCycle(){
  float hue_offset = 0.0f;
  
  while(true){
    for(int x = 0; x < display.getWidth(); x++){
      for(int y = 0; y < display.getHeight(); y++){
        float hue = (x * 360.0f / display.getWidth()) + hue_offset;
        RGB color = hueToRgb(hue);
        display.setPixel(x, y, color);
      }
    }
    
    display.show();
    
    hue_offset += 5.0f;
    if(hue_offset >= 360.0f) hue_offset -= 360.0f;
    
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
```

### Bouncing Ball

```cpp
void bouncingBall(){
  float x = 32.0f, y = 16.0f;
  float vx = 2.0f, vy = 1.5f;
  int radius = 3;
  
  while(true){
    display.fillScreen(RGB(0, 0, 0));
    
    // Update position
    x += vx;
    y += vy;
    
    // Bounce off walls
    if(x - radius < 0 || x + radius >= display.getWidth()){
      vx = -vx;
    }
    if(y - radius < 0 || y + radius >= display.getHeight()){
      vy = -vy;
    }
    
    // Draw ball
    drawCircle((int)x, (int)y, radius, RGB(255, 0, 0));
    display.show();
    
    vTaskDelay(pdMS_TO_TICKS(30));
  }
}
```

### Brightness Fade

```cpp
void breathingEffect(){
  while(true){
    // Fade up
    for(int brightness = 0; brightness <= 255; brightness += 5){
      display.setBrightness(brightness);
      display.show();
      vTaskDelay(pdMS_TO_TICKS(20));
    }
    
    // Fade down
    for(int brightness = 255; brightness >= 0; brightness -= 5){
      display.setBrightness(brightness);
      display.show();
      vTaskDelay(pdMS_TO_TICKS(20));
    }
  }
}
```

---

## Color Gradients

### Horizontal Gradient

```cpp
void horizontalGradient(RGB color1, RGB color2){
  int width = display.getWidth();
  int height = display.getHeight();
  
  for(int x = 0; x < width; x++){
    float t = (float)x / (width - 1);
    
    uint8_t r = color1.r + (color2.r - color1.r) * t;
    uint8_t g = color1.g + (color2.g - color1.g) * t;
    uint8_t b = color1.b + (color2.b - color1.b) * t;
    
    RGB color(r, g, b);
    
    for(int y = 0; y < height; y++){
      display.setPixel(x, y, color);
    }
  }
  
  display.show();
}

// Example: Blue to red gradient
horizontalGradient(RGB(0, 0, 255), RGB(255, 0, 0));
```

### Radial Gradient

```cpp
void radialGradient(int cx, int cy, RGB center_color, RGB edge_color){
  int width = display.getWidth();
  int height = display.getHeight();
  float max_dist = sqrtf(width*width + height*height) / 2.0f;
  
  for(int y = 0; y < height; y++){
    for(int x = 0; x < width; x++){
      float dx = x - cx;
      float dy = y - cy;
      float dist = sqrtf(dx*dx + dy*dy);
      float t = fminf(dist / max_dist, 1.0f);
      
      uint8_t r = center_color.r + (edge_color.r - center_color.r) * t;
      uint8_t g = center_color.g + (edge_color.g - center_color.g) * t;
      uint8_t b = center_color.b + (edge_color.b - center_color.b) * t;
      
      display.setPixel(x, y, RGB(r, g, b));
    }
  }
  
  display.show();
}

// Example: White center to black edges
radialGradient(32, 16, RGB(255, 255, 255), RGB(0, 0, 0));
```

---

## Text Display

### Simple 5x7 Font

```cpp
// Simple 5x7 bitmap font (example: letter 'A')
const uint8_t font_A[7] = {
  0b01110,  //  ***
  0b10001,  // *   *
  0b10001,  // *   *
  0b11111,  // *****
  0b10001,  // *   *
  0b10001,  // *   *
  0b10001   // *   *
};

void drawChar(int x, int y, const uint8_t* char_data, RGB color){
  for(int row = 0; row < 7; row++){
    for(int col = 0; col < 5; col++){
      if(char_data[row] & (1 << (4 - col))){
        display.setPixel(x + col, y + row, color);
      }
    }
  }
}

// Example usage
drawChar(10, 10, font_A, RGB(255, 255, 0));
display.show();
```

### Scrolling Text

```cpp
void scrollText(const char* text, RGB color, int speed_ms){
  int x_offset = display.getWidth();
  int text_width = strlen(text) * 6;  // 5 pixels + 1 space
  
  while(true){
    display.fillScreen(RGB(0, 0, 0));
    
    // Draw text at current offset
    for(int i = 0; text[i]; i++){
      drawChar(x_offset + i * 6, 12, getCharBitmap(text[i]), color);
    }
    
    display.show();
    
    // Move left
    x_offset--;
    if(x_offset < -text_width){
      x_offset = display.getWidth();
    }
    
    vTaskDelay(pdMS_TO_TICKS(speed_ms));
  }
}
```

---

## Advanced Patterns

### Matrix Rain Effect

```cpp
void matrixRain(){
  struct Drop {
    int x;
    int y;
    int length;
    int speed;
  };
  
  const int MAX_DROPS = 20;
  Drop drops[MAX_DROPS];
  
  // Initialize drops
  for(int i = 0; i < MAX_DROPS; i++){
    drops[i].x = rand() % display.getWidth();
    drops[i].y = -(rand() % 20);
    drops[i].length = 5 + (rand() % 10);
    drops[i].speed = 1 + (rand() % 3);
  }
  
  while(true){
    // Fade existing pixels
    for(int y = 0; y < display.getHeight(); y++){
      for(int x = 0; x < display.getWidth(); x++){
        // Fade to black (requires reading pixels - use shadow buffer)
      }
    }
    
    // Update and draw drops
    for(int i = 0; i < MAX_DROPS; i++){
      drops[i].y += drops[i].speed;
      
      // Draw drop
      for(int j = 0; j < drops[i].length; j++){
        int y = drops[i].y - j;
        if(y >= 0 && y < display.getHeight()){
          uint8_t brightness = 255 - (j * 255 / drops[i].length);
          display.setPixel(drops[i].x, y, RGB(0, brightness, 0));
        }
      }
      
      // Reset drop at bottom
      if(drops[i].y - drops[i].length > display.getHeight()){
        drops[i].y = 0;
        drops[i].x = rand() % display.getWidth();
      }
    }
    
    display.show();
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
```

### Plasma Effect

```cpp
void plasmaEffect(){
  float time = 0.0f;
  
  while(true){
    for(int y = 0; y < display.getHeight(); y++){
      for(int x = 0; x < display.getWidth(); x++){
        float value = sinf(x * 0.1f + time) + 
                     sinf(y * 0.1f + time) + 
                     sinf((x + y) * 0.1f + time);
        
        value = (value + 3.0f) / 6.0f;  // Normalize to 0-1
        
        // Map to rainbow colors
        float hue = value * 360.0f;
        RGB color = hueToRgb(hue);
        
        display.setPixel(x, y, color);
      }
    }
    
    display.show();
    time += 0.1f;
    vTaskDelay(pdMS_TO_TICKS(30));
  }
}
```

### Fire Effect

```cpp
void fireEffect(){
  const int width = display.getWidth();
  const int height = display.getHeight();
  uint8_t heat[width][height];
  
  while(true){
    // Cool down
    for(int y = 0; y < height; y++){
      for(int x = 0; x < width; x++){
        int cooling = rand() % 50;
        heat[x][y] = heat[x][y] > cooling ? heat[x][y] - cooling : 0;
      }
    }
    
    // Heat from bottom
    for(int x = 0; x < width; x++){
      if(rand() % 3 == 0){
        heat[x][height-1] = 255;
      }
    }
    
    // Drift upwards
    for(int y = 0; y < height - 1; y++){
      for(int x = 0; x < width; x++){
        heat[x][y] = (heat[x][y+1] + heat[x][y] * 2) / 3;
      }
    }
    
    // Map heat to color
    for(int y = 0; y < height; y++){
      for(int x = 0; x < width; x++){
        uint8_t h = heat[x][y];
        RGB color;
        if(h < 85){
          color = RGB(h * 3, 0, 0);                      // Black to red
        }else if(h < 170){
          color = RGB(255, (h - 85) * 3, 0);            // Red to yellow
        }else{
          color = RGB(255, 255, (h - 170) * 3);         // Yellow to white
        }
        display.setPixel(x, y, color);
      }
    }
    
    display.show();
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
```

---

## Performance Tips

1. **Minimize show() calls:**
   ```cpp
   // Bad: show() in loop
   for(int i = 0; i < 100; i++){
     display.setPixel(i, 10, RGB(255, 0, 0));
     display.show();  // Too many!
   }
   
   // Good: show() once
   for(int i = 0; i < 100; i++){
     display.setPixel(i, 10, RGB(255, 0, 0));
   }
   display.show();
   ```

2. **Pre-calculate colors:**
   ```cpp
   // Bad: calculate in loop
   for(int x = 0; x < width; x++){
     RGB color = hueToRgb(x * 360.0f / width);
     drawVLine(x, 0, height, color);
   }
   
   // Good: pre-calculate
   RGB colors[width];
   for(int x = 0; x < width; x++){
     colors[x] = hueToRgb(x * 360.0f / width);
   }
   for(int x = 0; x < width; x++){
     drawVLine(x, 0, height, colors[x]);
   }
   ```

3. **Use fillScreen() when possible:**
   ```cpp
   // Faster than setting each pixel
   display.fillScreen(RGB(0, 0, 0));
   ```
