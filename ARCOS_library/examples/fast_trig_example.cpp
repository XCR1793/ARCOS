/**
 * @file fast_trig_example.cpp
 * @brief Example usage of ARCOS fast trigonometric functions
 * @author ARCOS Team
 * @date 2025-10-18
 */

#include "core/maths.hpp"
#include <iostream>
#include <iomanip>

using namespace arcos::core::maths;

int main(){
  std::cout << "ARCOS Fast Trigonometric Functions Example\n";
  std::cout << "==========================================\n\n";

  // Initialize the global fast trigonometric functions with high precision
  initializeFastTrig(FastTrig::Precision::DEG_0_01);

  // Example 1: Basic trigonometric functions
  std::cout << "1. Basic Trigonometric Functions:\n";
  std::cout << std::fixed << std::setprecision(6);
  
  float angles[] = {0.0f, 0.7854f, 1.5708f, 3.1416f, 4.7124f, 6.2832f}; // 0°, 45°, 90°, 180°, 270°, 360°
  const char* angle_names[] = {"0°", "45°", "90°", "180°", "270°", "360°"};
  
  for(size_t i = 0; i < 6; ++i){
    float angle = angles[i];
    std::cout << "  " << angle_names[i] << " (" << angle << " rad):\n";
    std::cout << "    sin = " << fastSin(angle) << "\n";
    std::cout << "    cos = " << fastCos(angle) << "\n";
    std::cout << "    tan = " << fastTan(angle) << "\n\n";
  }

  // Example 2: Hyperbolic functions
  std::cout << "2. Hyperbolic Functions:\n";
  float values[] = {-2.0f, -1.0f, 0.0f, 1.0f, 2.0f};
  
  for(size_t i = 0; i < 5; ++i){
    float x = values[i];
    std::cout << "  x = " << x << ":\n";
    std::cout << "    sinh = " << fastSinh(x) << "\n";
    std::cout << "    cosh = " << fastCosh(x) << "\n";
    std::cout << "    tanh = " << fastTanh(x) << "\n\n";
  }

  // Example 3: Inverse trigonometric functions
  std::cout << "3. Inverse Trigonometric Functions:\n";
  float inv_values[] = {-0.5f, 0.0f, 0.5f, 0.866f}; // Notable values
  
  for(size_t i = 0; i < 4; ++i){
    float x = inv_values[i];
    std::cout << "  x = " << x << ":\n";
    std::cout << "    asin = " << fastAsin(x) << " rad\n";
    std::cout << "    acos = " << fastAcos(x) << " rad\n";
    std::cout << "    atan = " << fastAtan(x) << " rad\n\n";
  }

  // Example 4: Inverse hyperbolic functions
  std::cout << "4. Inverse Hyperbolic Functions:\n";
  float inv_hyp_values[] = {-1.0f, 0.0f, 1.0f, 2.0f};
  
  for(size_t i = 0; i < 4; ++i){
    float x = inv_hyp_values[i];
    std::cout << "  x = " << x << ":\n";
    std::cout << "    asinh = " << fastAsinh(x) << "\n";
    if(x >= 1.0f){
      std::cout << "    acosh = " << fastAcosh(x) << "\n";
    }
    if(x > -1.0f && x < 1.0f){
      std::cout << "    atanh = " << fastAtanh(x) << "\n";
    }
    std::cout << "\n";
  }

  // Example 5: Using FastTrig class directly for custom precision
  std::cout << "5. Custom Precision Example:\n";
  
  FastTrig low_precision(FastTrig::Precision::DEG_10);
  FastTrig high_precision(FastTrig::Precision::DEG_0_001);
  
  float test_angle = 1.0f; // 1 radian
  
  std::cout << "  For angle = " << test_angle << " rad:\n";
  std::cout << "    Low precision (10°):   " << low_precision.sin(test_angle) << "\n";
  std::cout << "    High precision (0.001°): " << high_precision.sin(test_angle) << "\n";
  std::cout << "    Standard sin:          " << std::sin(test_angle) << "\n\n";

  // Example 6: Performance consideration
  std::cout << "6. Precision Levels Available:\n";
  std::cout << "    DEG_10:   " << static_cast<int>(FastTrig::Precision::DEG_10) << " table entries (~10° precision)\n";
  std::cout << "    DEG_1:    " << static_cast<int>(FastTrig::Precision::DEG_1) << " table entries (~1° precision)\n";
  std::cout << "    DEG_0_1:  " << static_cast<int>(FastTrig::Precision::DEG_0_1) << " table entries (~0.1° precision)\n";
  std::cout << "    DEG_0_01: " << static_cast<int>(FastTrig::Precision::DEG_0_01) << " table entries (~0.01° precision)\n";
  std::cout << "    DEG_0_001:" << static_cast<int>(FastTrig::Precision::DEG_0_001) << " table entries (~0.001° precision)\n\n";

  std::cout << "Usage Tips:\n";
  std::cout << "- Use DEG_10 precision for real-time applications where speed is critical\n";
  std::cout << "- Use DEG_1 precision for general purpose applications\n";
  std::cout << "- Use DEG_0_1 precision for applications requiring good accuracy\n";
  std::cout << "- Use DEG_0_01 precision for high accuracy applications\n";
  std::cout << "- Use DEG_0_001 precision for maximum accuracy (large memory usage)\n\n";

  std::cout << "Example completed successfully!\n";
  return 0;
}