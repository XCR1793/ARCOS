/**
 * @file fast_trig.hpp
 * @brief Fast trigonometric functions using lookup tables
 * @author ARCOS Team
 * @date 2025-10-18
 * 
 * This module provides fast trigonometric functions (sin, cos, tan, sinh, cosh, tanh)
 * and their inverse functions (asin, acos, atan, asinh, acosh, atanh) using 
 * pre-computed lookup tables. The precision can be configured at initialization.
 */

#pragma once

#include <cmath>
#include <memory>
#include "lerp.hpp"

namespace arcos::core::maths{

/**
 * @brief Fast trigonometric functions using lookup tables
 * 
 * This class provides fast approximations of trigonometric functions by using
 * pre-computed lookup tables. The precision is determined by the table size
 * specified during initialization.
 */
class FastTrig{
public:
  /**
   * @brief Precision levels for lookup tables based on angular resolution
   */
  enum class Precision{
    DEG_10 = 36,      // ~10 degree precision (360°/36 = 10°)
    DEG_1 = 360,      // ~1 degree precision (360°/360 = 1°)
    DEG_0_1 = 3600,   // ~0.1 degree precision (360°/3600 = 0.1°)
    DEG_0_01 = 36000, // ~0.01 degree precision (360°/36000 = 0.01°)
    DEG_0_001 = 360000 // ~0.001 degree precision (360°/360000 = 0.001°)
  };

  /**
   * @brief Function types for selective initialization
   */
  enum class FunctionType{
    SIN = 1 << 0,     // 0x01
    COS = 1 << 1,     // 0x02
    TAN = 1 << 2,     // 0x04
    SINH = 1 << 3,    // 0x08
    COSH = 1 << 4,    // 0x10
    TANH = 1 << 5,    // 0x20
    ASIN = 1 << 6,    // 0x40
    ACOS = 1 << 7,    // 0x80
    ATAN = 1 << 8,    // 0x100
    ASINH = 1 << 9,   // 0x200
    ACOSH = 1 << 10,  // 0x400
    ATANH = 1 << 11   // 0x800
  };

  // Helper constants for common function groups
  static constexpr uint32_t BASIC_TRIG = static_cast<uint32_t>(FunctionType::SIN) | 
                                         static_cast<uint32_t>(FunctionType::COS) | 
                                         static_cast<uint32_t>(FunctionType::TAN);
  
  static constexpr uint32_t HYPERBOLIC = static_cast<uint32_t>(FunctionType::SINH) | 
                                         static_cast<uint32_t>(FunctionType::COSH) | 
                                         static_cast<uint32_t>(FunctionType::TANH);
  
  static constexpr uint32_t INVERSE_TRIG = static_cast<uint32_t>(FunctionType::ASIN) | 
                                           static_cast<uint32_t>(FunctionType::ACOS) | 
                                           static_cast<uint32_t>(FunctionType::ATAN);
  
  static constexpr uint32_t INVERSE_HYPERBOLIC = static_cast<uint32_t>(FunctionType::ASINH) | 
                                                 static_cast<uint32_t>(FunctionType::ACOSH) | 
                                                 static_cast<uint32_t>(FunctionType::ATANH);
  
  static constexpr uint32_t ALL_FUNCTIONS = BASIC_TRIG | HYPERBOLIC | INVERSE_TRIG | INVERSE_HYPERBOLIC;

  /**
   * @brief Constructor - initializes lookup tables with specified precision and functions
   * @param precision The precision level for the lookup tables
   * @param functions Bitfield of functions to initialize (default: basic trig functions)
   */
  explicit FastTrig(Precision precision = Precision::DEG_0_1, 
                   uint32_t functions = BASIC_TRIG);

  /**
   * @brief Destructor
   */
  ~FastTrig() = default;

  // Disable copy constructor and assignment operator
  FastTrig(const FastTrig&) = delete;
  FastTrig& operator=(const FastTrig&) = delete;

  // Enable move constructor and assignment operator
  FastTrig(FastTrig&&) = default;
  FastTrig& operator=(FastTrig&&) = default;

  /**
   * @brief Fast sine function
   * @param angle Angle in radians
   * @return Sine of the angle
   */
  float sin(float angle) const;

  /**
   * @brief Fast cosine function
   * @param angle Angle in radians
   * @return Cosine of the angle
   */
  float cos(float angle) const;

  /**
   * @brief Fast tangent function
   * @param angle Angle in radians
   * @return Tangent of the angle
   */
  float tan(float angle) const;

  /**
   * @brief Fast hyperbolic sine function
   * @param x Input value
   * @return Hyperbolic sine of x
   */
  float sinh(float x) const;

  /**
   * @brief Fast hyperbolic cosine function
   * @param x Input value
   * @return Hyperbolic cosine of x
   */
  float cosh(float x) const;

