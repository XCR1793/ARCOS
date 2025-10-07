/*****************************************************************
 * File:      imu_fusion_impl.hpp
 * Category:  algorithms/fusion/imu_fusion
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Implementation of IMU sensor fusion algorithm template.
 *****************************************************************/

#ifndef ARCOS_ALGORITHMS_FUSION_IMU_FUSION_IMPL_HPP_
#define ARCOS_ALGORITHMS_FUSION_IMU_FUSION_IMPL_HPP_

#include <cmath>

namespace arcos::algorithms::fusion{

template<typename T>
IMUFusion<T>::IMUFusion()
  : initialized_(false)
  , roll_(static_cast<T>(0))
  , pitch_(static_cast<T>(0))
  , yaw_(static_cast<T>(0))
{
}

template<typename T>
IMUFusion<T>::~IMUFusion(){
}

template<typename T>
bool IMUFusion<T>::init(const IMUFusionConfig<T>& config){
  config_ = config;
  roll_ = static_cast<T>(0);
  pitch_ = static_cast<T>(0);
  yaw_ = static_cast<T>(0);
  initialized_ = true;
  return true;
}

template<typename T>
void IMUFusion<T>::update(const Vec3<T>& accel, const Vec3<T>& gyro, T dt){
  if(!initialized_){
    return;
  }
  
  // Integrate gyroscope data
  T gyro_roll = roll_ + gyro.x * dt;
  T gyro_pitch = pitch_ + gyro.y * dt;
  T gyro_yaw = yaw_ + gyro.z * dt;
  
  // Calculate angles from accelerometer
  T accel_roll, accel_pitch;
  calculateAccelAngles(accel, accel_roll, accel_pitch);
  
  // Complementary filter
  roll_ = config_.gyro_weight * gyro_roll + config_.accel_weight * accel_roll;
  pitch_ = config_.gyro_weight * gyro_pitch + config_.accel_weight * accel_pitch;
  yaw_ = gyro_yaw; // Yaw requires magnetometer for absolute reference
}

template<typename T>
void IMUFusion<T>::update(const Vec3<T>& accel, const Vec3<T>& gyro, const Vec3<T>& mag, T dt){
  if(!initialized_){
    return;
  }
  
  // First update without magnetometer
  update(accel, gyro, dt);
  
  // If magnetometer enabled, use it for yaw correction
  if(config_.use_magnetometer){
    // Calculate yaw from magnetometer (simplified)
    // This should be expanded with proper tilt compensation
    T mag_yaw = static_cast<T>(atan2(static_cast<double>(mag.y), static_cast<double>(mag.x)));
    yaw_ = config_.gyro_weight * yaw_ + config_.mag_weight * mag_yaw;
  }
}

template<typename T>
void IMUFusion<T>::getEulerAngles(T& roll, T& pitch, T& yaw) const{
  roll = roll_;
  pitch = pitch_;
  yaw = yaw_;
}

template<typename T>
void IMUFusion<T>::reset(){
  roll_ = static_cast<T>(0);
  pitch_ = static_cast<T>(0);
  yaw_ = static_cast<T>(0);
}

template<typename T>
void IMUFusion<T>::calculateAccelAngles(const Vec3<T>& accel, T& roll, T& pitch){
  // Normalize accelerometer reading
  Vec3<T> norm_accel = normalize(accel);
  
  // Calculate roll and pitch from accelerometer
  roll = static_cast<T>(atan2(static_cast<double>(norm_accel.y), static_cast<double>(norm_accel.z)));
  pitch = static_cast<T>(atan2(static_cast<double>(-norm_accel.x), 
                                sqrt(static_cast<double>(norm_accel.y * norm_accel.y + norm_accel.z * norm_accel.z))));
}

template<typename T>
Vec3<T> IMUFusion<T>::normalize(const Vec3<T>& v){
  T mag = magnitude(v);
  if(mag < static_cast<T>(0.0001)){
    return Vec3<T>(0, 0, 0);
  }
  return v / mag;
}

template<typename T>
T IMUFusion<T>::magnitude(const Vec3<T>& v){
  return static_cast<T>(sqrt(static_cast<double>(v.x * v.x + v.y * v.y + v.z * v.z)));
}

} // namespace arcos::algorithms::fusion

#endif // ARCOS_ALGORITHMS_FUSION_IMU_FUSION_IMPL_HPP_
