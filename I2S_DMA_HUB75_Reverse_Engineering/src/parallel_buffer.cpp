#include "parallel_buffer.hpp"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <cstring>

static const char* TAG = "PARALLEL_BUFFER";

ParallelBuffer::ParallelBuffer() 
  : buffer(nullptr)
  , buffer_size(0)
  , owns_buffer(false)
{
}

ParallelBuffer::~ParallelBuffer() {
  free();
}

bool ParallelBuffer::alloc(size_t sample_count){
  if(sample_count == 0){
    ESP_LOGE(TAG, "Sample count cannot be zero");
    return false;
  }

  // Free existing buffer if allocated
  if(buffer){
    free();
  }
  
  size_t buffer_bytes = sample_count * sizeof(uint16_t);
  buffer = static_cast<uint16_t*>(heap_caps_malloc(buffer_bytes, MALLOC_CAP_DMA));
  
  if(!buffer){
    ESP_LOGE(TAG, "Failed to allocate DMA buffer (%d bytes)", buffer_bytes);
    buffer_size = 0;
    return false;
  }
  
  buffer_size = sample_count;
  owns_buffer = true;
  ESP_LOGI(TAG, "Allocated DMA buffer: %d samples (%d bytes)", sample_count, buffer_bytes);
  
  /** Initialize to zero */
  std::memset(buffer, 0, buffer_bytes);
  
  return true;
}

void ParallelBuffer::free(){
  if(buffer && owns_buffer){
    heap_caps_free(buffer);
    ESP_LOGI(TAG, "DMA buffer freed");
  }
  buffer = nullptr;
  buffer_size = 0;
  owns_buffer = false;
}

uint16_t* ParallelBuffer::getBuffer() const {
  return buffer;
}

size_t ParallelBuffer::getSize() const {
  return buffer_size;
}

bool ParallelBuffer::fillPattern(size_t high_samples, size_t low_samples,
                                 uint16_t high_value, uint16_t low_value){
  if(!buffer || buffer_size == 0){
    ESP_LOGE(TAG, "No buffer allocated");
    return false;
  }
  
  if(high_samples + low_samples > buffer_size){
    ESP_LOGE(TAG, "Pattern size (%d) exceeds buffer size (%d)", 
             high_samples + low_samples, buffer_size);
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
  if(pattern_size < buffer_size){
    ESP_LOGI(TAG, "Repeating pattern to fill buffer");
    for(size_t i = pattern_size; i < buffer_size; i++){
      buffer[i] = buffer[i % pattern_size];
    }
  }
  
  return true;
}

void ParallelBuffer::fillSolid(uint16_t value){
  if(!buffer || buffer_size == 0){
    ESP_LOGE(TAG, "No buffer allocated");
    return;
  }
  
  ESP_LOGI(TAG, "Filling buffer with solid value: 0x%04X (%d samples)", value, buffer_size);
  
  for(size_t i = 0; i < buffer_size; i++){
    buffer[i] = value;
  }
}

bool ParallelBuffer::createTiming(uint32_t high_duration_ms, uint32_t low_duration_ms,
                                  uint32_t sample_rate_hz, uint16_t high_value, uint16_t low_value){
  if(!buffer || buffer_size == 0 || sample_rate_hz == 0){
    ESP_LOGE(TAG, "Invalid parameters or no buffer allocated");
    return false;
  }
  
  /** Calculate samples needed for each phase */
  size_t high_samples = (high_duration_ms * sample_rate_hz) / 1000;
  size_t low_samples = (low_duration_ms * sample_rate_hz) / 1000;
  size_t total_needed = high_samples + low_samples;
  
  ESP_LOGI(TAG, "Creating timing pattern:");
  ESP_LOGI(TAG, "  HIGH: %d ms -> %d samples", high_duration_ms, high_samples);
  ESP_LOGI(TAG, "  LOW:  %d ms -> %d samples", low_duration_ms, low_samples);
  ESP_LOGI(TAG, "  Total needed: %d samples, Available: %d samples", total_needed, buffer_size);
  
  if(total_needed > buffer_size){
    ESP_LOGE(TAG, "Timing pattern requires %d samples but buffer only has %d", 
             total_needed, buffer_size);
    return false;
  }
  
  return fillPattern(high_samples, low_samples, high_value, low_value);
}

bool ParallelBuffer::setDirectPointer(uint16_t* external_buffer, size_t size){
  if(!external_buffer || size == 0){
    ESP_LOGE(TAG, "Invalid external buffer pointer or size");
    return false;
  }
  
  // Free our own buffer if we own it
  if(buffer && owns_buffer){
    heap_caps_free(buffer);
  }
  
  buffer = external_buffer;
  buffer_size = size;
  owns_buffer = false;
  
  ESP_LOGI(TAG, "Set direct pointer: %d samples (zero-copy mode)", size);
  return true;
}

uint16_t* ParallelBuffer::getDirectAccess() const{
  return buffer;
}