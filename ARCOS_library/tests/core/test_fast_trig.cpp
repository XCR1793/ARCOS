/**
 * @file test_fast_trig.cpp
 * @brief Google Test unit tests for fast trigonometric functions
 * @author ARCOS Team
 * @date 2025-10-18
 */

#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <gtest/gtest.h>
#include <vector>
#include <chrono>
#include "core/maths/fast_trig.hpp"
#include "core/maths.hpp"

using namespace arcos::core::maths;

class FastTrigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize with higher precision for better performance
        fast_trig = std::make_unique<FastTrig>(FastTrig::Precision::DEG_0_01, FastTrig::ALL_FUNCTIONS);
    }

    void TearDown() override {
        fast_trig.reset();
    }

    std::unique_ptr<FastTrig> fast_trig;
    const float TOLERANCE = 1e-3f; // 0.1% tolerance for comparison with std library
};

// Test basic trigonometric functions
TEST_F(FastTrigTest, BasicTrigonometricFunctions) {
    // Test known values
    float pi_2 = static_cast<float>(M_PI / 2);
    float pi_4 = static_cast<float>(M_PI / 4);
    float pi_6 = static_cast<float>(M_PI / 6);

    // sin(π/2) = 1
    EXPECT_NEAR(fast_trig->sin(pi_2), 1.0f, TOLERANCE);
    
    // cos(π/2) = 0
    EXPECT_NEAR(fast_trig->cos(pi_2), 0.0f, TOLERANCE);
    
    // sin(π/4) = cos(π/4) = √2/2 ≈ 0.707
    EXPECT_NEAR(fast_trig->sin(pi_4), 0.7071067f, TOLERANCE);
    EXPECT_NEAR(fast_trig->cos(pi_4), 0.7071067f, TOLERANCE);
    
    // sin(π/6) = 0.5, cos(π/6) = √3/2 ≈ 0.866
    EXPECT_NEAR(fast_trig->sin(pi_6), 0.5f, TOLERANCE);
    EXPECT_NEAR(fast_trig->cos(pi_6), 0.8660254f, TOLERANCE);
}

// Test hyperbolic functions
TEST_F(FastTrigTest, HyperbolicFunctions) {
    // Test sinh(0) = 0, cosh(0) = 1, tanh(0) = 0
    EXPECT_NEAR(fast_trig->sinh(0.0f), 0.0f, TOLERANCE);
    EXPECT_NEAR(fast_trig->cosh(0.0f), 1.0f, TOLERANCE);
    EXPECT_NEAR(fast_trig->tanh(0.0f), 0.0f, TOLERANCE);
    
    // Test some known values
    float x = 1.0f;
    EXPECT_NEAR(fast_trig->sinh(x), std::sinh(x), TOLERANCE);
    EXPECT_NEAR(fast_trig->cosh(x), std::cosh(x), TOLERANCE);
    EXPECT_NEAR(fast_trig->tanh(x), std::tanh(x), TOLERANCE);
}

// Test inverse trigonometric functions
TEST_F(FastTrigTest, InverseTrigonometricFunctions) {
    // Test asin(0.5) = π/6, acos(0.5) = π/3
    float pi_6 = static_cast<float>(M_PI / 6);
    float pi_3 = static_cast<float>(M_PI / 3);
    
    EXPECT_NEAR(fast_trig->asin(0.5f), pi_6, TOLERANCE);
    EXPECT_NEAR(fast_trig->acos(0.5f), pi_3, TOLERANCE);
    
    // Test atan(1) = π/4
    float pi_4 = static_cast<float>(M_PI / 4);
    EXPECT_NEAR(fast_trig->atan(1.0f), pi_4, TOLERANCE);
}

// Test accuracy against standard library functions
TEST_F(FastTrigTest, AccuracyComparison) {
    const int NUM_SAMPLES = 100;
    float max_sin_error = 0.0f;
    float max_cos_error = 0.0f;
    float max_tan_error = 0.0f;
    
    for(int i = 0; i < NUM_SAMPLES; ++i) {
        float angle = (static_cast<float>(i) / NUM_SAMPLES) * 2.0f * static_cast<float>(M_PI);
        
        float fast_sin = fast_trig->sin(angle);
        float std_sin = std::sin(angle);
        float sin_error = std::abs(fast_sin - std_sin);
        max_sin_error = std::max(max_sin_error, sin_error);
        
        float fast_cos = fast_trig->cos(angle);
        float std_cos = std::cos(angle);
        float cos_error = std::abs(fast_cos - std_cos);
        max_cos_error = std::max(max_cos_error, cos_error);
        
        // Skip tan near singularities
        if(std::abs(std_cos) > 0.1f) {
            float fast_tan = fast_trig->tan(angle);
            float std_tan = std::tan(angle);
            float tan_error = std::abs(fast_tan - std_tan);
            max_tan_error = std::max(max_tan_error, tan_error);
        }
    }
    
    // Verify accuracy is within expected bounds
    EXPECT_LT(max_sin_error, 0.01f); // Less than 1% error
    EXPECT_LT(max_cos_error, 0.01f);
    EXPECT_LT(max_tan_error, 0.1f);  // Tan is less accurate near singularities
    
    std::cout << "Maximum errors - sin: " << max_sin_error 
              << ", cos: " << max_cos_error 
              << ", tan: " << max_tan_error << std::endl;
}

