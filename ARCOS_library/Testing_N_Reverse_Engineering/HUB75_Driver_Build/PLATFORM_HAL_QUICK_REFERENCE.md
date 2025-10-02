# Quick Reference: Platform Abstraction API

## Including the Platform HAL

```cpp
#include "platform_hal.hpp"       // Core interface
#include "esp32_platform_impl.hpp" // ESP32 implementation (in main.cpp)
```

## Getting Platform HAL Instance

```cpp
IPlatformHAL* hal = getPlatformHAL();
```

## GPIO Operations

```cpp
// Configure pin mode
hal->pinMode(LED_PIN, PinMode::OUTPUT);
hal->pinMode(BUTTON_PIN, PinMode::INPUT_PULLUP);

// Set drive strength
hal->setPinDriveStrength(LED_PIN, PinDriveStrength::STRONG);

// Digital I/O
hal->digitalWrite(LED_PIN, true);   // Set HIGH
bool state = hal->digitalRead(BUTTON_PIN);

// Connect pin to peripheral signal (platform-specific signal IDs)
hal->connectPinToSignal(pin, SIGNAL_ID, false);
```

## Memory Operations

```cpp
// Allocate DMA-capable memory
void* dma_buf = hal->allocateMemory(1024, MEM_CAP_DMA);

// Allocate default memory
void* buf = hal->allocateMemory(512, MEM_CAP_DEFAULT);

// Allocate internal memory (faster)
void* fast_buf = hal->allocateMemory(256, MEM_CAP_INTERNAL);

// Free memory
hal->freeMemory(dma_buf);

// Query memory
size_t total = hal->getTotalMemory(MEM_CAP_DMA);
size_t free = hal->getFreeMemory(MEM_CAP_DMA);
```

## Timing Operations

```cpp
// Get current time
uint64_t us = hal->getMicros();      // Microseconds
uint32_t ms = hal->getMillis();      // Milliseconds

// Delays
hal->delayMicros(100);               // 100 microseconds
hal->delayMillis(1000);              // 1 second
```

## Logging

```cpp
// Using macros (recommended)
PLATFORM_LOG_E(TAG, "Error: %d", error_code);      // Error
PLATFORM_LOG_W(TAG, "Warning: %s", message);       // Warning  
PLATFORM_LOG_I(TAG, "Info: frequency=%d Hz", freq); // Info
PLATFORM_LOG_D(TAG, "Debug: ptr=%p", pointer);     // Debug
PLATFORM_LOG_V(TAG, "Verbose: value=%.2f", val);   // Verbose

// Set log level
hal->setLogLevel(LogLevel::INFO);

// Direct logging (advanced)
hal->log(LogLevel::ERROR, "MY_TAG", "Error %d", code);
```

## Platform Information

```cpp
// Get platform name
const char* platform = hal->getPlatformName();  // "ESP32-S3", "STM32F4", etc.

// Get CPU frequency
uint32_t freq = hal->getCpuFrequency();  // Hz
```

## Platform-Agnostic Types

### Pin Types
```cpp
PinNumber pin = 5;        // Generic pin number
PinNumber nc = PIN_NC;    // Not connected (-1)
```

### Pin Modes
```cpp
PinMode::OUTPUT           // Digital output
PinMode::INPUT            // Digital input
PinMode::INPUT_PULLUP     // Input with pull-up
PinMode::INPUT_PULLDOWN   // Input with pull-down
PinMode::ANALOG           // Analog I/O
```

### Drive Strength
```cpp
PinDriveStrength::WEAK    // Minimum drive
PinDriveStrength::MEDIUM  // Medium drive (default)
PinDriveStrength::STRONG  // Maximum drive
```

### Memory Capabilities
```cpp
MEM_CAP_DEFAULT          // Standard RAM
MEM_CAP_DMA              // DMA-capable memory
MEM_CAP_32BIT_ALIGNED    // 32-bit aligned
MEM_CAP_INTERNAL         // Internal RAM (faster)
MEM_CAP_EXTERNAL         // External RAM/PSRAM
MEM_CAP_IRAM             // Instruction RAM
MEM_CAP_CACHE_SAFE       // Cache-safe memory

// Combine flags with bitwise OR
uint32_t caps = MEM_CAP_DMA | MEM_CAP_INTERNAL;
```

### Log Levels
```cpp
LogLevel::NONE           // No logging
LogLevel::ERROR          // Errors only
LogLevel::WARN           // Warnings and errors
LogLevel::INFO           // Info, warnings, and errors
LogLevel::DEBUG          // Debug and above
LogLevel::VERBOSE        // All messages
```

## Migration Examples

### Example 1: GPIO Configuration

**Before (ESP32):**
```cpp
#include "driver/gpio.h"

gpio_num_t pin = GPIO_NUM_5;
gpio_config_t cfg = {
  .pin_bit_mask = (1ULL << pin),
  .mode = GPIO_MODE_OUTPUT,
  // ...
};
gpio_config(&cfg);
gpio_set_drive_capability(pin, GPIO_DRIVE_CAP_3);
gpio_set_level(pin, 1);
```

**After (Platform-agnostic):**
```cpp
#include "platform_hal.hpp"

PinNumber pin = 5;
IPlatformHAL* hal = getPlatformHAL();
hal->pinMode(pin, PinMode::OUTPUT);
hal->setPinDriveStrength(pin, PinDriveStrength::STRONG);
hal->digitalWrite(pin, true);
```

### Example 2: Memory Allocation

