/*****************************************************************
 * File:      parallel_buffer_impl.hpp
 * Category:  abstraction/platforms/esp32/wroom32s3/module
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:    Parallel buffer management implementation
 *****************************************************************/

#include "../../../../core/hal_protocal_parallel_buffer.hpp"
#include <cstring>
#include "esp_log.h"
#include "esp_heap_caps.h"

namespace arcos::abstraction{

using dma::BufferMode;

namespace{
static const char* PARALLEL_BUFFER_TAG = "PARALLEL_BUFFER";
}

ParallelBuffer::ParallelBuffer() 
  : buffers(nullptr)
  , num_buffers(0)
  , buffer_size(0)
  , front_index(0)
  , back_index(0)
  , mode(BufferMode::SINGLE_BUFFER)
  , owns_buffer(nullptr)
  , initialized(false)
  , buffer(nullptr)
  , legacy_owns_buffer(false)
{
}

ParallelBuffer::~ParallelBuffer() {
  free();
}

bool ParallelBuffer::alloc(size_t sample_count){
  if(sample_count == 0){
    ESP_LOGE(PARALLEL_BUFFER_TAG, "Sample count cannot be zero");
    return false;
  }

  // Free existing buffer if allocated
  if(buffer){
    free();
  }
  
  size_t buffer_bytes = sample_count * sizeof(uint16_t);
  buffer = static_cast<uint16_t*>(heap_caps_malloc(buffer_bytes, MALLOC_CAP_DMA));
  
  if(!buffer){
    ESP_LOGE(PARALLEL_BUFFER_TAG, "Failed to allocate DMA buffer (%d bytes)", buffer_bytes);
    buffer_size = 0;
    return false;
  }
  
  buffer_size = sample_count;
  legacy_owns_buffer = true;
  ESP_LOGI(PARALLEL_BUFFER_TAG, "Allocated DMA buffer: %d samples (%d bytes)", sample_count, buffer_bytes);
  
  /** Initialize to zero */
  std::memset(buffer, 0, buffer_bytes);
  
  return true;
}

void ParallelBuffer::free(){
  // Free new multi-buffer system
  if(buffers){
    for(size_t i = 0; i < num_buffers; i++){
      if(buffers[i] && owns_buffer && owns_buffer[i]){
        heap_caps_free(buffers[i]);
      }
    }
    delete[] buffers;
    buffers = nullptr;
  }
  
  if(owns_buffer){
    delete[] owns_buffer;
    owns_buffer = nullptr;
  }
  
  // Free legacy single buffer
  if(buffer && legacy_owns_buffer){
    heap_caps_free(buffer);
    ESP_LOGI(PARALLEL_BUFFER_TAG, "DMA buffer freed");
  }
  buffer = nullptr;
  
  buffer_size = 0;
  num_buffers = 0;
  legacy_owns_buffer = false;
  initialized = false;
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
    ESP_LOGE(PARALLEL_BUFFER_TAG, "No buffer allocated");
    return false;
  }
  
  if(high_samples + low_samples > buffer_size){
    ESP_LOGE(PARALLEL_BUFFER_TAG, "Pattern size (%d) exceeds buffer size (%d)", 
             high_samples + low_samples, buffer_size);
    return false;
  }
  
  ESP_LOGI(PARALLEL_BUFFER_TAG, "Filling pattern: %d HIGH (0x%04X), %d LOW (0x%04X)", 
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
    ESP_LOGI(PARALLEL_BUFFER_TAG, "Repeating pattern to fill buffer");
    for(size_t i = pattern_size; i < buffer_size; i++){
      buffer[i] = buffer[i % pattern_size];
    }
  }
  
  return true;
}

void ParallelBuffer::fillSolid(uint16_t value){
  if(!buffer || buffer_size == 0){
    ESP_LOGE(PARALLEL_BUFFER_TAG, "No buffer allocated");
    return;
  }
  
  ESP_LOGI(PARALLEL_BUFFER_TAG, "Filling buffer with solid value: 0x%04X (%d samples)", value, buffer_size);
  
  for(size_t i = 0; i < buffer_size; i++){
    buffer[i] = value;
  }
}