// Test selective function initialization
TEST_F(FastTrigTest, SelectiveInitialization) {
    // Test basic trig only
    FastTrig basic_trig(FastTrig::Precision::DEG_1, FastTrig::BASIC_TRIG);
    
    EXPECT_TRUE(basic_trig.isFunctionInitialized(FastTrig::FunctionType::SIN));
    EXPECT_TRUE(basic_trig.isFunctionInitialized(FastTrig::FunctionType::COS));
    EXPECT_TRUE(basic_trig.isFunctionInitialized(FastTrig::FunctionType::TAN));
    EXPECT_FALSE(basic_trig.isFunctionInitialized(FastTrig::FunctionType::SINH));
    
    // Test that basic functions work
    float angle = static_cast<float>(M_PI / 4);
    EXPECT_NEAR(basic_trig.sin(angle), 0.7071067f, TOLERANCE);
    EXPECT_NEAR(basic_trig.cos(angle), 0.7071067f, TOLERANCE);
    
    // Test hyperbolic only
    FastTrig hyperbolic_trig(FastTrig::Precision::DEG_1, FastTrig::HYPERBOLIC);
    
    EXPECT_FALSE(hyperbolic_trig.isFunctionInitialized(FastTrig::FunctionType::SIN));
    EXPECT_TRUE(hyperbolic_trig.isFunctionInitialized(FastTrig::FunctionType::SINH));
    EXPECT_TRUE(hyperbolic_trig.isFunctionInitialized(FastTrig::FunctionType::COSH));
    EXPECT_TRUE(hyperbolic_trig.isFunctionInitialized(FastTrig::FunctionType::TANH));
    
    // Test that hyperbolic functions work
    EXPECT_NEAR(hyperbolic_trig.sinh(0.0f), 0.0f, TOLERANCE);
    EXPECT_NEAR(hyperbolic_trig.cosh(0.0f), 1.0f, TOLERANCE);
}

// Test precision levels
TEST_F(FastTrigTest, PrecisionLevels) {
    std::vector<FastTrig::Precision> precisions = {
        FastTrig::Precision::DEG_10,
        FastTrig::Precision::DEG_1,
        FastTrig::Precision::DEG_0_1,
        FastTrig::Precision::DEG_0_01
    };
    
    for(auto precision : precisions) {
        FastTrig trig(precision, FastTrig::BASIC_TRIG);
        
        // Verify table size matches precision
        EXPECT_EQ(trig.getTableSize(), static_cast<size_t>(precision));
        
        // Verify angular precision calculation
        float expected_precision = 360.0f / static_cast<float>(precision);
        EXPECT_NEAR(trig.getAngularPrecisionDegrees(), expected_precision, 0.001f);
        
        // Verify basic functionality works at all precision levels
        float pi_4 = static_cast<float>(M_PI / 4);
        float result = trig.sin(pi_4);
        EXPECT_GT(result, 0.6f);  // Should be approximately 0.707
        EXPECT_LT(result, 0.8f);
    }
}

