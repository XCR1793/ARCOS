/**
 * @file maths.hpp
 * @brief Convenience header for ARCOS mathematics utilities
 * @author ARCOS Team
 * @date 2025-10-18
 * 
 * This header provides easy access to all mathematics utilities in the ARCOS framework.
 */

#pragma once

#include "maths/fast_trig.hpp"

namespace arcos::core::maths{

/**
 * @brief Global fast trigonometric functions instance
 * 
 * This provides a convenient way to access fast trigonometric functions
 * without manually managing the FastTrig instance.
 * 
 * Usage:
 * @code
 * float result = arcos::core::maths::fastSin(angle);
 * @endcode
 */
namespace{
  static FastTrig global_fast_trig(FastTrig::Precision::DEG_0_1);
}

/**
 * @brief Convenience functions for fast trigonometric operations
 */

inline float fastSin(float angle){
  return global_fast_trig.sin(angle);
}

inline float fastCos(float angle){
  return global_fast_trig.cos(angle);
}

inline float fastTan(float angle){
  return global_fast_trig.tan(angle);
}

inline float fastSinh(float x){
  return global_fast_trig.sinh(x);
}

inline float fastCosh(float x){
  return global_fast_trig.cosh(x);
}

inline float fastTanh(float x){
  return global_fast_trig.tanh(x);
}

inline float fastAsin(float x){
  return global_fast_trig.asin(x);
}

inline float fastAcos(float x){
  return global_fast_trig.acos(x);
}

inline float fastAtan(float x){
  return global_fast_trig.atan(x);
}

inline float fastAsinh(float x){
  return global_fast_trig.asinh(x);
}

inline float fastAcosh(float x){
  return global_fast_trig.acosh(x);
}

inline float fastAtanh(float x){
  return global_fast_trig.atanh(x);
}

/**
 * @brief Initialize global fast trigonometric functions with custom precision and functions
 * @param precision The precision level for lookup tables
 * @param functions Bitfield of functions to initialize (default: basic trig functions)
 * 
 * Call this function once at startup to configure the precision of the
 * global fast trigonometric functions.
 */
inline void initializeFastTrig(FastTrig::Precision precision, 
                              uint32_t functions = FastTrig::BASIC_TRIG){
  global_fast_trig = FastTrig(precision, functions);
}

} // namespace arcos::core::maths