bool ParallelBuffer::createTiming(uint32_t high_duration_ms, uint32_t low_duration_ms,
                                  uint32_t sample_rate_hz, uint16_t high_value, uint16_t low_value){
  if(!buffer || buffer_size == 0 || sample_rate_hz == 0){
    ESP_LOGE(PARALLEL_BUFFER_TAG, "Invalid parameters or no buffer allocated");
    return false;
  }
  
  /** Calculate samples needed for each phase */
  size_t high_samples = (high_duration_ms * sample_rate_hz) / 1000;
  size_t low_samples = (low_duration_ms * sample_rate_hz) / 1000;
  size_t total_needed = high_samples + low_samples;
  
  ESP_LOGI(PARALLEL_BUFFER_TAG, "Creating timing pattern:");
  ESP_LOGI(PARALLEL_BUFFER_TAG, "  HIGH: %d ms -> %d samples", high_duration_ms, high_samples);
  ESP_LOGI(PARALLEL_BUFFER_TAG, "  LOW:  %d ms -> %d samples", low_duration_ms, low_samples);
  ESP_LOGI(PARALLEL_BUFFER_TAG, "  Total needed: %d samples, Available: %d samples", total_needed, buffer_size);
  
  if(total_needed > buffer_size){
    ESP_LOGE(PARALLEL_BUFFER_TAG, "Timing pattern requires %d samples but buffer only has %d", 
             total_needed, buffer_size);
    return false;
  }
  
  return fillPattern(high_samples, low_samples, high_value, low_value);
}

bool ParallelBuffer::setDirectPointer(uint16_t* external_buffer, size_t size){
  if(!external_buffer || size == 0){
    ESP_LOGE(PARALLEL_BUFFER_TAG, "Invalid external buffer pointer or size");
    return false;
  }
  
  // Free our own buffer if we own it
  if(buffer && legacy_owns_buffer){
    heap_caps_free(buffer);
  }
  
  buffer = external_buffer;
  buffer_size = size;
  legacy_owns_buffer = false;
  
  ESP_LOGI(PARALLEL_BUFFER_TAG, "Set direct pointer: %d samples (zero-copy mode)", size);
  return true;
}

uint16_t* ParallelBuffer::getDirectAccess() const{
  return buffer;
}

/** IDmaBufferManager interface implementations */

bool ParallelBuffer::init(const DmaBufferConfig& config){
  if(initialized){
    ESP_LOGW(PARALLEL_BUFFER_TAG, "Buffer manager already initialized");
    return true;
  }
  
  mode = config.mode;
  num_buffers = config.buffer_count;
  
  if(config.auto_allocate && config.sample_count > 0){
    return allocate(config.sample_count);
  }
  
  // Just initialize structure without allocating
  buffers = new uint16_t*[num_buffers];
  owns_buffer = new bool[num_buffers];
  
  for(size_t i = 0; i < num_buffers; i++){
    buffers[i] = nullptr;
    owns_buffer[i] = false;
  }
  
  back_index = (mode == BufferMode::DOUBLE_BUFFER && num_buffers >= 2) ? 1 : 0;
  initialized = true;
  
  ESP_LOGI(PARALLEL_BUFFER_TAG, "Buffer manager initialized: %d buffers, mode=%d", num_buffers, (int)mode);
  return true;
}

bool ParallelBuffer::allocate(size_t sample_count){
  if(sample_count == 0){
    ESP_LOGE(PARALLEL_BUFFER_TAG, "Sample count cannot be zero");
    return false;
  }
  
  // Free existing buffers
  if(buffers){
    free();
  }
  
  // Use default configuration if not initialized
  if(!initialized){
    DmaBufferConfig default_config;
    default_config.sample_count = sample_count;
    default_config.auto_allocate = false;
    if(!init(default_config)){
      return false;
    }
  }
  
  buffer_size = sample_count;
  size_t buffer_bytes = sample_count * sizeof(uint16_t);
  
  // Allocate all buffers
  for(size_t i = 0; i < num_buffers; i++){
    buffers[i] = static_cast<uint16_t*>(heap_caps_malloc(buffer_bytes, MALLOC_CAP_DMA));
    
    if(!buffers[i]){
      ESP_LOGE(PARALLEL_BUFFER_TAG, "Failed to allocate DMA buffer %d (%d bytes)", i, buffer_bytes);
      // Clean up partially allocated buffers
      for(size_t j = 0; j < i; j++){
        if(buffers[j]){
          heap_caps_free(buffers[j]);
          buffers[j] = nullptr;
        }
      }
      return false;
    }
    
    owns_buffer[i] = true;
    std::memset(buffers[i], 0, buffer_bytes);
  }
  
  ESP_LOGI(PARALLEL_BUFFER_TAG, "Allocated %d DMA buffers: %d samples each (%d bytes total)", 
           num_buffers, sample_count, buffer_bytes * num_buffers);
  
  return true;
}