  /**
   * @brief Fast hyperbolic tangent function
   * @param x Input value
   * @return Hyperbolic tangent of x
   */
  float tanh(float x) const;

  /**
   * @brief Fast inverse sine function (arcsine)
   * @param x Input value in range [-1, 1]
   * @return Arcsine of x in range [-π/2, π/2]
   */
  float asin(float x) const;

  /**
   * @brief Fast inverse cosine function (arccosine)
   * @param x Input value in range [-1, 1]
   * @return Arccosine of x in range [0, π]
   */
  float acos(float x) const;

  /**
   * @brief Fast inverse tangent function (arctangent)
   * @param x Input value
   * @return Arctangent of x in range [-π/2, π/2]
   */
  float atan(float x) const;

  /**
   * @brief Fast inverse hyperbolic sine function
   * @param x Input value
   * @return Inverse hyperbolic sine of x
   */
  float asinh(float x) const;

  /**
   * @brief Fast inverse hyperbolic cosine function
   * @param x Input value >= 1
   * @return Inverse hyperbolic cosine of x
   */
  float acosh(float x) const;

  /**
   * @brief Fast inverse hyperbolic tangent function
   * @param x Input value in range (-1, 1)
   * @return Inverse hyperbolic tangent of x
   */
  float atanh(float x) const;

  /**
   * @brief Get the current precision level
   * @return Current precision level
   */
  Precision getPrecision() const;

  /**
   * @brief Get the table size for current precision
   * @return Number of entries in lookup tables
   */
  size_t getTableSize() const;

  /**
   * @brief Get the actual angular precision in degrees
   * @return Angular precision in degrees
   */
  float getAngularPrecisionDegrees() const;

  /**
   * @brief Check if a specific function is initialized
   * @param function The function type to check
   * @return True if the function is initialized
   */
  bool isFunctionInitialized(FunctionType function) const;

  /**
   * @brief Get the currently initialized functions as a bitfield
   * @return Bitfield of initialized functions
   */
  uint32_t getInitializedFunctions() const;

private:
  /**
   * @brief Initialize all lookup tables
   */
  void initializeTables();

  /**
   * @brief Initialize a specific function's lookup table
   * @param function The function to initialize
   */
  void initializeFunction(FunctionType function);

  /**
   * @brief Check if function is available and throw if not
   * @param function The function to check
   */
  void ensureFunctionAvailable(FunctionType function) const;

  /**
   * @brief Normalize angle to [0, 2*PI) range
   * @param angle Input angle in radians
   * @return Normalized angle
   */
  float normalizeAngle(float angle) const;

  /**
   * @brief Clamp hyperbolic input to safe range
   * @param x Input value
   * @return Clamped value
   */
  float clampHyperbolic(float x) const;

  /**
   * @brief Linear interpolation between two values
   * @param a First value
   * @param b Second value
   * @param t Interpolation factor [0, 1]
   * @return Interpolated value
   */
  float lerp(float a, float b, float t) const;

  /**
   * @brief Clamp value to range [min, max]
   * @param value Input value
   * @param min_val Minimum value
   * @param max_val Maximum value
   * @return Clamped value
   */
  float clamp(float value, float min_val, float max_val) const;

  Precision precision_;
  size_t table_size_;
  uint32_t enabled_functions_;
  
  // Lookup tables
  std::unique_ptr<float[]> sin_table_;
  std::unique_ptr<float[]> cos_table_;
  std::unique_ptr<float[]> tan_table_;
  std::unique_ptr<float[]> sinh_table_;
  std::unique_ptr<float[]> cosh_table_;
  std::unique_ptr<float[]> tanh_table_;
  
  // Inverse function lookup tables
  std::unique_ptr<float[]> asin_table_;
  std::unique_ptr<float[]> acos_table_;
  std::unique_ptr<float[]> atan_table_;
  std::unique_ptr<float[]> asinh_table_;
  std::unique_ptr<float[]> acosh_table_;
  std::unique_ptr<float[]> atanh_table_;

  // Constants
  static constexpr float PI = 3.14159265358979323846f;
  static constexpr float TWO_PI = 2.0f * PI;
  static constexpr float HALF_PI = 0.5f * PI;
  static constexpr float HYPERBOLIC_MAX = 10.0f;  // Clamp range for hyperbolic functions
  static constexpr float ATAN_MAX = 10.0f;        // Range for atan lookup table
  static constexpr float ASINH_MAX = 10.0f;       // Range for asinh lookup table
  static constexpr float ACOSH_MAX = 10.0f;       // Range for acosh lookup table (>= 1)
};

} // namespace arcos::core::maths

// Include implementation
#include "fast_trig_impl.hpp"