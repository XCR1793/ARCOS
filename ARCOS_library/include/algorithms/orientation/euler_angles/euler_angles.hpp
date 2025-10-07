/*****************************************************************
 * File:      euler_angles.hpp
 * Category:  algorithms/orientation/euler_angles
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Euler angle representation and conversion utilities for
 *    3D orientation (roll, pitch, yaw).
 *****************************************************************/

#ifndef ARCOS_ALGORITHMS_ORIENTATION_EULER_ANGLES_HPP_
#define ARCOS_ALGORITHMS_ORIENTATION_EULER_ANGLES_HPP_

#include <stdint.h>

namespace arcos::algorithms::orientation{

/** Euler angles structure (roll, pitch, yaw)
 * 
 * Convention: Tait-Bryan angles (Z-Y-X rotation order)
 * - Roll: rotation around X-axis
 * - Pitch: rotation around Y-axis
 * - Yaw: rotation around Z-axis
 * All angles in radians
 */
struct EulerAngles{
  float roll;   // Rotation around X-axis (radians)
  float pitch;  // Rotation around Y-axis (radians)
  float yaw;    // Rotation around Z-axis (radians)
  
  /** Default constructor */
  EulerAngles() : roll(0.0f), pitch(0.0f), yaw(0.0f) {}
  
  /** Constructor with values */
  EulerAngles(float r, float p, float y) : roll(r), pitch(p), yaw(y) {}
  
  /** Normalize angles to [-PI, PI] range */
  void normalize();
  
  /** Convert to degrees */
  void toDegrees(float& roll_deg, float& pitch_deg, float& yaw_deg) const;
  
  /** Create from degrees */
  static EulerAngles fromDegrees(float roll_deg, float pitch_deg, float yaw_deg);
  
  /** Calculate rotation matrix from Euler angles
   * @param matrix Output 3x3 rotation matrix (row-major order)
   */
  void toRotationMatrix(float matrix[9]) const;
  
  /** Create Euler angles from rotation matrix
   * @param matrix Input 3x3 rotation matrix (row-major order)
   * @return Euler angles
   */
  static EulerAngles fromRotationMatrix(const float matrix[9]);
};

} // namespace arcos::algorithms::orientation

// Include implementation
#include "euler_angles_impl.hpp"

#endif // ARCOS_ALGORITHMS_ORIENTATION_EULER_ANGLES_HPP_
