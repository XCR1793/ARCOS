/*****************************************************************
 * File:      arcos.hpp
 * Category:  root
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Main header file for the complete ARCOS Hardware Abstraction Framework.
 *    Include this file to access ALL ARCOS library functionality.
 *    
 *    For modular inclusion, use individual module headers:
 *    - arcos_core.hpp      : HAL and platform abstractions
 *    - arcos_algorithms.hpp: Sensor fusion and math algorithms  
 *    - arcos_drivers.hpp   : Hardware component drivers
 *****************************************************************/

#ifndef ARCOS_ARCOS_HPP_
#define ARCOS_ARCOS_HPP_

// Include all ARCOS modules for complete functionality
#include "arcos_core.hpp"       // HAL and platform abstractions
#include "arcos_algorithms.hpp" // Sensor fusion and algorithms
#include "arcos_drivers.hpp"    // Hardware drivers (includes core)
#include "arcos_processing.hpp" // Processing and configuration utilities

/** 
 * @brief ARCOS Hardware Abstraction Framework - Complete Library
 * 
 * This header provides access to the complete ARCOS library functionality:
 * 
 * **Core Module (arcos_core.hpp):**
 * - Hardware Abstraction Layer (HAL) for GPIO, timers, protocols
 * - Platform-specific implementations (ESP32, Arduino, etc.)
 * - Communication interfaces (I2C, SPI, parallel)
 * 
 * **Algorithms Module (arcos_algorithms.hpp):**
 * - Sensor fusion algorithms for IMU data processing
 * - Quaternion mathematics and 3D rotations
 * - Euler angle conversions and gravity compensation
 * 
 * **Drivers Module (arcos_drivers.hpp):**
 * - Hardware component drivers (HUB75, ICM20948, BME280, etc.)
 * - Display and sensor abstractions
 * - Storage and communication drivers
 * 
 * **Processing Module (arcos_processing.hpp):**
 * - OWO configuration file parser (.owo format)
 * - Type-safe configuration management
 * - Firmware variable substitution
 * - Data processing utilities
 * 
 * **Usage Options:**
 * 
 * 1. **Complete Library (this file):**
 * ```cpp
 * #include <arcos.hpp>
 * // Access to everything
 * ```
 * 
 * 2. **Individual Modules:**
 * ```cpp
 * #include <arcos_core.hpp>       // Just HAL and platforms
 * #include <arcos_algorithms.hpp> // Just sensor fusion
 * #include <arcos_drivers.hpp>    // Just drivers (includes core)
 * #include <arcos_processing.hpp> // Just processing utilities
 * ```
 * 
 * 3. **Combination:**
 * ```cpp
 * #include <arcos_core.hpp>
 * #include <arcos_algorithms.hpp>
 * #include <arcos_processing.hpp>
 * // HAL + algorithms + processing, but no drivers
 * ```
 * 
 * @note This is a header-only library - no separate compilation required
 * @version 1.0.0
 * @author XCR1793 (Feather Forge)
 */
namespace arcos {
  /** Library version information */
  constexpr const char* VERSION = "1.0.0";
  constexpr int VERSION_MAJOR = 1;
  constexpr int VERSION_MINOR = 0;
  constexpr int VERSION_PATCH = 0;
  
  /** Module availability flags */
  constexpr bool CORE_AVAILABLE = true;
  constexpr bool ALGORITHMS_AVAILABLE = true;
  constexpr bool DRIVERS_AVAILABLE = true;
  constexpr bool PROCESSING_AVAILABLE = true;
}

#endif // ARCOS_ARCOS_HPP_