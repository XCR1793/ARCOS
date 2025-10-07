/*****************************************************************
 * File:      gravity_vector.hpp
 * Category:  algorithms/orientation/gravity_vector
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Calculates gravity vector in body frame from IMU orientation
 *    and accelerometer data. Useful for gravity compensation and
 *    determining true linear acceleration.
 *****************************************************************/

#ifndef ARCOS_ALGORITHMS_ORIENTATION_GRAVITY_VECTOR_HPP_
#define ARCOS_ALGORITHMS_ORIENTATION_GRAVITY_VECTOR_HPP_

#include <stdint.h>

namespace arcos::algorithms::orientation{

/** 3D vector structure */
struct Vec3{
  float x, y, z;
  
  Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
  Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
  
  Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
  Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
  Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
  float dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }
};

/** Gravity vector calculator configuration */
struct GravityVectorConfig{
  float gravity_magnitude = 9.81f;  // Standard gravity in m/s^2
  bool use_local_gravity = false;   // Use local gravity value instead of standard
  float filter_alpha = 0.9f;        // Low-pass filter coefficient for smoothing
};

/** Gravity Vector Calculator
 * 
 * Calculates the gravity vector in the body frame given the current
 * orientation (roll, pitch, yaw). This allows separation of gravitational
 * acceleration from linear acceleration in IMU readings.
 * 
 * In the earth frame, gravity points downward (0, 0, -g). By knowing the
 * orientation, we can calculate what gravity looks like in the body frame
 * and subtract it from accelerometer readings to get true linear acceleration.
 */
class GravityVector{
public:
  GravityVector();
  ~GravityVector();
  
  /** Initialize gravity vector calculator
   * @param config Configuration parameters
   * @return true if successful
   */
  bool init(const GravityVectorConfig& config);
  
  /** Calculate gravity vector from Euler angles
   * @param roll Roll angle in radians
   * @param pitch Pitch angle in radians
   * @param yaw Yaw angle in radians (not used for gravity calculation)
   * @return Gravity vector in body frame (m/s^2)
   */
  Vec3 calculateFromEuler(float roll, float pitch, float yaw);
  
  /** Calculate gravity vector from rotation matrix
   * @param rotation_matrix 3x3 rotation matrix (row-major order)
   * @return Gravity vector in body frame (m/s^2)
   */
  Vec3 calculateFromRotationMatrix(const float rotation_matrix[9]);
  
  /** Remove gravity from accelerometer reading
   * @param accel_reading Raw accelerometer reading (m/s^2)
   * @param roll Roll angle in radians
   * @param pitch Pitch angle in radians
   * @param yaw Yaw angle in radians
   * @return Linear acceleration without gravity component
   */
  Vec3 removeGravity(const Vec3& accel_reading, float roll, float pitch, float yaw);
  
  /** Get last calculated gravity vector */
  Vec3 getLastGravityVector() const { return last_gravity_; }
  
  /** Reset filter state */
  void reset();
  
  /** Check if initialized */
  bool isInitialized() const { return initialized_; }
  
private:
  bool initialized_;
  GravityVectorConfig config_;
  Vec3 last_gravity_;
  bool has_previous_;
  
  /** Apply low-pass filter to gravity vector */
  Vec3 applyFilter(const Vec3& current, const Vec3& previous);
};

} // namespace arcos::algorithms::orientation

// Include implementation
#include "gravity_vector_impl.hpp"

#endif // ARCOS_ALGORITHMS_ORIENTATION_GRAVITY_VECTOR_HPP_
