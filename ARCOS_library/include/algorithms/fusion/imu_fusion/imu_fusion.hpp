/*****************************************************************
 * File:      imu_fusion.hpp
 * Category:  algorithms/fusion/imu_fusion
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    IMU sensor fusion combining accelerometer, gyroscope, and
 *    magnetometer data for accurate orientation estimation.
 *****************************************************************/

#ifndef ARCOS_ALGORITHMS_FUSION_IMU_FUSION_HPP_
#define ARCOS_ALGORITHMS_FUSION_IMU_FUSION_HPP_

#include <stdint.h>

namespace arcos::algorithms::fusion{

/** 3D vector structure template */
template<typename T>
struct Vec3{
  T x, y, z;
  
  Vec3() : x(0), y(0), z(0) {}
  Vec3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}
  
  Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
  Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
  Vec3 operator*(T s) const { return Vec3(x * s, y * s, z * s); }
  Vec3 operator/(T s) const { return Vec3(x / s, y / s, z / s); }
};

// Type aliases for common uses
using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;

/** IMU fusion configuration template */
template<typename T>
struct IMUFusionConfig{
  T gyro_weight = static_cast<T>(0.98);        // Weight for gyroscope data
  T accel_weight = static_cast<T>(0.02);       // Weight for accelerometer data
  T mag_weight = static_cast<T>(0.0);          // Weight for magnetometer data (optional)
  T sample_rate_hz = static_cast<T>(100.0);    // Sampling rate in Hz
  bool use_magnetometer = false;                // Enable magnetometer fusion
};

// Type aliases for common uses
using IMUFusionConfigf = IMUFusionConfig<float>;
using IMUFusionConfigd = IMUFusionConfig<double>;

/** IMU sensor fusion algorithm template
 * 
 * Fuses accelerometer, gyroscope, and optional magnetometer data
 * to provide accurate orientation estimation. Uses complementary
 * filter approach for computational efficiency.
 * 
 * @tparam T Scalar type (float, double, or fixed-point)
 */
template<typename T = float>
class IMUFusion{
public:
  IMUFusion();
  ~IMUFusion();
  
  /** Initialize IMU fusion
   * @param config Fusion configuration parameters
   * @return true if successful
   */
  bool init(const IMUFusionConfig<T>& config);
  
  /** Update fusion with new sensor data
   * @param accel Accelerometer reading (m/s^2)
   * @param gyro Gyroscope reading (rad/s)
   * @param mag Magnetometer reading (optional, uT)
   * @param dt Time delta since last update (seconds)
   */
  void update(const Vec3<T>& accel, const Vec3<T>& gyro, const Vec3<T>& mag, T dt);
  
  /** Update fusion with new sensor data (no magnetometer)
   * @param accel Accelerometer reading (m/s^2)
   * @param gyro Gyroscope reading (rad/s)
   * @param dt Time delta since last update (seconds)
   */
  void update(const Vec3<T>& accel, const Vec3<T>& gyro, T dt);
  
  /** Get current orientation as Euler angles (radians)
   * @param roll Output roll angle
   * @param pitch Output pitch angle
   * @param yaw Output yaw angle
   */
  void getEulerAngles(T& roll, T& pitch, T& yaw) const;
  
  /** Reset fusion state */
  void reset();
  
  /** Check if initialized */
  bool isInitialized() const { return initialized_; }
  
private:
  bool initialized_;
  IMUFusionConfig<T> config_;
  
  // Current orientation (Euler angles in radians)
  T roll_;
  T pitch_;
  T yaw_;
  
  /** Calculate roll and pitch from accelerometer */
  void calculateAccelAngles(const Vec3<T>& accel, T& roll, T& pitch);
  
  /** Normalize vector */
  Vec3<T> normalize(const Vec3<T>& v);
  
  /** Calculate magnitude of vector */
  T magnitude(const Vec3<T>& v);
};

// Type aliases for common uses
using IMUFusionf = IMUFusion<float>;
using IMUFusiond = IMUFusion<double>;

} // namespace arcos::algorithms::fusion

// Include implementation
#include "imu_fusion_impl.hpp"

#endif // ARCOS_ALGORITHMS_FUSION_IMU_FUSION_HPP_
