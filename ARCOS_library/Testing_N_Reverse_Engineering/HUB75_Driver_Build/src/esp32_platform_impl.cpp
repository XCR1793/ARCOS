#include "esp32_platform_impl.hpp"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_clk_tree.h"
#include "esp_rom_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <cstring>
#include <cstdio>

/** Singleton instance */
static ESP32PlatformHAL* g_esp32_platform_hal = nullptr;

ESP32PlatformHAL::ESP32PlatformHAL()
  : current_log_level(LogLevel::INFO)
{
}

/** ============================================================================
 *  GPIO OPERATIONS
 *  ========================================================================= */

bool ESP32PlatformHAL::pinMode(PinNumber pin, PinMode mode){
  if(pin < 0) return false;
  
  gpio_config_t io_conf = {};
  io_conf.pin_bit_mask = (1ULL << pin);
  io_conf.intr_type = GPIO_INTR_DISABLE;
  
  switch(mode){
    case PinMode::OUTPUT:
      io_conf.mode = GPIO_MODE_OUTPUT;
      io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
      io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
      break;
    case PinMode::INPUT:
      io_conf.mode = GPIO_MODE_INPUT;
      io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
      io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
      break;
    case PinMode::INPUT_PULLUP:
      io_conf.mode = GPIO_MODE_INPUT;
      io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
      io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
      break;
    case PinMode::INPUT_PULLDOWN:
      io_conf.mode = GPIO_MODE_INPUT;
      io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
      io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
      break;
    default:
      return false;
  }
  
  return gpio_config(&io_conf) == ESP_OK;
}

bool ESP32PlatformHAL::setPinDriveStrength(PinNumber pin, PinDriveStrength strength){
  if(pin < 0) return false;
  
  gpio_drive_cap_t cap;
  switch(strength){
    case PinDriveStrength::WEAK:
      cap = GPIO_DRIVE_CAP_0;
      break;
    case PinDriveStrength::MEDIUM:
      cap = GPIO_DRIVE_CAP_1;
      break;
    case PinDriveStrength::STRONG:
      cap = GPIO_DRIVE_CAP_3;
      break;
    default:
      cap = GPIO_DRIVE_CAP_1;
  }
  
  return gpio_set_drive_capability((gpio_num_t)pin, cap) == ESP_OK;
}

bool ESP32PlatformHAL::digitalWrite(PinNumber pin, bool value){
  if(pin < 0) return false;
  return gpio_set_level((gpio_num_t)pin, value ? 1 : 0) == ESP_OK;
}

bool ESP32PlatformHAL::digitalRead(PinNumber pin){
  if(pin < 0) return false;
  return gpio_get_level((gpio_num_t)pin) != 0;
}

bool ESP32PlatformHAL::connectPinToSignal(PinNumber pin, uint32_t signal, bool invert){
  if(pin < 0) return false;
  esp_rom_gpio_connect_out_signal(pin, signal, invert, false);
  return true;
}

/** ============================================================================
 *  MEMORY OPERATIONS
 *  ========================================================================= */

void* ESP32PlatformHAL::allocateMemory(size_t size, uint32_t caps){
  uint32_t esp_caps = 0;
  
  // Map platform-agnostic caps to ESP32 capabilities
  if(caps & MEM_CAP_DMA){
    esp_caps |= MALLOC_CAP_DMA;
  }
  if(caps & MEM_CAP_32BIT_ALIGNED){
    esp_caps |= MALLOC_CAP_32BIT;
  }
  if(caps & MEM_CAP_INTERNAL){
    esp_caps |= MALLOC_CAP_INTERNAL;
  }
  if(caps & MEM_CAP_EXTERNAL){
    esp_caps |= MALLOC_CAP_SPIRAM;
  }
  if(caps & MEM_CAP_IRAM){
    esp_caps |= MALLOC_CAP_IRAM_8BIT;
  }
  if(caps == MEM_CAP_DEFAULT){
    esp_caps = MALLOC_CAP_8BIT;
  }
  
  return heap_caps_malloc(size, esp_caps);
}

void ESP32PlatformHAL::freeMemory(void* ptr){
  if(ptr){
    heap_caps_free(ptr);
  }
}

