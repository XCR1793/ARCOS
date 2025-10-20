/**
 * @file fast_trig_impl.hpp
 * @brief Implementation of fast trigonometric functions using lookup tables
 * @author ARCOS Team
 * @date 2025-10-18
 */

#pragma once

#include "fast_trig.hpp"
#include <cmath>
#include <algorithm>

namespace arcos::core::maths{

inline FastTrig::FastTrig(Precision precision, uint32_t functions) 
  : precision_(precision), 
    table_size_(static_cast<size_t>(precision)),
    enabled_functions_(functions){
  initializeTables();
}

inline void FastTrig::initializeTables(){
  // Initialize only the requested functions
  if(enabled_functions_ & static_cast<uint32_t>(FunctionType::SIN)){
    initializeFunction(FunctionType::SIN);
  }
  if(enabled_functions_ & static_cast<uint32_t>(FunctionType::COS)){
    initializeFunction(FunctionType::COS);
  }
  if(enabled_functions_ & static_cast<uint32_t>(FunctionType::TAN)){
    initializeFunction(FunctionType::TAN);
  }
  if(enabled_functions_ & static_cast<uint32_t>(FunctionType::SINH)){
    initializeFunction(FunctionType::SINH);
  }
  if(enabled_functions_ & static_cast<uint32_t>(FunctionType::COSH)){
    initializeFunction(FunctionType::COSH);
  }
  if(enabled_functions_ & static_cast<uint32_t>(FunctionType::TANH)){
    initializeFunction(FunctionType::TANH);
  }
  if(enabled_functions_ & static_cast<uint32_t>(FunctionType::ASIN)){
    initializeFunction(FunctionType::ASIN);
  }
  if(enabled_functions_ & static_cast<uint32_t>(FunctionType::ACOS)){
    initializeFunction(FunctionType::ACOS);
  }
  if(enabled_functions_ & static_cast<uint32_t>(FunctionType::ATAN)){
    initializeFunction(FunctionType::ATAN);
  }
  if(enabled_functions_ & static_cast<uint32_t>(FunctionType::ASINH)){
    initializeFunction(FunctionType::ASINH);
  }
  if(enabled_functions_ & static_cast<uint32_t>(FunctionType::ACOSH)){
    initializeFunction(FunctionType::ACOSH);
  }
  if(enabled_functions_ & static_cast<uint32_t>(FunctionType::ATANH)){
    initializeFunction(FunctionType::ATANH);
  }
}

inline void FastTrig::initializeFunction(FunctionType function){
  switch(function){
    case FunctionType::SIN:
      if(!sin_table_){
        sin_table_ = std::make_unique<float[]>(table_size_);
        const float angle_step = TWO_PI / static_cast<float>(table_size_);
        for(size_t i = 0; i < table_size_; ++i){
          const float angle = static_cast<float>(i) * angle_step;
          sin_table_[i] = std::sin(angle);
        }
      }
      break;
      
    case FunctionType::COS:
      if(!cos_table_){
        cos_table_ = std::make_unique<float[]>(table_size_);
        const float angle_step = TWO_PI / static_cast<float>(table_size_);
        for(size_t i = 0; i < table_size_; ++i){
          const float angle = static_cast<float>(i) * angle_step;
          cos_table_[i] = std::cos(angle);
        }
      }
      break;
      
    case FunctionType::TAN:
      if(!tan_table_){
        // Ensure sin and cos are available for tan calculation
        if(!sin_table_) initializeFunction(FunctionType::SIN);
        if(!cos_table_) initializeFunction(FunctionType::COS);
        
        tan_table_ = std::make_unique<float[]>(table_size_);
        for(size_t i = 0; i < table_size_; ++i){
          const float cos_val = cos_table_[i];
          if(std::abs(cos_val) > 1e-6f){
            tan_table_[i] = sin_table_[i] / cos_val;
          }else{
            tan_table_[i] = (sin_table_[i] >= 0.0f) ? 1e6f : -1e6f;
          }
        }
      }
      break;
      
    case FunctionType::SINH:
      if(!sinh_table_){
        sinh_table_ = std::make_unique<float[]>(table_size_);
        const float hyperbolic_step = (2.0f * HYPERBOLIC_MAX) / static_cast<float>(table_size_);
        for(size_t i = 0; i < table_size_; ++i){
          const float x = -HYPERBOLIC_MAX + static_cast<float>(i) * hyperbolic_step;
          sinh_table_[i] = std::sinh(x);
        }
      }
      break;
      
    case FunctionType::COSH:
      if(!cosh_table_){
        cosh_table_ = std::make_unique<float[]>(table_size_);
        const float hyperbolic_step = (2.0f * HYPERBOLIC_MAX) / static_cast<float>(table_size_);
        for(size_t i = 0; i < table_size_; ++i){
          const float x = -HYPERBOLIC_MAX + static_cast<float>(i) * hyperbolic_step;
          cosh_table_[i] = std::cosh(x);
        }
      }
      break;
      
    case FunctionType::TANH:
      if(!tanh_table_){
        tanh_table_ = std::make_unique<float[]>(table_size_);
        const float hyperbolic_step = (2.0f * HYPERBOLIC_MAX) / static_cast<float>(table_size_);
        for(size_t i = 0; i < table_size_; ++i){
          const float x = -HYPERBOLIC_MAX + static_cast<float>(i) * hyperbolic_step;
          tanh_table_[i] = std::tanh(x);
        }
      }
      break;
      
    case FunctionType::ASIN:
      if(!asin_table_){
        asin_table_ = std::make_unique<float[]>(table_size_);
        const float trig_inv_step = 2.0f / static_cast<float>(table_size_);
        for(size_t i = 0; i < table_size_; ++i){
          const float x = -1.0f + static_cast<float>(i) * trig_inv_step;
          asin_table_[i] = std::asin(clamp(x, -1.0f, 1.0f));
        }
      }
      break;
      
    case FunctionType::ACOS:
      if(!acos_table_){
        acos_table_ = std::make_unique<float[]>(table_size_);
        const float trig_inv_step = 2.0f / static_cast<float>(table_size_);
        for(size_t i = 0; i < table_size_; ++i){
          const float x = -1.0f + static_cast<float>(i) * trig_inv_step;
          acos_table_[i] = std::acos(clamp(x, -1.0f, 1.0f));
        }
      }
      break;
      
    case FunctionType::ATAN:
      if(!atan_table_){
        atan_table_ = std::make_unique<float[]>(table_size_);
        const float atan_step = (2.0f * ATAN_MAX) / static_cast<float>(table_size_);
        for(size_t i = 0; i < table_size_; ++i){
          const float x = -ATAN_MAX + static_cast<float>(i) * atan_step;
          atan_table_[i] = std::atan(x);
        }
      }
      break;
      
    case FunctionType::ASINH:
      if(!asinh_table_){
        asinh_table_ = std::make_unique<float[]>(table_size_);
        const float asinh_step = (2.0f * ASINH_MAX) / static_cast<float>(table_size_);
        for(size_t i = 0; i < table_size_; ++i){
          const float x = -ASINH_MAX + static_cast<float>(i) * asinh_step;
          asinh_table_[i] = std::asinh(x);
        }
      }
      break;
      
    case FunctionType::ACOSH:
      if(!acosh_table_){
        acosh_table_ = std::make_unique<float[]>(table_size_);
        const float acosh_step = (ACOSH_MAX - 1.0f) / static_cast<float>(table_size_);
        for(size_t i = 0; i < table_size_; ++i){
          const float x = 1.0f + static_cast<float>(i) * acosh_step;
          acosh_table_[i] = std::acosh(x);
        }
      }
      break;
      
    case FunctionType::ATANH:
      if(!atanh_table_){
        atanh_table_ = std::make_unique<float[]>(table_size_);
        const float atanh_range = 0.99999f;
        const float atanh_step = (2.0f * atanh_range) / static_cast<float>(table_size_);
        for(size_t i = 0; i < table_size_; ++i){
          const float x = -atanh_range + static_cast<float>(i) * atanh_step;
          atanh_table_[i] = std::atanh(x);
        }
      }
      break;
  }
}

inline float FastTrig::sin(float angle) const{
  // Fast path - skip validation in release builds
  #ifdef _DEBUG
  ensureFunctionAvailable(FunctionType::SIN);
  #endif
  
  // Fast angle normalization using fmod
  const float normalized = std::fmod(angle, TWO_PI);
  const float positive_angle = (normalized < 0.0f) ? normalized + TWO_PI : normalized;
  
  // Optimize for direct table lookup when possible
  const float scale = static_cast<float>(table_size_) / TWO_PI;
  const float index_float = positive_angle * scale;
  const size_t index = static_cast<size_t>(index_float);
  
  // Simple linear interpolation
  if(index >= table_size_ - 1) {
    return sin_table_[0]; // Wrap around
  }
  
  const float frac = index_float - static_cast<float>(index);
  const float val1 = sin_table_[index];
  const float val2 = sin_table_[index + 1];
  return val1 + frac * (val2 - val1);
}

inline float FastTrig::cos(float angle) const{
  #ifdef _DEBUG
  ensureFunctionAvailable(FunctionType::COS);
  #endif
  
  // Fast angle normalization
  const float normalized = std::fmod(angle, TWO_PI);
  const float positive_angle = (normalized < 0.0f) ? normalized + TWO_PI : normalized;
  
  // Direct table lookup with interpolation
  const float scale = static_cast<float>(table_size_) / TWO_PI;
  const float index_float = positive_angle * scale;
  const size_t index = static_cast<size_t>(index_float);
  
  if(index >= table_size_ - 1) {
    return cos_table_[0];
  }
  
  const float frac = index_float - static_cast<float>(index);
  const float val1 = cos_table_[index];
  const float val2 = cos_table_[index + 1];
  return val1 + frac * (val2 - val1);
}

inline float FastTrig::tan(float angle) const{
  ensureFunctionAvailable(FunctionType::TAN);
  
  // Normalize angle to [0, 2*PI)
  const float normalized = normalizeAngle(angle);
  
  // Convert to table index
  const float index_float = (normalized / TWO_PI) * static_cast<float>(table_size_);
  const size_t index = static_cast<size_t>(index_float);
  const float frac = index_float - static_cast<float>(index);
  
  // Linear interpolation between adjacent values
  const size_t next_index = (index + 1) % table_size_;
  return arcos::core::maths::lerp(tan_table_[index], tan_table_[next_index], frac);
}

inline float FastTrig::sinh(float x) const{
  ensureFunctionAvailable(FunctionType::SINH);
  
  // Clamp input to safe range
  const float clamped = clampHyperbolic(x);
  
  // Convert to table index
  const float index_float = ((clamped + HYPERBOLIC_MAX) / (2.0f * HYPERBOLIC_MAX)) * static_cast<float>(table_size_);
  const size_t index = std::min(static_cast<size_t>(index_float), table_size_ - 1);
  const float frac = index_float - static_cast<float>(index);
  
  // Linear interpolation between adjacent values
  const size_t next_index = std::min(index + 1, table_size_ - 1);
  return arcos::core::maths::lerp(sinh_table_[index], sinh_table_[next_index], frac);
}

inline float FastTrig::cosh(float x) const{
  ensureFunctionAvailable(FunctionType::COSH);
  
  // Clamp input to safe range
  const float clamped = clampHyperbolic(x);
  
  // Convert to table index
  const float index_float = ((clamped + HYPERBOLIC_MAX) / (2.0f * HYPERBOLIC_MAX)) * static_cast<float>(table_size_);
  const size_t index = std::min(static_cast<size_t>(index_float), table_size_ - 1);
  const float frac = index_float - static_cast<float>(index);
  
  // Linear interpolation between adjacent values
  const size_t next_index = std::min(index + 1, table_size_ - 1);
  return arcos::core::maths::lerp(cosh_table_[index], cosh_table_[next_index], frac);
}

inline float FastTrig::tanh(float x) const{
  ensureFunctionAvailable(FunctionType::TANH);
  
  // Clamp input to safe range
  const float clamped = clampHyperbolic(x);
  
  // Convert to table index
  const float index_float = ((clamped + HYPERBOLIC_MAX) / (2.0f * HYPERBOLIC_MAX)) * static_cast<float>(table_size_);
  const size_t index = std::min(static_cast<size_t>(index_float), table_size_ - 1);
  const float frac = index_float - static_cast<float>(index);
  
  // Linear interpolation between adjacent values
  const size_t next_index = std::min(index + 1, table_size_ - 1);
  return arcos::core::maths::lerp(tanh_table_[index], tanh_table_[next_index], frac);
}

inline float FastTrig::asin(float x) const{
  #ifdef _DEBUG
  ensureFunctionAvailable(FunctionType::ASIN);
  #endif
  
  // Handle exact boundary values
  if(x >= 1.0f) return static_cast<float>(M_PI) / 2.0f;
  if(x <= -1.0f) return -static_cast<float>(M_PI) / 2.0f;
  
  // Clamp input to valid range [-1, 1]
  const float clamped = clamp(x, -1.0f, 1.0f);
  
  // Convert to table index with proper scaling
  const float normalized = (clamped + 1.0f) / 2.0f; // [0, 1]
  const float index_float = normalized * static_cast<float>(table_size_ - 1);
  const size_t index = std::min(static_cast<size_t>(index_float), table_size_ - 1);
  
  if(index >= table_size_ - 1) {
    return asin_table_[table_size_ - 1];
  }
  
  const float frac = index_float - static_cast<float>(index);
  const float val1 = asin_table_[index];
  const float val2 = asin_table_[index + 1];
  return val1 + frac * (val2 - val1);
}

inline float FastTrig::acos(float x) const{
  #ifdef _DEBUG
  ensureFunctionAvailable(FunctionType::ACOS);
  #endif
  
  // Handle exact boundary values
  if(x >= 1.0f) return 0.0f;
  if(x <= -1.0f) return static_cast<float>(M_PI);
  
  // Clamp input to valid range [-1, 1]
  const float clamped = clamp(x, -1.0f, 1.0f);
  
  // Convert to table index with proper scaling
  const float normalized = (clamped + 1.0f) / 2.0f; // [0, 1]
  const float index_float = normalized * static_cast<float>(table_size_ - 1);
  const size_t index = std::min(static_cast<size_t>(index_float), table_size_ - 1);
  
  if(index >= table_size_ - 1) {
    return acos_table_[table_size_ - 1];
  }
  
  const float frac = index_float - static_cast<float>(index);
  const float val1 = acos_table_[index];
  const float val2 = acos_table_[index + 1];
  return val1 + frac * (val2 - val1);
}

inline float FastTrig::atan(float x) const{
  ensureFunctionAvailable(FunctionType::ATAN);
  
  // Clamp input to lookup table range
  const float clamped = clamp(x, -ATAN_MAX, ATAN_MAX);
  
  // Convert to table index
  const float index_float = ((clamped + ATAN_MAX) / (2.0f * ATAN_MAX)) * static_cast<float>(table_size_);
  const size_t index = std::min(static_cast<size_t>(index_float), table_size_ - 1);
  const float frac = index_float - static_cast<float>(index);
  
  // Linear interpolation between adjacent values
  const size_t next_index = std::min(index + 1, table_size_ - 1);
  return arcos::core::maths::lerp(atan_table_[index], atan_table_[next_index], frac);
}

inline float FastTrig::asinh(float x) const{
  ensureFunctionAvailable(FunctionType::ASINH);
  
  // Clamp input to lookup table range
  const float clamped = clamp(x, -ASINH_MAX, ASINH_MAX);
  
  // Convert to table index
  const float index_float = ((clamped + ASINH_MAX) / (2.0f * ASINH_MAX)) * static_cast<float>(table_size_);
  const size_t index = std::min(static_cast<size_t>(index_float), table_size_ - 1);
  const float frac = index_float - static_cast<float>(index);
  
  // Linear interpolation between adjacent values
  const size_t next_index = std::min(index + 1, table_size_ - 1);
  return arcos::core::maths::lerp(asinh_table_[index], asinh_table_[next_index], frac);
}

inline float FastTrig::acosh(float x) const{
  ensureFunctionAvailable(FunctionType::ACOSH);
  
  // Clamp input to valid range [1, ACOSH_MAX]
  const float clamped = clamp(x, 1.0f, ACOSH_MAX);
  
  // Convert to table index
  const float index_float = ((clamped - 1.0f) / (ACOSH_MAX - 1.0f)) * static_cast<float>(table_size_);
  const size_t index = std::min(static_cast<size_t>(index_float), table_size_ - 1);
  const float frac = index_float - static_cast<float>(index);
  
  // Linear interpolation between adjacent values
  const size_t next_index = std::min(index + 1, table_size_ - 1);
  return arcos::core::maths::lerp(acosh_table_[index], acosh_table_[next_index], frac);
}

inline float FastTrig::atanh(float x) const{
  ensureFunctionAvailable(FunctionType::ATANH);
  
  // Clamp input to valid range (-1, 1) with small margin
  const float atanh_range = 0.99999f;
  const float clamped = clamp(x, -atanh_range, atanh_range);
  
  // Convert to table index
  const float index_float = ((clamped + atanh_range) / (2.0f * atanh_range)) * static_cast<float>(table_size_);
  const size_t index = std::min(static_cast<size_t>(index_float), table_size_ - 1);
  const float frac = index_float - static_cast<float>(index);
  
  // Linear interpolation between adjacent values
  const size_t next_index = std::min(index + 1, table_size_ - 1);
  return arcos::core::maths::lerp(atanh_table_[index], atanh_table_[next_index], frac);
}

inline FastTrig::Precision FastTrig::getPrecision() const{
  return precision_;
}

inline size_t FastTrig::getTableSize() const{
  return table_size_;
}

inline float FastTrig::getAngularPrecisionDegrees() const{
  return 360.0f / static_cast<float>(table_size_);
}

inline float FastTrig::normalizeAngle(float angle) const{
  // Normalize angle to [0, 2*PI) range
  float normalized = std::fmod(angle, TWO_PI);
  if(normalized < 0.0f){
    normalized += TWO_PI;
  }
  return normalized;
}

inline float FastTrig::clampHyperbolic(float x) const{
  return clamp(x, -HYPERBOLIC_MAX, HYPERBOLIC_MAX);
}

inline float FastTrig::lerp(float a, float b, float t) const{
  return arcos::core::maths::lerp(a, b, t);
}

inline float FastTrig::clamp(float value, float min_val, float max_val) const{
  return (value < min_val) ? min_val : (value > max_val) ? max_val : value;
}

inline bool FastTrig::isFunctionInitialized(FunctionType function) const{
  return (enabled_functions_ & static_cast<uint32_t>(function)) != 0;
}

inline uint32_t FastTrig::getInitializedFunctions() const{
  return enabled_functions_;
}

inline void FastTrig::ensureFunctionAvailable(FunctionType function) const{
  if(!isFunctionInitialized(function)){
    // For now, we'll just return silently. In a full implementation,
    // you might want to throw an exception or initialize on-demand
    return;
  }
}

} // namespace arcos::core::maths