/*****************************************************************
 * File:      test_gravity_vector.cpp
 * Category:  tests/algorithms
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Google Test unit tests for gravity vector calculation.
 *****************************************************************/

#include <gtest/gtest.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "algorithms/orientation/gravity_vector/gravity_vector.hpp"

using namespace arcos::algorithms::orientation;

class GravityVectorTest : public ::testing::Test{
protected:
  GravityVector gravity_calc;
  GravityVectorConfig config;
  const float EPSILON = 0.01f;
  const float PI = 3.14159265358979323846f;
  
  void SetUp() override{
    config.gravity_magnitude = 9.81f;
    config.use_local_gravity = false;
    config.filter_alpha = 1.0f; // No filtering for tests
  }
};

TEST_F(GravityVectorTest, InitializationSuccess){
  EXPECT_TRUE(gravity_calc.init(config));
  EXPECT_TRUE(gravity_calc.isInitialized());
}

TEST_F(GravityVectorTest, LevelOrientationGravityPointsDown){
  gravity_calc.init(config);
  
  // Level orientation (no rotation)
  Vec3 gravity = gravity_calc.calculateFromEuler(0.0f, 0.0f, 0.0f);
  
  // Gravity should point down in body frame (positive Z for downward convention)
  EXPECT_NEAR(gravity.x, 0.0f, EPSILON);
  EXPECT_NEAR(gravity.y, 0.0f, EPSILON);
  EXPECT_NEAR(gravity.z, 9.81f, EPSILON);
}

TEST_F(GravityVectorTest, Roll90DegreesGravityInYAxis){
  gravity_calc.init(config);
  
  // Roll 90 degrees (tilted right)
  Vec3 gravity = gravity_calc.calculateFromEuler(PI / 2.0f, 0.0f, 0.0f);
  
  // Gravity should now point in Y direction
  EXPECT_NEAR(gravity.x, 0.0f, EPSILON);
  EXPECT_NEAR(gravity.y, 9.81f, EPSILON);
  EXPECT_NEAR(gravity.z, 0.0f, EPSILON);
}

TEST_F(GravityVectorTest, Pitch90DegreesGravityInXAxis){
  gravity_calc.init(config);
  
  // Pitch 90 degrees (nose up)
  Vec3 gravity = gravity_calc.calculateFromEuler(0.0f, PI / 2.0f, 0.0f);
  
  // Gravity should point backward (negative X)
  EXPECT_NEAR(gravity.x, -9.81f, EPSILON);
  EXPECT_NEAR(gravity.y, 0.0f, EPSILON);
  EXPECT_NEAR(gravity.z, 0.0f, EPSILON);
}

TEST_F(GravityVectorTest, UpsideDownGravityPointsUp){
  gravity_calc.init(config);
  
  // Roll 180 degrees (upside down)
  Vec3 gravity = gravity_calc.calculateFromEuler(PI, 0.0f, 0.0f);
  
  // Gravity should point up in body frame (negative Z)
  EXPECT_NEAR(gravity.x, 0.0f, EPSILON);
  EXPECT_NEAR(gravity.y, 0.0f, EPSILON);
  EXPECT_NEAR(gravity.z, -9.81f, EPSILON);
}

TEST_F(GravityVectorTest, RemoveGravityLevelOrientation){
  gravity_calc.init(config);
  
  // Accelerometer reading at rest (measuring gravity)
  Vec3 accel_reading(0.0f, 0.0f, 9.81f);
  
  // Remove gravity
  Vec3 linear_accel = gravity_calc.removeGravity(accel_reading, 0.0f, 0.0f, 0.0f);
  
  // Should be zero (no linear acceleration)
  EXPECT_NEAR(linear_accel.x, 0.0f, EPSILON);
  EXPECT_NEAR(linear_accel.y, 0.0f, EPSILON);
  EXPECT_NEAR(linear_accel.z, 0.0f, EPSILON);
}

