/*****************************************************************
 * File:      arcos_algorithms.hpp
 * Category:  algorithms
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    ARCOS sensor fusion and mathematical algorithms module.
 *    Include this for IMU fusion, quaternion operations, and
 *    orientation calculations without drivers or HAL.
 *****************************************************************/

#ifndef ARCOS_ALGORITHMS_HPP_
#define ARCOS_ALGORITHMS_HPP_

// Sensor fusion algorithms
#include "algorithms/fusion/imu_fusion/imu_fusion.hpp"
#include "algorithms/fusion/sensor_fusion/sensor_fusion.hpp"

// Orientation algorithms  
#include "algorithms/orientation/euler_angles/euler_angles.hpp"
#include "algorithms/orientation/gravity_vector/gravity_vector.hpp"
#include "algorithms/orientation/quaternion/quaternion.hpp"

/** 
 * @brief ARCOS Algorithms Module
 * 
 * Provides sensor fusion and mathematical algorithms including:
 * - IMU sensor fusion (Madgwick, Mahony filters)
 * - Quaternion mathematics and operations
 * - Euler angle conversions
 * - Gravity vector calculations
 * - Multi-sensor fusion algorithms
 * 
 * Usage:
 * ```cpp
 * #include <arcos_algorithms.hpp>
 * 
 * using namespace arcos::algorithms;
 * 
 * // Use sensor fusion
 * fusion::IMUFusion imu_fusion;
 * orientation::Quaternion quat(1.0f, 0.0f, 0.0f, 0.0f);
 * ```
 */
namespace arcos::algorithms {
  constexpr const char* MODULE_NAME = "ARCOS_ALGORITHMS";
  constexpr const char* VERSION = "1.0.0";
}

#endif // ARCOS_ALGORITHMS_HPP_