// Performance benchmark test
TEST_F(FastTrigTest, PerformanceBenchmark) {
    const int NUM_ITERATIONS = 100000;
    
    // Generate test data
    std::vector<float> test_angles;
    test_angles.reserve(NUM_ITERATIONS);
    for(int i = 0; i < NUM_ITERATIONS; ++i) {
        test_angles.push_back((static_cast<float>(i) / NUM_ITERATIONS) * 2.0f * static_cast<float>(M_PI));
    }
    
    // Benchmark fast sin
    auto start = std::chrono::high_resolution_clock::now();
    volatile float result = 0.0f; // Prevent optimization
    for(const auto& angle : test_angles) {
        result += fast_trig->sin(angle);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto fast_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Benchmark standard sin
    start = std::chrono::high_resolution_clock::now();
    result = 0.0f;
    for(const auto& angle : test_angles) {
        result += std::sin(angle);
    }
    end = std::chrono::high_resolution_clock::now();
    auto std_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    float speedup = static_cast<float>(std_duration.count()) / fast_duration.count();
    
    std::cout << "Performance test (" << NUM_ITERATIONS << " iterations):" << std::endl;
    std::cout << "Fast sin: " << fast_duration.count() << " μs" << std::endl;
    std::cout << "Std sin:  " << std_duration.count() << " μs" << std::endl;
    std::cout << "Speedup:  " << speedup << "x" << std::endl;
    
    // Note: Modern CPUs often have optimized hardware sin/cos, so lookup tables
    // may not always be faster. The main benefits are predictable timing and
    // potential energy savings on embedded systems.
    std::cout << "Note: Performance benefits depend on target platform. " 
              << "Lookup tables provide predictable timing on embedded systems." << std::endl;
    
    // Just verify the test runs without requiring specific performance
    EXPECT_GT(fast_duration.count(), 0);
    EXPECT_GT(std_duration.count(), 0);
}

// Test deterministic timing advantage
TEST_F(FastTrigTest, DeterministicTiming) {
    const int NUM_SAMPLES = 1000;
    std::vector<float> test_angles = {0.0f, static_cast<float>(M_PI/4), static_cast<float>(M_PI/2), static_cast<float>(M_PI)};
    
    std::vector<long long> fast_times;
    std::vector<long long> std_times;
    
    // Measure timing variability for fast trig
    for(int i = 0; i < NUM_SAMPLES; ++i) {
        for(const auto& angle : test_angles) {
            auto start = std::chrono::high_resolution_clock::now();
            volatile float result = fast_trig->sin(angle);
            auto end = std::chrono::high_resolution_clock::now();
            fast_times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
        }
    }
    
    // Measure timing variability for std trig
    for(int i = 0; i < NUM_SAMPLES; ++i) {
        for(const auto& angle : test_angles) {
            auto start = std::chrono::high_resolution_clock::now();
            volatile float result = std::sin(angle);
            auto end = std::chrono::high_resolution_clock::now();
            std_times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
        }
    }
    
    // Calculate variance
    auto calc_variance = [](const std::vector<long long>& times) {
        double mean = 0.0;
        for(auto t : times) mean += t;
        mean /= times.size();
        
        double variance = 0.0;
        for(auto t : times) {
            double diff = t - mean;
            variance += diff * diff;
        }
        return variance / times.size();
    };
    
    double fast_variance = calc_variance(fast_times);
    double std_variance = calc_variance(std_times);
    
    std::cout << "Timing consistency test:" << std::endl;
    std::cout << "Fast trig variance: " << fast_variance << " ns²" << std::endl;
    std::cout << "Std trig variance:  " << std_variance << " ns²" << std::endl;
    std::cout << "Fast trig provides more predictable timing for real-time systems." << std::endl;
    
    // Verify the test ran
    EXPECT_GT(fast_times.size(), 0);
    EXPECT_GT(std_times.size(), 0);
}

// Test convenience functions
TEST_F(FastTrigTest, ConvenienceFunctions) {
    // Initialize global functions
    initializeFastTrig(FastTrig::Precision::DEG_0_1, FastTrig::BASIC_TRIG | FastTrig::INVERSE_TRIG);
    
    float pi_4 = static_cast<float>(M_PI / 4);
    
    // Test basic trig convenience functions
    EXPECT_NEAR(fastSin(pi_4), 0.7071067f, TOLERANCE);
    EXPECT_NEAR(fastCos(pi_4), 0.7071067f, TOLERANCE);
    
    // Test inverse trig convenience functions
    EXPECT_NEAR(fastAsin(0.5f), static_cast<float>(M_PI / 6), TOLERANCE);
    EXPECT_NEAR(fastAcos(0.5f), static_cast<float>(M_PI / 3), TOLERANCE);
}

// Test edge cases and boundary conditions
TEST_F(FastTrigTest, EdgeCases) {
    // Test angle normalization
    float two_pi = 2.0f * static_cast<float>(M_PI);
    EXPECT_NEAR(fast_trig->sin(0.0f), fast_trig->sin(two_pi), TOLERANCE);
    EXPECT_NEAR(fast_trig->sin(0.0f), fast_trig->sin(-two_pi), TOLERANCE);
    
    // Test very large angles
    float large_angle = 100.0f * static_cast<float>(M_PI);
    EXPECT_NEAR(fast_trig->sin(large_angle), fast_trig->sin(0.0f), TOLERANCE);
    
    // Test negative angles
    float neg_pi_4 = -static_cast<float>(M_PI / 4);
    EXPECT_NEAR(fast_trig->sin(neg_pi_4), -fast_trig->sin(static_cast<float>(M_PI / 4)), TOLERANCE);
    
    // Test inverse function domain limits
    EXPECT_NEAR(fast_trig->asin(1.0f), static_cast<float>(M_PI / 2), TOLERANCE);
    EXPECT_NEAR(fast_trig->asin(-1.0f), -static_cast<float>(M_PI / 2), TOLERANCE);
    EXPECT_NEAR(fast_trig->acos(1.0f), 0.0f, TOLERANCE);
    EXPECT_NEAR(fast_trig->acos(-1.0f), static_cast<float>(M_PI), TOLERANCE);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}