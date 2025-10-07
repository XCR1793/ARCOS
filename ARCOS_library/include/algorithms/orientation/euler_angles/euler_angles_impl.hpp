/*****************************************************************
 * File:      euler_angles_impl.hpp
 * Category:  algorithms/orientation/euler_angles
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Implementation of Euler angle operations.
 *****************************************************************/

#ifndef ARCOS_ALGORITHMS_ORIENTATION_EULER_ANGLES_IMPL_HPP_
#define ARCOS_ALGORITHMS_ORIENTATION_EULER_ANGLES_IMPL_HPP_

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace arcos::algorithms::orientation{

inline void EulerAngles::normalize(){
  // Normalize to [-PI, PI]
  auto normalizeAngle = [](float angle) -> float {
    while(angle > M_PI) angle -= 2.0f * M_PI;
    while(angle < -M_PI) angle += 2.0f * M_PI;
    return angle;
  };
  
  roll = normalizeAngle(roll);
  pitch = normalizeAngle(pitch);
  yaw = normalizeAngle(yaw);
}

inline void EulerAngles::toDegrees(float& roll_deg, float& pitch_deg, float& yaw_deg) const{
  constexpr float RAD_TO_DEG = 180.0f / M_PI;
  roll_deg = roll * RAD_TO_DEG;
  pitch_deg = pitch * RAD_TO_DEG;
  yaw_deg = yaw * RAD_TO_DEG;
}

inline EulerAngles EulerAngles::fromDegrees(float roll_deg, float pitch_deg, float yaw_deg){
  constexpr float DEG_TO_RAD = M_PI / 180.0f;
  return EulerAngles(
    roll_deg * DEG_TO_RAD,
    pitch_deg * DEG_TO_RAD,
    yaw_deg * DEG_TO_RAD
  );
}

inline void EulerAngles::toRotationMatrix(float matrix[9]) const{
  float cr = cosf(roll);
  float sr = sinf(roll);
  float cp = cosf(pitch);
  float sp = sinf(pitch);
  float cy = cosf(yaw);
  float sy = sinf(yaw);
  
  // Z-Y-X rotation order (yaw-pitch-roll)
  matrix[0] = cy * cp;
  matrix[1] = cy * sp * sr - sy * cr;
  matrix[2] = cy * sp * cr + sy * sr;
  
  matrix[3] = sy * cp;
  matrix[4] = sy * sp * sr + cy * cr;
  matrix[5] = sy * sp * cr - cy * sr;
  
  matrix[6] = -sp;
  matrix[7] = cp * sr;
  matrix[8] = cp * cr;
}

inline EulerAngles EulerAngles::fromRotationMatrix(const float matrix[9]){
  EulerAngles angles;
  
  // Extract pitch
  float sin_pitch = -matrix[6];
  if(fabsf(sin_pitch) >= 1.0f){
    angles.pitch = copysignf(M_PI / 2.0f, sin_pitch); // Gimbal lock case
    angles.roll = 0.0f;
    angles.yaw = atan2f(-matrix[1], matrix[4]);
  }else{
    angles.pitch = asinf(sin_pitch);
    angles.roll = atan2f(matrix[7], matrix[8]);
    angles.yaw = atan2f(matrix[3], matrix[0]);
  }
  
  return angles;
}

} // namespace arcos::algorithms::orientation

#endif // ARCOS_ALGORITHMS_ORIENTATION_EULER_ANGLES_IMPL_HPP_
