/*****************************************************************
 * File:      test_euler_angles.cpp
 * Category:  tests/algorithms
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Google Test unit tests for Euler angles operations.
 *****************************************************************/

#include <gtest/gtest.h>
#include <cmath>
#include "algorithms/orientation/euler_angles/euler_angles.hpp"

using namespace arcos::algorithms::orientation;

class EulerAnglesTest : public ::testing::Test{
protected:
  const float EPSILON = 0.0001f;
  const float PI = 3.14159265358979323846f;
};

TEST_F(EulerAnglesTest, DefaultConstructorIsZero){
  EulerAngles angles;
  EXPECT_FLOAT_EQ(angles.roll, 0.0f);
  EXPECT_FLOAT_EQ(angles.pitch, 0.0f);
  EXPECT_FLOAT_EQ(angles.yaw, 0.0f);
}

TEST_F(EulerAnglesTest, ConstructorWithValues){
  EulerAngles angles(0.5f, 1.0f, 1.5f);
  EXPECT_FLOAT_EQ(angles.roll, 0.5f);
  EXPECT_FLOAT_EQ(angles.pitch, 1.0f);
  EXPECT_FLOAT_EQ(angles.yaw, 1.5f);
}

TEST_F(EulerAnglesTest, NormalizeWithinRange){
  EulerAngles angles(0.5f, 1.0f, 1.5f);
  angles.normalize();
  
  EXPECT_FLOAT_EQ(angles.roll, 0.5f);
  EXPECT_FLOAT_EQ(angles.pitch, 1.0f);
  EXPECT_FLOAT_EQ(angles.yaw, 1.5f);
}

TEST_F(EulerAnglesTest, NormalizeOutOfRange){
  EulerAngles angles(4.0f * PI, -4.0f * PI, 3.0f * PI);
  angles.normalize();
  
  EXPECT_GT(angles.roll, -PI);
  EXPECT_LT(angles.roll, PI);
  EXPECT_GT(angles.pitch, -PI);
  EXPECT_LT(angles.pitch, PI);
  EXPECT_GT(angles.yaw, -PI);
  EXPECT_LT(angles.yaw, PI);
}

TEST_F(EulerAnglesTest, ToDegreesConversion){
  EulerAngles angles(PI / 2.0f, PI / 4.0f, PI);
  
  float roll_deg, pitch_deg, yaw_deg;
  angles.toDegrees(roll_deg, pitch_deg, yaw_deg);
  
  EXPECT_NEAR(roll_deg, 90.0f, 0.01f);
  EXPECT_NEAR(pitch_deg, 45.0f, 0.01f);
  EXPECT_NEAR(yaw_deg, 180.0f, 0.01f);
}

TEST_F(EulerAnglesTest, FromDegreesConversion){
  EulerAngles angles = EulerAngles::fromDegrees(90.0f, 45.0f, 180.0f);
  
  EXPECT_NEAR(angles.roll, PI / 2.0f, 0.01f);
  EXPECT_NEAR(angles.pitch, PI / 4.0f, 0.01f);
  EXPECT_NEAR(angles.yaw, PI, 0.01f);
}

TEST_F(EulerAnglesTest, DegreesConversionRoundTrip){
  float original_roll_deg = 30.0f;
  float original_pitch_deg = 60.0f;
  float original_yaw_deg = 90.0f;
  
  EulerAngles angles = EulerAngles::fromDegrees(original_roll_deg, original_pitch_deg, original_yaw_deg);
  
  float roll_deg, pitch_deg, yaw_deg;
  angles.toDegrees(roll_deg, pitch_deg, yaw_deg);
  
  EXPECT_NEAR(roll_deg, original_roll_deg, 0.01f);
  EXPECT_NEAR(pitch_deg, original_pitch_deg, 0.01f);
  EXPECT_NEAR(yaw_deg, original_yaw_deg, 0.01f);
}

TEST_F(EulerAnglesTest, ToRotationMatrixIdentity){
  EulerAngles angles(0.0f, 0.0f, 0.0f);
  float matrix[9];
  
  angles.toRotationMatrix(matrix);
  
  // Should be identity matrix
  EXPECT_NEAR(matrix[0], 1.0f, EPSILON);
  EXPECT_NEAR(matrix[1], 0.0f, EPSILON);
  EXPECT_NEAR(matrix[2], 0.0f, EPSILON);
  EXPECT_NEAR(matrix[3], 0.0f, EPSILON);
  EXPECT_NEAR(matrix[4], 1.0f, EPSILON);
  EXPECT_NEAR(matrix[5], 0.0f, EPSILON);
  EXPECT_NEAR(matrix[6], 0.0f, EPSILON);
  EXPECT_NEAR(matrix[7], 0.0f, EPSILON);
  EXPECT_NEAR(matrix[8], 1.0f, EPSILON);
}

TEST_F(EulerAnglesTest, FromRotationMatrixIdentity){
  float identity[9] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
  };
  
  EulerAngles angles = EulerAngles::fromRotationMatrix(identity);
  
  EXPECT_NEAR(angles.roll, 0.0f, 0.01f);
  EXPECT_NEAR(angles.pitch, 0.0f, 0.01f);
  EXPECT_NEAR(angles.yaw, 0.0f, 0.01f);
}

TEST_F(EulerAnglesTest, RotationMatrixRoundTrip){
  EulerAngles original(0.5f, 0.3f, 0.7f);
  float matrix[9];
  
  original.toRotationMatrix(matrix);
  EulerAngles converted = EulerAngles::fromRotationMatrix(matrix);
  
  EXPECT_NEAR(converted.roll, original.roll, 0.01f);
  EXPECT_NEAR(converted.pitch, original.pitch, 0.01f);
  EXPECT_NEAR(converted.yaw, original.yaw, 0.01f);
}

TEST_F(EulerAnglesTest, RotationMatrix90DegreeRoll){
  EulerAngles angles(PI / 2.0f, 0.0f, 0.0f);
  float matrix[9];
  
  angles.toRotationMatrix(matrix);
  
  // Rotation around X-axis by 90 degrees
  EXPECT_NEAR(matrix[0], 1.0f, 0.01f);
  EXPECT_NEAR(matrix[4], 0.0f, 0.01f);
  EXPECT_NEAR(matrix[5], -1.0f, 0.01f);
  EXPECT_NEAR(matrix[7], 1.0f, 0.01f);
  EXPECT_NEAR(matrix[8], 0.0f, 0.01f);
}
