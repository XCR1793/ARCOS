/*****************************************************************
 * File:      quaternion.hpp
 * Category:  algorithms/orientation/quaternion
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Quaternion representation for 3D orientation with operations
 *    for rotation, conversion, and interpolation.
 *****************************************************************/

#ifndef ARCOS_ALGORITHMS_ORIENTATION_QUATERNION_HPP_
#define ARCOS_ALGORITHMS_ORIENTATION_QUATERNION_HPP_

#include <stdint.h>

namespace arcos::algorithms::orientation{

/** Quaternion structure for orientation representation
 * 
 * Quaternion format: q = w + xi + yj + zk
 * where w is the scalar part and (x, y, z) is the vector part
 */
struct Quaternion{
  float w, x, y, z;
  
  /** Default constructor - identity quaternion */
  Quaternion() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}
  
  /** Constructor with components */
  Quaternion(float w_, float x_, float y_, float z_) : w(w_), x(x_), y(y_), z(z_) {}
  
  /** Normalize the quaternion */
  void normalize();
  
  /** Get conjugate */
  Quaternion conjugate() const;
  
  /** Quaternion multiplication */
  Quaternion operator*(const Quaternion& q) const;
  
  /** Convert to Euler angles (roll, pitch, yaw in radians) */
  void toEuler(float& roll, float& pitch, float& yaw) const;
  
  /** Create quaternion from Euler angles (radians) */
  static Quaternion fromEuler(float roll, float pitch, float yaw);
  
  /** Create quaternion from axis-angle representation
   * @param axis Rotation axis (must be normalized)
   * @param angle Rotation angle in radians
   */
  static Quaternion fromAxisAngle(float axis_x, float axis_y, float axis_z, float angle);
  
  /** Spherical linear interpolation between two quaternions
   * @param q1 Start quaternion
   * @param q2 End quaternion
   * @param t Interpolation parameter [0, 1]
   * @return Interpolated quaternion
   */
  static Quaternion slerp(const Quaternion& q1, const Quaternion& q2, float t);
};

} // namespace arcos::algorithms::orientation

// Include implementation
#include "quaternion_impl.hpp"

#endif // ARCOS_ALGORITHMS_ORIENTATION_QUATERNION_HPP_
