/*****************************************************************
 * File:      test_quaternion.cpp
 * Category:  tests/algorithms
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Google Test unit tests for quaternion operations.
 *****************************************************************/

#include <gtest/gtest.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "algorithms/orientation/quaternion/quaternion.hpp"

using namespace arcos::algorithms::orientation;

class QuaternionTest : public ::testing::Test{
protected:
  const float EPSILON = 0.0001f;
};

TEST_F(QuaternionTest, DefaultConstructorIsIdentity){
  Quaternion q;
  EXPECT_FLOAT_EQ(q.w, 1.0f);
  EXPECT_FLOAT_EQ(q.x, 0.0f);
  EXPECT_FLOAT_EQ(q.y, 0.0f);
  EXPECT_FLOAT_EQ(q.z, 0.0f);
}

TEST_F(QuaternionTest, ConstructorWithValues){
  Quaternion q(0.5f, 0.5f, 0.5f, 0.5f);
  EXPECT_FLOAT_EQ(q.w, 0.5f);
  EXPECT_FLOAT_EQ(q.x, 0.5f);
  EXPECT_FLOAT_EQ(q.y, 0.5f);
  EXPECT_FLOAT_EQ(q.z, 0.5f);
}

TEST_F(QuaternionTest, Normalize){
  Quaternion q(1.0f, 1.0f, 1.0f, 1.0f);
  q.normalize();
  
  float magnitude = sqrtf(q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z);
  EXPECT_NEAR(magnitude, 1.0f, EPSILON);
}

TEST_F(QuaternionTest, Conjugate){
  Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);
  Quaternion conj = q.conjugate();
  
  EXPECT_FLOAT_EQ(conj.w, 1.0f);
  EXPECT_FLOAT_EQ(conj.x, -2.0f);
  EXPECT_FLOAT_EQ(conj.y, -3.0f);
  EXPECT_FLOAT_EQ(conj.z, -4.0f);
}

TEST_F(QuaternionTest, IdentityMultiplication){
  Quaternion q(0.5f, 0.5f, 0.5f, 0.5f);
  Quaternion identity;
  
  Quaternion result = q * identity;
  
  EXPECT_NEAR(result.w, q.w, EPSILON);
  EXPECT_NEAR(result.x, q.x, EPSILON);
  EXPECT_NEAR(result.y, q.y, EPSILON);
  EXPECT_NEAR(result.z, q.z, EPSILON);
}

TEST_F(QuaternionTest, FromEulerZeroRotation){
  Quaternion q = Quaternion::fromEuler(0.0f, 0.0f, 0.0f);
  
  EXPECT_NEAR(q.w, 1.0f, EPSILON);
  EXPECT_NEAR(q.x, 0.0f, EPSILON);
  EXPECT_NEAR(q.y, 0.0f, EPSILON);
  EXPECT_NEAR(q.z, 0.0f, EPSILON);
}

TEST_F(QuaternionTest, ToEulerZeroRotation){
  Quaternion q; // Identity
  
  float roll, pitch, yaw;
  q.toEuler(roll, pitch, yaw);
  
  EXPECT_NEAR(roll, 0.0f, EPSILON);
  EXPECT_NEAR(pitch, 0.0f, EPSILON);
  EXPECT_NEAR(yaw, 0.0f, EPSILON);
}

TEST_F(QuaternionTest, EulerConversionRoundTrip){
  float original_roll = 0.5f;
  float original_pitch = 0.3f;
  float original_yaw = 0.7f;
  
  Quaternion q = Quaternion::fromEuler(original_roll, original_pitch, original_yaw);
  
  float roll, pitch, yaw;
  q.toEuler(roll, pitch, yaw);
  
  EXPECT_NEAR(roll, original_roll, 0.01f);
  EXPECT_NEAR(pitch, original_pitch, 0.01f);
  EXPECT_NEAR(yaw, original_yaw, 0.01f);
}

TEST_F(QuaternionTest, FromAxisAngleXAxis){
  // 90 degree rotation around X-axis
  float angle = M_PI / 2.0f;
  Quaternion q = Quaternion::fromAxisAngle(1.0f, 0.0f, 0.0f, angle);
  
  EXPECT_NEAR(q.w, cosf(angle / 2.0f), EPSILON);
  EXPECT_NEAR(q.x, sinf(angle / 2.0f), EPSILON);
  EXPECT_NEAR(q.y, 0.0f, EPSILON);
  EXPECT_NEAR(q.z, 0.0f, EPSILON);
}

TEST_F(QuaternionTest, SlerpSameQuaternion){
  Quaternion q1(1.0f, 0.0f, 0.0f, 0.0f);
  Quaternion q2(1.0f, 0.0f, 0.0f, 0.0f);
  
  Quaternion result = Quaternion::slerp(q1, q2, 0.5f);
  
  EXPECT_NEAR(result.w, 1.0f, 0.01f);
  EXPECT_NEAR(result.x, 0.0f, 0.01f);
  EXPECT_NEAR(result.y, 0.0f, 0.01f);
  EXPECT_NEAR(result.z, 0.0f, 0.01f);
}

TEST_F(QuaternionTest, SlerpAtT0ReturnsFirst){
  Quaternion q1 = Quaternion::fromEuler(0.0f, 0.0f, 0.0f);
  Quaternion q2 = Quaternion::fromEuler(1.0f, 1.0f, 1.0f);
  
  Quaternion result = Quaternion::slerp(q1, q2, 0.0f);
  
  EXPECT_NEAR(result.w, q1.w, 0.01f);
  EXPECT_NEAR(result.x, q1.x, 0.01f);
  EXPECT_NEAR(result.y, q1.y, 0.01f);
  EXPECT_NEAR(result.z, q1.z, 0.01f);
}

TEST_F(QuaternionTest, SlerpAtT1ReturnsSecond){
  Quaternion q1 = Quaternion::fromEuler(0.0f, 0.0f, 0.0f);
  Quaternion q2 = Quaternion::fromEuler(1.0f, 1.0f, 1.0f);
  
  Quaternion result = Quaternion::slerp(q1, q2, 1.0f);
  
  EXPECT_NEAR(result.w, q2.w, 0.01f);
  EXPECT_NEAR(result.x, q2.x, 0.01f);
  EXPECT_NEAR(result.y, q2.y, 0.01f);
  EXPECT_NEAR(result.z, q2.z, 0.01f);
}