uint16_t* ParallelBuffer::getFrontBuffer() const {
  if(!buffers || num_buffers == 0){
    return buffer;  // Fall back to legacy single buffer
  }
  return buffers[front_index];
}

uint16_t* ParallelBuffer::getBackBuffer() const {
  if(!buffers || num_buffers < 2){
    return nullptr;  // No back buffer in single buffer mode
  }
  return buffers[back_index];
}

uint16_t* ParallelBuffer::getBuffer(size_t index) const {
  if(!buffers || index >= num_buffers){
    if(index == 0){
      return buffer;  // Fall back to legacy single buffer
    }
    return nullptr;
  }
  return buffers[index];
}

size_t ParallelBuffer::getBufferCount() const {
  return buffers ? num_buffers : (buffer ? 1 : 0);
}

size_t ParallelBuffer::getBufferSize() const {
  return buffer_size;
}

bool ParallelBuffer::swapBuffers(){
  if(mode != BufferMode::DOUBLE_BUFFER || num_buffers < 2){
    ESP_LOGW(PARALLEL_BUFFER_TAG, "Cannot swap buffers - not in double buffer mode");
    return false;
  }
  
  // Swap front and back indices
  size_t temp = front_index;
  front_index = back_index;
  back_index = temp;
  
  return true;
}

BufferMode ParallelBuffer::getMode() const {
  return mode;
}

bool ParallelBuffer::fillBuffer(size_t buffer_index, uint16_t value){
  uint16_t* target_buffer = getBuffer(buffer_index);
  if(!target_buffer || buffer_size == 0){
    ESP_LOGE(PARALLEL_BUFFER_TAG, "Invalid buffer index or buffer not allocated");
    return false;
  }
  
  for(size_t i = 0; i < buffer_size; i++){
    target_buffer[i] = value;
  }
  
  ESP_LOGI(PARALLEL_BUFFER_TAG, "Filled buffer %d with value 0x%04X (%d samples)", buffer_index, value, buffer_size);
  return true;
}

bool ParallelBuffer::fillPattern(size_t buffer_index, size_t high_samples, size_t low_samples,
                                 uint16_t high_value, uint16_t low_value){
  uint16_t* target_buffer = getBuffer(buffer_index);
  if(!target_buffer || buffer_size == 0){
    ESP_LOGE(PARALLEL_BUFFER_TAG, "Invalid buffer index or buffer not allocated");
    return false;
  }
  
  if(high_samples + low_samples > buffer_size){
    ESP_LOGE(PARALLEL_BUFFER_TAG, "Pattern size (%d) exceeds buffer size (%d)", 
             high_samples + low_samples, buffer_size);
    return false;
  }
  
  // Fill HIGH samples
  for(size_t i = 0; i < high_samples; i++){
    target_buffer[i] = high_value;
  }
  
  // Fill LOW samples
  for(size_t i = high_samples; i < high_samples + low_samples; i++){
    target_buffer[i] = low_value;
  }
  
  // Repeat pattern if needed
  size_t pattern_size = high_samples + low_samples;
  if(pattern_size < buffer_size){
    for(size_t i = pattern_size; i < buffer_size; i++){
      target_buffer[i] = target_buffer[i % pattern_size];
    }
  }
  
  ESP_LOGI(PARALLEL_BUFFER_TAG, "Filled buffer %d with pattern: %d HIGH (0x%04X), %d LOW (0x%04X)", 
           buffer_index, high_samples, high_value, low_samples, low_value);
  
  return true;
}

bool ParallelBuffer::isAllocated() const {
  return (buffers && num_buffers > 0) || buffer != nullptr;
}

bool ParallelBuffer::setExternalBuffer(size_t buffer_index, uint16_t* external_buffer, size_t size){
  if(!external_buffer || size == 0){
    ESP_LOGE(PARALLEL_BUFFER_TAG, "Invalid external buffer pointer or size");
    return false;
  }
  
  if(buffer_index >= num_buffers){
    ESP_LOGE(PARALLEL_BUFFER_TAG, "Buffer index %d out of range (max %d)", buffer_index, num_buffers - 1);
    return false;
  }
  
  // Free our own buffer if we own it
  if(buffers[buffer_index] && owns_buffer[buffer_index]){
    heap_caps_free(buffers[buffer_index]);
  }
  
  buffers[buffer_index] = external_buffer;
  buffer_size = size;
  owns_buffer[buffer_index] = false;
  
  ESP_LOGI(PARALLEL_BUFFER_TAG, "Set external buffer %d: %d samples (zero-copy mode)", buffer_index, size);
  return true;
}

} // namespace arcos::abstraction

