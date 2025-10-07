/*****************************************************************
 * File:      test_imu_fusion.cpp
 * Category:  tests/algorithms
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Google Test unit tests for IMU fusion algorithm.
 *****************************************************************/

#include <gtest/gtest.h>
#include <cmath>
#include "algorithms/fusion/imu_fusion/imu_fusion.hpp"

using namespace arcos::algorithms::fusion;

class IMUFusionTest : public ::testing::Test{
protected:
  IMUFusion<float> fusion;
  IMUFusionConfig<float> config;
  
  void SetUp() override{
    config.gyro_weight = 0.98f;
    config.accel_weight = 0.02f;
    config.sample_rate_hz = 100.0f;
    config.use_magnetometer = false;
  }
};

TEST_F(IMUFusionTest, InitializationSuccess){
  EXPECT_TRUE(fusion.init(config));
  EXPECT_TRUE(fusion.isInitialized());
}

TEST_F(IMUFusionTest, InitialOrientationIsZero){
  fusion.init(config);
  
  float roll, pitch, yaw;
  fusion.getEulerAngles(roll, pitch, yaw);
  
  EXPECT_FLOAT_EQ(roll, 0.0f);
  EXPECT_FLOAT_EQ(pitch, 0.0f);
  EXPECT_FLOAT_EQ(yaw, 0.0f);
}

TEST_F(IMUFusionTest, UpdateWithNoRotation){
  fusion.init(config);
  
  // Gravity pointing down (standard orientation)
  Vec3<float> accel(0.0f, 0.0f, 9.81f);
  Vec3<float> gyro(0.0f, 0.0f, 0.0f);
  float dt = 0.01f; // 10ms
  
  fusion.update(accel, gyro, dt);
  
  float roll, pitch, yaw;
  fusion.getEulerAngles(roll, pitch, yaw);
  
  // Should remain close to zero
  EXPECT_NEAR(roll, 0.0f, 0.1f);
  EXPECT_NEAR(pitch, 0.0f, 0.1f);
}

TEST_F(IMUFusionTest, GyroIntegration){
  fusion.init(config);
  
  Vec3<float> accel(0.0f, 0.0f, 9.81f);
  Vec3<float> gyro(1.0f, 0.0f, 0.0f); // 1 rad/s around X-axis
  float dt = 0.01f;
  
  // Update 100 times (1 second of rotation)
  for(int i = 0; i < 100; i++){
    fusion.update(accel, gyro, dt);
  }
  
  float roll, pitch, yaw;
  fusion.getEulerAngles(roll, pitch, yaw);
  
  // After 1 second at 1 rad/s with 98% gyro weight, roll should be ~0.98 rad
  // But accel influence pulls it down slightly, expect around 0.4-0.5
  EXPECT_GT(roll, 0.35f);
  EXPECT_LT(roll, 1.0f);
}

TEST_F(IMUFusionTest, AccelerometerTilt){
  fusion.init(config);
  
  // Accelerometer shows tilt (gravity in X direction)
  Vec3<float> accel(9.81f, 0.0f, 0.0f);
  Vec3<float> gyro(0.0f, 0.0f, 0.0f);
  float dt = 0.01f;
  
  // Multiple updates to let accel influence settle (needs many iterations with 2% weight)
  for(int i = 0; i < 100; i++){
    fusion.update(accel, gyro, dt);
  }
  
  float roll, pitch, yaw;
  fusion.getEulerAngles(roll, pitch, yaw);
  
  // Pitch should be negative (tilted forward) - with low accel weight, expect smaller angle
  EXPECT_LT(pitch, -0.5f);
  EXPECT_GT(pitch, -1.6f);
}

TEST_F(IMUFusionTest, ResetClearsOrientation){
  fusion.init(config);
  
  Vec3<float> accel(0.0f, 0.0f, 9.81f);
  Vec3<float> gyro(1.0f, 1.0f, 1.0f);
  float dt = 0.01f;
  
  for(int i = 0; i < 50; i++){
    fusion.update(accel, gyro, dt);
  }
  
  fusion.reset();
  
  float roll, pitch, yaw;
  fusion.getEulerAngles(roll, pitch, yaw);
  
  EXPECT_FLOAT_EQ(roll, 0.0f);
  EXPECT_FLOAT_EQ(pitch, 0.0f);
  EXPECT_FLOAT_EQ(yaw, 0.0f);
}

TEST_F(IMUFusionTest, Vec3Operations){
  Vec3<float> v1(1.0f, 2.0f, 3.0f);
  Vec3<float> v2(4.0f, 5.0f, 6.0f);
  
  Vec3<float> sum = v1 + v2;
  EXPECT_FLOAT_EQ(sum.x, 5.0f);
  EXPECT_FLOAT_EQ(sum.y, 7.0f);
  EXPECT_FLOAT_EQ(sum.z, 9.0f);
  
  Vec3<float> diff = v2 - v1;
  EXPECT_FLOAT_EQ(diff.x, 3.0f);
  EXPECT_FLOAT_EQ(diff.y, 3.0f);
  EXPECT_FLOAT_EQ(diff.z, 3.0f);
  
  Vec3<float> scaled = v1 * 2.0f;
  EXPECT_FLOAT_EQ(scaled.x, 2.0f);
  EXPECT_FLOAT_EQ(scaled.y, 4.0f);
  EXPECT_FLOAT_EQ(scaled.z, 6.0f);
}
