# Coding Style Guide

This document outlines the coding conventions and formatting rules used in the HUB75 Driver project.

---

## Table of Contents
1. [General Principles](#general-principles)
2. [Naming Conventions](#naming-conventions)
3. [Formatting Rules](#formatting-rules)
4. [File Organization](#file-organization)
5. [Comments and Documentation](#comments-and-documentation)
6. [Best Practices](#best-practices)

---

## General Principles

- **Consistency**: Follow existing patterns in the codebase
- **Readability**: Code should be self-documenting where possible
- **Simplicity**: Prefer clear, straightforward solutions
- **Performance**: Optimize critical paths (BCM conversion, DMA operations)

---

## Naming Conventions

### Classes and Structs

```cpp
// PascalCase for class names
class HUB75Driver{
  // ...
};

struct HUB75Config{
  // ...
};

struct PanelInversion{
  // ...
};
```

### Variables

```cpp
// snake_case for variables
int panel_width = 64;
int panel_height = 32;
bool dual_display_mode = false;

// Member variables (no special prefix)
class HUB75Driver{
  int width;
  int height;
  uint8_t bcm_brightness;
  RGB* framebuffer;
};
```

### Functions and Methods

```cpp
// camelCase for functions and methods
void init(HUB75Config config);
void setPixel(int x, int y, RGB color);
RGB getPixel(int x, int y);
void fillScreen(RGB color);

// Boolean functions can use 'is' prefix
bool isInitialized();
bool isRunning();
```

### Constants and Macros

```cpp
// UPPER_CASE for constants and macros
#define MAX_BRIGHTNESS 255
#define BCM_BITS 5

const int DEFAULT_BRIGHTNESS = 128;
const float GAMMA_VALUE = 2.2f;
```

### Enumerations

```cpp
// PascalCase for enum names, UPPER_CASE for values
enum ExpansionMode{
  SINGLE,
  PARALLEL_OE,
  SERIES_CHAIN
};
```

---

## Formatting Rules

### Braces and Spacing

**Tighter style - no space before opening brace:**

```cpp
// Correct - tighter style
void function(){
  if(condition){
    doSomething();
  }else{
    doSomethingElse();
  }
}

// Incorrect - too spaced out
void function() {
  if (condition) {
    doSomething();
  } else {
    doSomethingElse();
  }
}
```

### Control Structures

```cpp
// If-else statements
if(x > 0){
  // do something
}else if(x < 0){
  // do something else
}else{
  // default case
}

// For loops
for(int i = 0; i < count; i++){
  // loop body
}

// While loops
while(condition){
  // loop body
}

// Switch statements
switch(value){
  case OPTION_A:
    handleA();
    break;
  case OPTION_B:
    handleB();
    break;
  default:
    handleDefault();
    break;
}
```

### Function Declarations

```cpp
// Short functions on one line if simple
void clear(){ fillScreen(RGB(0, 0, 0)); }

// Multi-line for complex functions
void setPixel(int x, int y, RGB color){
  if(x < 0 || x >= width || y < 0 || y >= height){
    return;
  }
  
  int panel_index = (dual_display_mode && x >= panel_width) ? 1 : 0;
  int local_x = dual_display_mode ? (x % panel_width) : x;
  
  framebuffer[panel_index][y * panel_width + local_x] = color;
}

// Function parameters - no space after opening parenthesis
void drawRect(int x, int y, int w, int h, RGB color){
  // implementation
}
```

### Class Definitions

```cpp
class HUB75Driver{
public:
  // Constructor
  HUB75Driver();
  
  // Public methods
  void init(HUB75Config config);
  void start();
  void stop();
  
  // Getters/setters
  void setBrightness(uint8_t brightness);
  uint8_t getBrightness() const;
  
private:
  // Private members
  int width;
  int height;
  RGB* framebuffer;
  
  // Private methods
  void convertToBCM();
  void updateDMA();
};
```

### Spacing and Indentation

```cpp
// Use 2 spaces for indentation (not tabs)
void example(){
  if(condition){
    int value = calculate();
    process(value);
  }
}

// No space after function name
setPixel(x, y, color);

// Space after keywords
if(condition){ }
while(running){ }
for(int i = 0; i < n; i++){ }

// Space around operators
int result = a + b * c;
bool flag = (x > 5) && (y < 10);

// No space in array/pointer access
buffer[index] = value;
ptr->member = data;
```

### Line Length

- **Preferred**: Keep lines under 100 characters
- **Maximum**: 120 characters
- Break long lines logically:

```cpp
// Break at logical points
RGB color = interpolate(
  start_color,
  end_color,
  progress
);

// Or align parameters
display.drawComplexShape(x, y, width, height,
                        color, border_width,
                        fill_mode, alpha);
```

---

## File Organization

### Header Files (.hpp)

```cpp
#pragma once

// System includes first
#include <stdint.h>
#include <stdbool.h>

// Framework includes
#include "esp_lcd_panel_io.h"
#include "esp_heap_caps.h"

// Project includes
#include "parallel_hardware_interface.hpp"
#include "dma_buffer_manager.hpp"

// Forward declarations if needed
class BufferManager;

// Class definition
class HUB75Driver{
public:
  // Public interface
  
private:
  // Private members
};
```

### Source Files (.cpp)

```cpp
// Include own header first
#include "hub75_driver.hpp"

// System includes
#include <string.h>
#include <math.h>

// Framework includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Project includes
#include "lcd_parallel.hpp"

// Implementation
HUB75Driver::HUB75Driver(){
  // constructor
}

void HUB75Driver::init(HUB75Config config){
  // method implementation
}
```

---

## Comments and Documentation

### File Headers

```cpp
/**
 * @file hub75_driver.cpp
 * @brief HUB75 LED matrix driver with BCM brightness control
 * 
 * Implements a high-performance driver for HUB75 LED panels using
 * ESP32-S3's LCD_CAM peripheral with DMA acceleration.
 */
```

### Function Documentation

```cpp
/**
 * Set a single pixel color
 * 
 * @param x X coordinate (0 to width-1)
 * @param y Y coordinate (0 to height-1)
 * @param color RGB color value
 */
void setPixel(int x, int y, RGB color){
  // implementation
}
```

### Inline Comments

```cpp
// Single-line comments for brief explanations
int brightness_scale = bcm_brightness >> 2;  // Map 0-255 to 0-63

// Multi-line comments for complex logic
// Calculate active BCM cycles based on brightness
// The fill-up strategy allocates max buffer space and fills
// only the needed portion with OE-enabled cycles
int active_cycles = base_bcm_length * brightness_scale;
int inactive_cycles = max_cycles - active_cycles;
```

### TODO Comments

```cpp
// TODO: Implement vertical flip for panel inversion
// FIXME: Handle edge case when brightness is 0
// NOTE: This assumes 64 pixels per row
// OPTIMIZE: Consider SIMD for gamma correction
```

---

## Best Practices

### Memory Management

```cpp
// Prefer RAII and smart pointers when possible
class Resource{
public:
  Resource(){
    data = (uint8_t*)heap_caps_malloc(SIZE, MALLOC_CAP_DMA);
  }
  
  ~Resource(){
    if(data){
      free(data);
      data = nullptr;
    }
  }
  
private:
  uint8_t* data;
};

// Check allocations
void* buffer = heap_caps_malloc(size, MALLOC_CAP_DMA);
if(!buffer){
  ESP_LOGE(TAG, "Failed to allocate buffer");
  return false;
}
```

### Error Handling

```cpp
// Return bool for success/failure
bool init(HUB75Config config){
  if(!validateConfig(config)){
    ESP_LOGE(TAG, "Invalid configuration");
    return false;
  }
  
  if(!allocateBuffers()){
    ESP_LOGE(TAG, "Failed to allocate buffers");
    return false;
  }
  
  return true;
}

// Use assertions for internal checks
void setPixel(int x, int y, RGB color){
  assert(x >= 0 && x < width);
  assert(y >= 0 && y < height);
  // implementation
}
```

### Const Correctness

```cpp
// Use const for parameters that won't be modified
void drawImage(const uint8_t* image_data, int width, int height);

// Const member functions
int getWidth() const{ return width; }
int getHeight() const{ return height; }

// Const references for large objects
void processConfig(const HUB75Config& config);
```

### Type Safety

```cpp
// Use explicit types
uint8_t brightness = 255;  // Not just 'int'
size_t buffer_size = 1024;
bool is_running = false;

// Avoid magic numbers
const int ROWS_PER_PANEL = 16;
const int BCM_LEVELS = 5;

// Use enums for options
enum ColorMode{
  RGB565,
  RGB888,
  GRAYSCALE
};
```

### Performance Considerations

```cpp
// Mark hot path functions inline
inline uint8_t convert8to5(uint8_t value){
  return value >> 3;
}

// Use const references to avoid copies
void processPixels(const std::vector<RGB>& pixels){
  for(const RGB& pixel : pixels){
    // process
  }
}

// Minimize function calls in tight loops
void convertBuffer(){
  // Cache frequently accessed values
  int w = width;
  int h = height;
  
  for(int y = 0; y < h; y++){
    for(int x = 0; x < w; x++){
      // tight loop
    }
  }
}
```

### Platform Abstraction

```cpp
// Define clean interfaces
class IPlatformHAL{
public:
  virtual ~IPlatformHAL() = default;
  
  virtual bool init() = 0;
  virtual void delayMs(uint32_t ms) = 0;
  virtual uint64_t getTimeMicros() = 0;
};

// Implement for specific platform
class ESP32HAL : public IPlatformHAL{
public:
  bool init() override{
    // ESP32-specific initialization
  }
  
  void delayMs(uint32_t ms) override{
    vTaskDelay(pdMS_TO_TICKS(ms));
  }
};
```

---

## Code Examples

### Good Example

```cpp
void HUB75Driver::setBrightness(uint8_t brightness){
  if(brightness != bcm_brightness){
    bcm_brightness = brightness;
    convertToBCM();
  }
}

void HUB75Driver::fillScreen(RGB color){
  int total_pixels = width * height;
  
  for(int i = 0; i < panel_count; i++){
    for(int j = 0; j < total_pixels / panel_count; j++){
      framebuffer[i][j] = color;
    }
  }
}

RGB HUB75Driver::getPixel(int x, int y){
  if(x < 0 || x >= width || y < 0 || y >= height){
    return RGB(0, 0, 0);
  }
  
  int panel_index = (dual_display_mode && x >= panel_width) ? 1 : 0;
  int local_x = dual_display_mode ? (x % panel_width) : x;
  
  return framebuffer[panel_index][y * panel_width + local_x];
}
```

### Bad Example (Avoid)

```cpp
// Poor naming, inconsistent style
void HUB75Driver::SetBrightness(uint8_t b) {  // Wrong: PascalCase for method
  if (b != bcm_brightness) {  // Wrong: space before opening brace
    bcm_brightness = b;  // Poor: single letter variable
    convertToBCM();
  }
}

// No bounds checking, unclear logic
void HUB75Driver::FillScreen(RGB color) {
  for (int i = 0; i < width * height; i++) {
    framebuffer[0][i] = color;  // Wrong: assumes single panel
  }
}

// Magic numbers, no error handling
RGB HUB75Driver::getPixel(int x, int y) {
  return framebuffer[0][y * 64 + x];  // Wrong: magic number, no validation
}
```

---

## Summary

**Key Points:**
1. Use **tighter formatting**: `}else{` not `} else {`
2. Use **snake_case** for variables, **camelCase** for functions, **PascalCase** for classes
3. Use **2 spaces** for indentation
4. Keep lines under **100 characters** (max 120)
5. Use **const correctness** and **explicit types**
6. **Comment complex logic**, keep simple code self-documenting
7. **Validate inputs** and handle errors gracefully
8. **Optimize hot paths** (BCM conversion, DMA operations)

Following these conventions ensures the codebase remains clean, consistent, and maintainable.