TEST_F(GravityVectorTest, RemoveGravityWithLinearAcceleration){
  gravity_calc.init(config);
  
  // Accelerometer reading: gravity + 2 m/s^2 acceleration in X
  Vec3 accel_reading(2.0f, 0.0f, 9.81f);
  
  // Remove gravity
  Vec3 linear_accel = gravity_calc.removeGravity(accel_reading, 0.0f, 0.0f, 0.0f);
  
  // Should isolate the linear acceleration
  EXPECT_NEAR(linear_accel.x, 2.0f, EPSILON);
  EXPECT_NEAR(linear_accel.y, 0.0f, EPSILON);
  EXPECT_NEAR(linear_accel.z, 0.0f, EPSILON);
}

TEST_F(GravityVectorTest, CustomGravityMagnitude){
  config.gravity_magnitude = 1.62f; // Moon gravity
  gravity_calc.init(config);
  
  Vec3 gravity = gravity_calc.calculateFromEuler(0.0f, 0.0f, 0.0f);
  
  EXPECT_NEAR(gravity.z, 1.62f, EPSILON);
}

TEST_F(GravityVectorTest, FilteringSmooths){
  config.filter_alpha = 0.5f; // 50% filtering
  gravity_calc.init(config);
  
  // First call
  Vec3 gravity1 = gravity_calc.calculateFromEuler(0.0f, 0.0f, 0.0f);
  EXPECT_NEAR(gravity1.z, 9.81f, EPSILON);
  
  // Second call with different orientation
  Vec3 gravity2 = gravity_calc.calculateFromEuler(PI / 4.0f, 0.0f, 0.0f);
  
  // Should be filtered (between two values)
  float expected_z_unfiltered = 9.81f * cosf(PI / 4.0f);
  EXPECT_GT(gravity2.z, expected_z_unfiltered);
  EXPECT_LT(gravity2.z, 9.81f);
}

TEST_F(GravityVectorTest, ResetClearsFilter){
  config.filter_alpha = 0.5f;
  gravity_calc.init(config);
  
  gravity_calc.calculateFromEuler(0.0f, 0.0f, 0.0f);
  gravity_calc.reset();
  
  // After reset, no filtering should occur
  Vec3 gravity = gravity_calc.calculateFromEuler(PI / 2.0f, 0.0f, 0.0f);
  
  EXPECT_NEAR(gravity.y, 9.81f, EPSILON);
  EXPECT_NEAR(gravity.z, 0.0f, EPSILON);
}

TEST_F(GravityVectorTest, GetLastGravityVector){
  gravity_calc.init(config);
  
  Vec3 calculated = gravity_calc.calculateFromEuler(0.0f, 0.0f, 0.0f);
  Vec3 last = gravity_calc.getLastGravityVector();
  
  EXPECT_FLOAT_EQ(last.x, calculated.x);
  EXPECT_FLOAT_EQ(last.y, calculated.y);
  EXPECT_FLOAT_EQ(last.z, calculated.z);
}

TEST_F(GravityVectorTest, Vec3DotProduct){
  Vec3 v1(1.0f, 0.0f, 0.0f);
  Vec3 v2(0.0f, 1.0f, 0.0f);
  Vec3 v3(1.0f, 0.0f, 0.0f);
  
  EXPECT_FLOAT_EQ(v1.dot(v2), 0.0f); // Perpendicular
  EXPECT_FLOAT_EQ(v1.dot(v3), 1.0f); // Parallel
}

TEST_F(GravityVectorTest, RotationMatrixMethod){
  gravity_calc.init(config);
  
  // Identity rotation matrix
  float identity[9] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
  };
  
  Vec3 gravity = gravity_calc.calculateFromRotationMatrix(identity);
  
  EXPECT_NEAR(gravity.x, 0.0f, EPSILON);
  EXPECT_NEAR(gravity.y, 0.0f, EPSILON);
  EXPECT_NEAR(gravity.z, 9.81f, EPSILON);
}