**Before (ESP32):**
```cpp
#include "esp_heap_caps.h"

void* buf = heap_caps_malloc(1024, MALLOC_CAP_DMA);
if(!buf) {
  ESP_LOGE(TAG, "Allocation failed");
  return;
}
// ... use buffer
heap_caps_free(buf);
```

**After (Platform-agnostic):**
```cpp
#include "platform_hal.hpp"

IPlatformHAL* hal = getPlatformHAL();
void* buf = hal->allocateMemory(1024, MEM_CAP_DMA);
if(!buf) {
  PLATFORM_LOG_E(TAG, "Allocation failed");
  return;
}
// ... use buffer
hal->freeMemory(buf);
```

### Example 3: Timing

**Before (ESP32):**
```cpp
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

uint64_t start = esp_timer_get_time();
esp_rom_delay_us(100);
uint64_t elapsed = esp_timer_get_time() - start;
vTaskDelay(pdMS_TO_TICKS(1000));
```

**After (Platform-agnostic):**
```cpp
#include "platform_hal.hpp"

IPlatformHAL* hal = getPlatformHAL();
uint64_t start = hal->getMicros();
hal->delayMicros(100);
uint64_t elapsed = hal->getMicros() - start;
hal->delayMillis(1000);
```

### Example 4: Logging

**Before (ESP32):**
```cpp
#include "esp_log.h"

static const char* TAG = "MY_MODULE";
ESP_LOGE(TAG, "Error: %d", error_code);
ESP_LOGW(TAG, "Warning: %s", message);
ESP_LOGI(TAG, "Info: %d Hz", frequency);
ESP_LOGD(TAG, "Debug data");
```

**After (Platform-agnostic):**
```cpp
#include "platform_hal.hpp"

static const char* TAG = "MY_MODULE";
PLATFORM_LOG_E(TAG, "Error: %d", error_code);
PLATFORM_LOG_W(TAG, "Warning: %s", message);
PLATFORM_LOG_I(TAG, "Info: %d Hz", frequency);
PLATFORM_LOG_D(TAG, "Debug data");
```

## Complete Example: LED Blink

### Platform-Agnostic Version
```cpp
#include "platform_hal.hpp"

static const char* TAG = "LED_BLINK";
static const PinNumber LED_PIN = 5;

void setup() {
  IPlatformHAL* hal = getPlatformHAL();
  
  PLATFORM_LOG_I(TAG, "Platform: %s", hal->getPlatformName());
  PLATFORM_LOG_I(TAG, "CPU: %d MHz", hal->getCpuFrequency() / 1000000);
  
  hal->pinMode(LED_PIN, PinMode::OUTPUT);
  hal->setPinDriveStrength(LED_PIN, PinDriveStrength::MEDIUM);
}

void loop() {
  IPlatformHAL* hal = getPlatformHAL();
  
  hal->digitalWrite(LED_PIN, true);
  PLATFORM_LOG_D(TAG, "LED ON");
  hal->delayMillis(1000);
  
  hal->digitalWrite(LED_PIN, false);
  PLATFORM_LOG_D(TAG, "LED OFF");
  hal->delayMillis(1000);
}
```

This code works unchanged on ESP32, STM32, RP2040, or any platform with a HAL implementation!

## Common Patterns

### Pattern 1: DMA Buffer Allocation
```cpp
IPlatformHAL* hal = getPlatformHAL();

// Allocate DMA buffer
size_t size = 1024 * sizeof(uint16_t);
uint16_t* dma_buf = static_cast<uint16_t*>(
  hal->allocateMemory(size, MEM_CAP_DMA)
);

if(!dma_buf) {
  PLATFORM_LOG_E(TAG, "Failed to allocate DMA buffer");
  return false;
}

// Clear buffer
memset(dma_buf, 0, size);

// Use buffer...

// Free when done
hal->freeMemory(dma_buf);
```

### Pattern 2: Performance Timing
```cpp
IPlatformHAL* hal = getPlatformHAL();

uint64_t start = hal->getMicros();

// ... do work ...

uint64_t elapsed = hal->getMicros() - start;
PLATFORM_LOG_I(TAG, "Operation took %llu us", elapsed);
```

### Pattern 3: Conditional Logging
```cpp
#if DEBUG_MODE
hal->setLogLevel(LogLevel::DEBUG);
#else
hal->setLogLevel(LogLevel::INFO);
#endif
```

## Best Practices

1. **Get HAL once**: Cache the HAL pointer instead of calling `getPlatformHAL()` repeatedly
2. **Check allocations**: Always check if memory allocation succeeded
3. **Free memory**: Always free allocated memory when done
4. **Use macros for logging**: `PLATFORM_LOG_*` macros are more efficient
5. **Use strong types**: Prefer `PinNumber` over `int` for clarity
6. **Document platform requirements**: If your code needs specific capabilities

## Troubleshooting

### Problem: "undefined reference to getPlatformHAL"
**Solution**: Include platform implementation in your main file:
```cpp
#include "esp32_platform_impl.hpp"  // or other platform
```

### Problem: DMA allocation fails
**Solution**: Check available DMA memory:
```cpp
size_t free = hal->getFreeMemory(MEM_CAP_DMA);
PLATFORM_LOG_I(TAG, "Free DMA memory: %d bytes", free);
```

### Problem: No log output
**Solution**: Set log level:
```cpp
hal->setLogLevel(LogLevel::DEBUG);
```

---

**Quick Start**: See `PLATFORM_ABSTRACTION_README.md` for full documentation.
