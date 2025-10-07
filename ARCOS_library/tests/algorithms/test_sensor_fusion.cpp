/*****************************************************************
 * File:      test_sensor_fusion.cpp
 * Category:  tests/algorithms
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Google Test unit tests for sensor fusion algorithm.
 *****************************************************************/

#include <gtest/gtest.h>
#include "algorithms/fusion/sensor_fusion/sensor_fusion.hpp"

using namespace arcos::algorithms::fusion;

class SensorFusionTest : public ::testing::Test{
protected:
  SensorFusion<float> fusion;
  SensorFusionConfig config;
  
  void SetUp() override{
    config.weight_sensor_a = 0.7f;
    config.weight_sensor_b = 0.3f;
    config.enable_filtering = false;
  }
};

TEST_F(SensorFusionTest, InitializationSuccess){
  EXPECT_TRUE(fusion.init(config));
  EXPECT_TRUE(fusion.isInitialized());
}

TEST_F(SensorFusionTest, WeightedAverageTwoSensors){
  fusion.init(config);
  
  float sensor_a = 10.0f;
  float sensor_b = 20.0f;
  
  float result = fusion.fuse(sensor_a, sensor_b);
  
  // Expected: 0.7 * 10 + 0.3 * 20 = 7 + 6 = 13
  EXPECT_FLOAT_EQ(result, 13.0f);
}

TEST_F(SensorFusionTest, EqualWeights){
  config.weight_sensor_a = 0.5f;
  config.weight_sensor_b = 0.5f;
  fusion.init(config);
  
  float result = fusion.fuse(10.0f, 20.0f);
  
  // Expected: 0.5 * 10 + 0.5 * 20 = 15
  EXPECT_FLOAT_EQ(result, 15.0f);
}

TEST_F(SensorFusionTest, FilteringEnabled){
  config.enable_filtering = true;
  config.filter_alpha = 0.5f;
  fusion.init(config);
  
  // First call - no previous data
  float result1 = fusion.fuse(10.0f, 20.0f);
  EXPECT_FLOAT_EQ(result1, 13.0f); // 0.7*10 + 0.3*20 = 13
  
  // Second call - with filtering
  float result2 = fusion.fuse(10.0f, 20.0f);
  // Expected: 0.5 * 13 + 0.5 * 13 = 13
  EXPECT_FLOAT_EQ(result2, 13.0f);
}

TEST_F(SensorFusionTest, ResetClearsState){
  config.enable_filtering = true;
  fusion.init(config);
  
  fusion.fuse(10.0f, 20.0f);
  fusion.reset();
  
  // After reset, should not have previous data for filtering
  float result = fusion.fuse(10.0f, 20.0f);
  EXPECT_FLOAT_EQ(result, 13.0f);
}

TEST_F(SensorFusionTest, UninitializedReturnsDefault){
  float result = fusion.fuse(10.0f, 20.0f);
  EXPECT_FLOAT_EQ(result, 0.0f);
}
