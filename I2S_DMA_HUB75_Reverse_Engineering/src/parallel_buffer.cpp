#include "parallel_buffer.hpp"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <cstring>

static const char* TAG = "PARALLEL_BUFFER";

uint16_t* ParallelBuffer::alloc(size_t sample_count){
  if(sample_count == 0){
    ESP_LOGE(TAG, "Sample count cannot be zero");
    return nullptr;
  }
  
  size_t buffer_size = sample_count * sizeof(uint16_t);
  uint16_t* buffer = static_cast<uint16_t*>(heap_caps_malloc(buffer_size, MALLOC_CAP_DMA));
  
  if(!buffer){
    ESP_LOGE(TAG, "Failed to allocate DMA buffer (%d bytes)", buffer_size);
    return nullptr;
  }
  
  ESP_LOGI(TAG, "Allocated DMA buffer: %d samples (%d bytes)", sample_count, buffer_size);
  
  /** Initialize to zero */
  std::memset(buffer, 0, buffer_size);
  
  return buffer;
}

void ParallelBuffer::free(uint16_t* buffer){
  if(buffer){
    heap_caps_free(buffer);
    ESP_LOGI(TAG, "DMA buffer freed");
  }
}

bool ParallelBuffer::fillPattern(uint16_t* buffer, size_t sample_count,
                                size_t high_samples, size_t low_samples,
                                uint16_t high_value, uint16_t low_value){
  if(!buffer || sample_count == 0){
    ESP_LOGE(TAG, "Invalid buffer parameters");
    return false;
  }
  
  if(high_samples + low_samples > sample_count){
    ESP_LOGE(TAG, "Pattern size (%d) exceeds buffer size (%d)", 
             high_samples + low_samples, sample_count);
    return false;
  }
  
  ESP_LOGI(TAG, "Filling pattern: %d HIGH (0x%04X), %d LOW (0x%04X)", 
           high_samples, high_value, low_samples, low_value);
  
  /** Fill HIGH samples */
  for(size_t i = 0; i < high_samples; i++){
    buffer[i] = high_value;
  }
  
  /** Fill LOW samples */
  for(size_t i = high_samples; i < high_samples + low_samples; i++){
    buffer[i] = low_value;
  }
  
  /** If pattern is smaller than buffer, repeat it */
  size_t pattern_size = high_samples + low_samples;
  if(pattern_size < sample_count){
    ESP_LOGI(TAG, "Repeating pattern to fill buffer");
    for(size_t i = pattern_size; i < sample_count; i++){
      buffer[i] = buffer[i % pattern_size];
    }
  }
  
  return true;
}

void ParallelBuffer::fillSolid(uint16_t* buffer, size_t sample_count, uint16_t value){
  if(!buffer || sample_count == 0){
    return;
  }
  
  ESP_LOGI(TAG, "Filling buffer with solid value: 0x%04X (%d samples)", value, sample_count);
  
  for(size_t i = 0; i < sample_count; i++){
    buffer[i] = value;
  }
}

bool ParallelBuffer::createTiming(uint16_t* buffer, size_t sample_count,
                                 uint32_t high_duration_ms, uint32_t low_duration_ms,
                                 uint32_t sample_rate_hz, uint16_t high_value, uint16_t low_value){
  if(!buffer || sample_count == 0 || sample_rate_hz == 0){
    ESP_LOGE(TAG, "Invalid parameters");
    return false;
  }
  
  /** Calculate samples needed for each phase */
  size_t high_samples = (high_duration_ms * sample_rate_hz) / 1000;
  size_t low_samples = (low_duration_ms * sample_rate_hz) / 1000;
  size_t total_needed = high_samples + low_samples;
  
  ESP_LOGI(TAG, "Creating timing pattern:");
  ESP_LOGI(TAG, "  HIGH: %d ms -> %d samples", high_duration_ms, high_samples);
  ESP_LOGI(TAG, "  LOW:  %d ms -> %d samples", low_duration_ms, low_samples);
  ESP_LOGI(TAG, "  Total needed: %d samples, Available: %d samples", total_needed, sample_count);
  
  if(total_needed > sample_count){
    ESP_LOGE(TAG, "Timing pattern requires %d samples but buffer only has %d", 
             total_needed, sample_count);
    return false;
  }
  
  return fillPattern(buffer, sample_count, high_samples, low_samples, high_value, low_value);
}