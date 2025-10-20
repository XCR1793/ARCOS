/**
 * @file selective_trig_example.cpp
 * @brief Example of selective initialization of trigonometric functions
 * @author ARCOS Team
 * @date 2025-10-18
 */

#include "core/maths.hpp"
#include <iostream>
#include <iomanip>

using namespace arcos::core::maths;

int main(){
  std::cout << "ARCOS Selective Fast Trigonometric Functions Example\n";
  std::cout << "====================================================\n\n";

  // Example 1: Initialize only basic trigonometric functions
  std::cout << "1. Basic Trigonometric Functions Only:\n";
  FastTrig basic_trig(FastTrig::Precision::DEG_0_1, FastTrig::BASIC_TRIG);
  
  std::cout << "   Initialized functions: " << std::hex << basic_trig.getInitializedFunctions() << std::dec << "\n";
  std::cout << "   Memory usage: ~" << (basic_trig.getTableSize() * 3 * 4) / 1024 << " KB\n";
  
  float angle = 1.5708f; // π/2
  std::cout << "   sin(π/2) = " << basic_trig.sin(angle) << "\n";
  std::cout << "   cos(π/2) = " << basic_trig.cos(angle) << "\n";
  std::cout << "   tan(π/2) = " << basic_trig.tan(angle) << "\n\n";

  // Example 2: Initialize only hyperbolic functions
  std::cout << "2. Hyperbolic Functions Only:\n";
  FastTrig hyperbolic_trig(FastTrig::Precision::DEG_0_1, FastTrig::HYPERBOLIC);
  
  std::cout << "   Initialized functions: " << std::hex << hyperbolic_trig.getInitializedFunctions() << std::dec << "\n";
  std::cout << "   Memory usage: ~" << (hyperbolic_trig.getTableSize() * 3 * 4) / 1024 << " KB\n";
  
  float x = 1.0f;
  std::cout << "   sinh(1.0) = " << hyperbolic_trig.sinh(x) << "\n";
  std::cout << "   cosh(1.0) = " << hyperbolic_trig.cosh(x) << "\n";
  std::cout << "   tanh(1.0) = " << hyperbolic_trig.tanh(x) << "\n\n";

  // Example 3: Initialize only specific functions
  std::cout << "3. Custom Selection (sin + cos + asin):\n";
  uint32_t custom_functions = static_cast<uint32_t>(FastTrig::FunctionType::SIN) |
                              static_cast<uint32_t>(FastTrig::FunctionType::COS) |
                              static_cast<uint32_t>(FastTrig::FunctionType::ASIN);
  
  FastTrig custom_trig(FastTrig::Precision::DEG_0_1, custom_functions);
  
  std::cout << "   Initialized functions: " << std::hex << custom_trig.getInitializedFunctions() << std::dec << "\n";
  std::cout << "   Memory usage: ~" << (custom_trig.getTableSize() * 3 * 4) / 1024 << " KB\n";
  
  std::cout << "   sin(π/4) = " << custom_trig.sin(0.7854f) << "\n";
  std::cout << "   cos(π/4) = " << custom_trig.cos(0.7854f) << "\n";
  std::cout << "   asin(0.5) = " << custom_trig.asin(0.5f) << "\n\n";

  // Example 4: All functions for comparison
  std::cout << "4. All Functions:\n";
  FastTrig all_trig(FastTrig::Precision::DEG_0_1, FastTrig::ALL_FUNCTIONS);
  
  std::cout << "   Initialized functions: " << std::hex << all_trig.getInitializedFunctions() << std::dec << "\n";
  std::cout << "   Memory usage: ~" << (all_trig.getTableSize() * 12 * 4) / 1024 << " KB\n\n";

  // Example 5: Using convenience functions with selective initialization
  std::cout << "5. Global Convenience Functions:\n";
  
  // Initialize global functions with only what we need
  initializeFastTrig(FastTrig::Precision::DEG_0_1, 
                     FastTrig::BASIC_TRIG | FastTrig::INVERSE_TRIG);
  
  std::cout << "   Using fastSin(π/6) = " << fastSin(0.5236f) << "\n";
  std::cout << "   Using fastCos(π/6) = " << fastCos(0.5236f) << "\n";
  std::cout << "   Using fastAsin(0.5) = " << fastAsin(0.5f) << "\n";
  std::cout << "   Using fastAcos(0.5) = " << fastAcos(0.5f) << "\n\n";

  // Example 6: Function availability checking
  std::cout << "6. Function Availability:\n";
  
  std::cout << "   Basic trig has SIN: " << basic_trig.isFunctionInitialized(FastTrig::FunctionType::SIN) << "\n";
  std::cout << "   Basic trig has SINH: " << basic_trig.isFunctionInitialized(FastTrig::FunctionType::SINH) << "\n";
  std::cout << "   Hyperbolic has SINH: " << hyperbolic_trig.isFunctionInitialized(FastTrig::FunctionType::SINH) << "\n";
  std::cout << "   Hyperbolic has SIN: " << hyperbolic_trig.isFunctionInitialized(FastTrig::FunctionType::SIN) << "\n\n";

  std::cout << "Memory Savings Summary:\n";
  std::cout << "- Basic trig only: ~43 KB (vs ~172 KB for all functions)\n";
  std::cout << "- Hyperbolic only: ~43 KB (vs ~172 KB for all functions)\n";
  std::cout << "- Custom selection: Variable based on selection\n";
  std::cout << "- Choose only what you need for optimal memory usage!\n\n";

  std::cout << "Example completed successfully!\n";
  return 0;
}