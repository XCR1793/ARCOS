/**
 * @file lerp.hpp
 * @brief Templated linear interpolation utilities
 * @author ARCOS Team
 * @date 2025-10-18
 * 
 * This module provides templated linear interpolation functions that work
 * with various numeric types (float, double, int, etc.).
 */

#pragma once

#include <type_traits>

namespace arcos::core::maths{

/**
 * @brief Linear interpolation between two values
 * @tparam T Numeric type (float, double, int, etc.)
 * @param a First value
 * @param b Second value
 * @param t Interpolation factor [0, 1]
 * @return Interpolated value between a and b
 */
template<typename T>
constexpr T lerp(const T& a, const T& b, const T& t){
  static_assert(std::is_arithmetic_v<T>, "lerp requires arithmetic types");
  return a + t * (b - a);
}

/**
 * @brief Linear interpolation with explicit type conversion
 * @tparam TResult Result type
 * @tparam TValue Value type
 * @tparam TFactor Interpolation factor type
 * @param a First value
 * @param b Second value
 * @param t Interpolation factor [0, 1]
 * @return Interpolated value cast to TResult
 */
template<typename TResult, typename TValue, typename TFactor>
constexpr TResult lerp_cast(const TValue& a, const TValue& b, const TFactor& t){
  static_assert(std::is_arithmetic_v<TValue>, "lerp_cast requires arithmetic value types");
  static_assert(std::is_arithmetic_v<TFactor>, "lerp_cast requires arithmetic factor type");
  static_assert(std::is_arithmetic_v<TResult>, "lerp_cast requires arithmetic result type");
  return static_cast<TResult>(a + t * (b - a));
}

/**
 * @brief Inverse linear interpolation - find t given a, b, and result
 * @tparam T Numeric type
 * @param a First value
 * @param b Second value
 * @param value Value to find t for
 * @return Interpolation factor t such that lerp(a, b, t) ≈ value
 */
template<typename T>
constexpr T inverse_lerp(const T& a, const T& b, const T& value){
  static_assert(std::is_arithmetic_v<T>, "inverse_lerp requires arithmetic types");
  return (value - a) / (b - a);
}

/**
 * @brief Smooth step interpolation (Hermite interpolation)
 * @tparam T Numeric type
 * @param a First value
 * @param b Second value
 * @param t Interpolation factor [0, 1]
 * @return Smoothly interpolated value with zero derivatives at endpoints
 */
template<typename T>
constexpr T smooth_step(const T& a, const T& b, const T& t){
  static_assert(std::is_floating_point_v<T>, "smooth_step requires floating point types");
  const T clamped_t = (t < T(0)) ? T(0) : (t > T(1)) ? T(1) : t;
  const T smooth_t = clamped_t * clamped_t * (T(3) - T(2) * clamped_t);
  return lerp(a, b, smooth_t);
}

/**
 * @brief Smoother step interpolation (5th order Hermite)
 * @tparam T Numeric type
 * @param a First value
 * @param b Second value
 * @param t Interpolation factor [0, 1]
 * @return Very smoothly interpolated value with zero first and second derivatives at endpoints
 */
template<typename T>
constexpr T smoother_step(const T& a, const T& b, const T& t){
  static_assert(std::is_floating_point_v<T>, "smoother_step requires floating point types");
  const T clamped_t = (t < T(0)) ? T(0) : (t > T(1)) ? T(1) : t;
  const T smooth_t = clamped_t * clamped_t * clamped_t * (clamped_t * (clamped_t * T(6) - T(15)) + T(10));
  return lerp(a, b, smooth_t);
}

/**
 * @brief Bilinear interpolation for 2D data
 * @tparam T Numeric type
 * @param v00 Value at (0,0)
 * @param v10 Value at (1,0)
 * @param v01 Value at (0,1)
 * @param v11 Value at (1,1)
 * @param tx Interpolation factor in x direction [0, 1]
 * @param ty Interpolation factor in y direction [0, 1]
 * @return Bilinearly interpolated value
 */
template<typename T>
constexpr T bilerp(const T& v00, const T& v10, const T& v01, const T& v11, const T& tx, const T& ty){
  static_assert(std::is_arithmetic_v<T>, "bilerp requires arithmetic types");
  const T x1 = lerp(v00, v10, tx);
  const T x2 = lerp(v01, v11, tx);
  return lerp(x1, x2, ty);
}

} // namespace arcos::core::maths