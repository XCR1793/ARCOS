/*****************************************************************
 * File:      quaternion_impl.hpp
 * Category:  algorithms/orientation/quaternion
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Implementation of quaternion operations.
 *****************************************************************/

#ifndef ARCOS_ALGORITHMS_ORIENTATION_QUATERNION_IMPL_HPP_
#define ARCOS_ALGORITHMS_ORIENTATION_QUATERNION_IMPL_HPP_

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace arcos::algorithms::orientation{

inline void Quaternion::normalize(){
  float mag = sqrtf(w*w + x*x + y*y + z*z);
  if(mag > 0.0001f){
    w /= mag;
    x /= mag;
    y /= mag;
    z /= mag;
  }
}

inline Quaternion Quaternion::conjugate() const{
  return Quaternion(w, -x, -y, -z);
}

inline Quaternion Quaternion::operator*(const Quaternion& q) const{
  return Quaternion(
    w*q.w - x*q.x - y*q.y - z*q.z,
    w*q.x + x*q.w + y*q.z - z*q.y,
    w*q.y - x*q.z + y*q.w + z*q.x,
    w*q.z + x*q.y - y*q.x + z*q.w
  );
}

inline void Quaternion::toEuler(float& roll, float& pitch, float& yaw) const{
  // Roll (x-axis rotation)
  float sinr_cosp = 2.0f * (w * x + y * z);
  float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
  roll = atan2f(sinr_cosp, cosr_cosp);
  
  // Pitch (y-axis rotation)
  float sinp = 2.0f * (w * y - z * x);
  if(fabsf(sinp) >= 1.0f){
    pitch = copysignf(M_PI / 2.0f, sinp); // Use 90 degrees if out of range
  }else{
    pitch = asinf(sinp);
  }
  
  // Yaw (z-axis rotation)
  float siny_cosp = 2.0f * (w * z + x * y);
  float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
  yaw = atan2f(siny_cosp, cosy_cosp);
}

inline Quaternion Quaternion::fromEuler(float roll, float pitch, float yaw){
  float cy = cosf(yaw * 0.5f);
  float sy = sinf(yaw * 0.5f);
  float cp = cosf(pitch * 0.5f);
  float sp = sinf(pitch * 0.5f);
  float cr = cosf(roll * 0.5f);
  float sr = sinf(roll * 0.5f);
  
  Quaternion q;
  q.w = cr * cp * cy + sr * sp * sy;
  q.x = sr * cp * cy - cr * sp * sy;
  q.y = cr * sp * cy + sr * cp * sy;
  q.z = cr * cp * sy - sr * sp * cy;
  
  return q;
}

inline Quaternion Quaternion::fromAxisAngle(float axis_x, float axis_y, float axis_z, float angle){
  float half_angle = angle * 0.5f;
  float s = sinf(half_angle);
  
  return Quaternion(
    cosf(half_angle),
    axis_x * s,
    axis_y * s,
    axis_z * s
  );
}

inline Quaternion Quaternion::slerp(const Quaternion& q1, const Quaternion& q2, float t){
  Quaternion result;
  
  // Calculate dot product
  float dot = q1.w*q2.w + q1.x*q2.x + q1.y*q2.y + q1.z*q2.z;
  
  // If negative dot, negate one quaternion to take shortest path
  Quaternion q2_temp = q2;
  if(dot < 0.0f){
    q2_temp.w = -q2_temp.w;
    q2_temp.x = -q2_temp.x;
    q2_temp.y = -q2_temp.y;
    q2_temp.z = -q2_temp.z;
    dot = -dot;
  }
  
  // If quaternions are very close, use linear interpolation
  if(dot > 0.9995f){
    result.w = q1.w + t * (q2_temp.w - q1.w);
    result.x = q1.x + t * (q2_temp.x - q1.x);
    result.y = q1.y + t * (q2_temp.y - q1.y);
    result.z = q1.z + t * (q2_temp.z - q1.z);
    result.normalize();
    return result;
  }
  
  // Spherical interpolation
  float theta_0 = acosf(dot);
  float theta = theta_0 * t;
  float sin_theta = sinf(theta);
  float sin_theta_0 = sinf(theta_0);
  
  float s0 = cosf(theta) - dot * sin_theta / sin_theta_0;
  float s1 = sin_theta / sin_theta_0;
  
  result.w = s0 * q1.w + s1 * q2_temp.w;
  result.x = s0 * q1.x + s1 * q2_temp.x;
  result.y = s0 * q1.y + s1 * q2_temp.y;
  result.z = s0 * q1.z + s1 * q2_temp.z;
  
  return result;
}

} // namespace arcos::algorithms::orientation

#endif // ARCOS_ALGORITHMS_ORIENTATION_QUATERNION_IMPL_HPP_
