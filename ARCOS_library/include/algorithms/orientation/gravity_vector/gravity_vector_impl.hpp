/*****************************************************************
 * File:      gravity_vector_impl.hpp
 * Category:  algorithms/orientation/gravity_vector
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Implementation of gravity vector calculation algorithm.
 *****************************************************************/

#ifndef ARCOS_ALGORITHMS_ORIENTATION_GRAVITY_VECTOR_IMPL_HPP_
#define ARCOS_ALGORITHMS_ORIENTATION_GRAVITY_VECTOR_IMPL_HPP_

#include <cmath>

namespace arcos::algorithms::orientation{

inline GravityVector::GravityVector()
  : initialized_(false)
  , has_previous_(false)
{
}

inline GravityVector::~GravityVector(){
}

inline bool GravityVector::init(const GravityVectorConfig& config){
  config_ = config;
  has_previous_ = false;
  last_gravity_ = Vec3(0.0f, 0.0f, 0.0f);
  initialized_ = true;
  return true;
}

inline Vec3 GravityVector::calculateFromEuler(float roll, float pitch, float yaw){
  if(!initialized_){
    return Vec3(0.0f, 0.0f, 0.0f);
  }
  
  // In earth frame, gravity points down: [0, 0, -g]
  // Transform to body frame using inverse rotation (transpose of rotation matrix)
  
  float cr = cosf(roll);
  float sr = sinf(roll);
  float cp = cosf(pitch);
  float sp = sinf(pitch);
  
  // Gravity in body frame (simplified - yaw doesn't affect gravity vector)
  // This is the third column of the rotation matrix transposed
  Vec3 gravity;
  gravity.x = -config_.gravity_magnitude * sp;
  gravity.y = config_.gravity_magnitude * cp * sr;
  gravity.z = config_.gravity_magnitude * cp * cr;
  
  // Apply filtering if we have previous data
  if(has_previous_){
    gravity = applyFilter(gravity, last_gravity_);
  }
  
  last_gravity_ = gravity;
  has_previous_ = true;
  
  return gravity;
}

inline Vec3 GravityVector::calculateFromRotationMatrix(const float rotation_matrix[9]){
  if(!initialized_){
    return Vec3(0.0f, 0.0f, 0.0f);
  }
  
  // Gravity in earth frame points down: [0, 0, -g] where +Z is up
  // Transform to body frame: R^T * [0, 0, -g]
  // This is -g times the third column of R^T (third row of R)
  // Result: gravity vector as measured by accelerometer in body frame
  
  Vec3 gravity;
  gravity.x = config_.gravity_magnitude * rotation_matrix[6];
  gravity.y = config_.gravity_magnitude * rotation_matrix[7];
  gravity.z = config_.gravity_magnitude * rotation_matrix[8];
  
  // Apply filtering if we have previous data
  if(has_previous_){
    gravity = applyFilter(gravity, last_gravity_);
  }
  
  last_gravity_ = gravity;
  has_previous_ = true;
  
  return gravity;
}

inline Vec3 GravityVector::removeGravity(const Vec3& accel_reading, float roll, float pitch, float yaw){
  if(!initialized_){
    return accel_reading;
  }
  
  // Calculate expected gravity in body frame
  Vec3 gravity = calculateFromEuler(roll, pitch, yaw);
  
  // Subtract gravity to get linear acceleration
  // Note: Accelerometer measures (a - g), so we add g to remove it
  // But since our gravity vector is already in the correct direction,
  // we just subtract it
  return accel_reading - gravity;
}

inline void GravityVector::reset(){
  has_previous_ = false;
  last_gravity_ = Vec3(0.0f, 0.0f, 0.0f);
}

inline Vec3 GravityVector::applyFilter(const Vec3& current, const Vec3& previous){
  // Low-pass filter: output = alpha * current + (1 - alpha) * previous
  // Higher alpha = more responsive, lower alpha = smoother
  float alpha = config_.filter_alpha;
  return Vec3(
    alpha * current.x + (1.0f - alpha) * previous.x,
    alpha * current.y + (1.0f - alpha) * previous.y,
    alpha * current.z + (1.0f - alpha) * previous.z
  );
}

} // namespace arcos::algorithms::orientation

#endif // ARCOS_ALGORITHMS_ORIENTATION_GRAVITY_VECTOR_IMPL_HPP_