size_t ESP32PlatformHAL::getTotalMemory(uint32_t caps){
  uint32_t esp_caps = MALLOC_CAP_8BIT;
  if(caps & MEM_CAP_DMA) esp_caps = MALLOC_CAP_DMA;
  return heap_caps_get_total_size(esp_caps);
}

size_t ESP32PlatformHAL::getFreeMemory(uint32_t caps){
  uint32_t esp_caps = MALLOC_CAP_8BIT;
  if(caps & MEM_CAP_DMA) esp_caps = MALLOC_CAP_DMA;
  return heap_caps_get_free_size(esp_caps);
}

/** ============================================================================
 *  TIMING OPERATIONS
 *  ========================================================================= */

uint64_t ESP32PlatformHAL::getMicros(){
  return esp_timer_get_time();
}

uint32_t ESP32PlatformHAL::getMillis(){
  return (uint32_t)(esp_timer_get_time() / 1000);
}

void ESP32PlatformHAL::delayMicros(uint32_t us){
  esp_rom_delay_us(us);
}

void ESP32PlatformHAL::delayMillis(uint32_t ms){
  vTaskDelay(pdMS_TO_TICKS(ms));
}

/** ============================================================================
 *  LOGGING OPERATIONS
 *  ========================================================================= */

void ESP32PlatformHAL::setLogLevel(LogLevel level){
  current_log_level = level;
  
  // Map to ESP log level
  esp_log_level_t esp_level;
  switch(level){
    case LogLevel::NONE:
      esp_level = ESP_LOG_NONE;
      break;
    case LogLevel::ERROR:
      esp_level = ESP_LOG_ERROR;
      break;
    case LogLevel::WARN:
      esp_level = ESP_LOG_WARN;
      break;
    case LogLevel::INFO:
      esp_level = ESP_LOG_INFO;
      break;
    case LogLevel::DEBUG:
      esp_level = ESP_LOG_DEBUG;
      break;
    case LogLevel::VERBOSE:
      esp_level = ESP_LOG_VERBOSE;
      break;
    default:
      esp_level = ESP_LOG_INFO;
  }
  
  esp_log_level_set("*", esp_level);
}

void ESP32PlatformHAL::log(LogLevel level, const char* tag, const char* format, ...){
  // Check if this log level should be printed
  if(level > current_log_level){
    return;
  }
  
  // Format the message
  va_list args;
  va_start(args, format);
  vsnprintf(log_buffer, sizeof(log_buffer), format, args);
  va_end(args);
  
  // Map to ESP log function
  switch(level){
    case LogLevel::ERROR:
      ESP_LOGE(tag, "%s", log_buffer);
      break;
    case LogLevel::WARN:
      ESP_LOGW(tag, "%s", log_buffer);
      break;
    case LogLevel::INFO:
      ESP_LOGI(tag, "%s", log_buffer);
      break;
    case LogLevel::DEBUG:
      ESP_LOGD(tag, "%s", log_buffer);
      break;
    case LogLevel::VERBOSE:
      ESP_LOGV(tag, "%s", log_buffer);
      break;
    default:
      break;
  }
}

/** ============================================================================
 *  PLATFORM INFORMATION
 *  ========================================================================= */

const char* ESP32PlatformHAL::getPlatformName(){
#ifdef CONFIG_IDF_TARGET_ESP32
  return "ESP32";
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
  return "ESP32-S2";
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  return "ESP32-S3";
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  return "ESP32-C3";
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  return "ESP32-C6";
#elif defined(CONFIG_IDF_TARGET_ESP32H2)
  return "ESP32-H2";
#else
  return "ESP32-Unknown";
#endif
}

uint32_t ESP32PlatformHAL::getCpuFrequency(){
  // Get CPU frequency from FreeRTOS tick rate (more reliable across IDF versions)
  uint32_t freq_hz = 0;
  esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &freq_hz);
  return freq_hz;
}

/** ============================================================================
 *  SINGLETON ACCESS
 *  ========================================================================= */

ESP32PlatformHAL* getESP32PlatformHAL(){
  if(!g_esp32_platform_hal){
    g_esp32_platform_hal = new ESP32PlatformHAL();
  }
  return g_esp32_platform_hal;
}

/** Implement global HAL accessor */
IPlatformHAL* getPlatformHAL(){
  return getESP32PlatformHAL();
}
