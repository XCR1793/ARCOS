/*****************************************************************
 * File:      test_bit_ops.cpp
 * Category:  tests/core
 * Author:    ARCOS Team
 * 
 * Purpose:
 *    Google Test unit tests for bit operations and conversions.
 *    Tests all functionality including bit manipulation, extraction,
 *    packing, and color space conversions.
 *****************************************************************/

#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include "core/bit_ops.hpp"
#include "core/bit_ops_convenience.hpp"

using namespace arcos::core::bit_ops;

class BitOpsTest : public ::testing::Test{
protected:
  void SetUp() override{
    // Initialize with default configuration
    bit_ops = std::make_unique<BitOps>();
    
    // Also test without gamma (since we removed gamma correction)
    BitOpsConfig config;
    bit_ops_gamma = std::make_unique<BitOps>(config);
    
    // Initialize convenience functions
    initializeBitOps();
  }

  void TearDown() override{
    bit_ops.reset();
    bit_ops_gamma.reset();
  }

  std::unique_ptr<BitOps> bit_ops;
  std::unique_ptr<BitOps> bit_ops_gamma;
};

// Test bit depth conversions
TEST_F(BitOpsTest, BitDepthConversions){
  // Test 8-bit to 5-bit conversion
  EXPECT_EQ(bit_ops->convert8to5(0), 0);     // 0 >> 3 = 0
  EXPECT_EQ(bit_ops->convert8to5(7), 0);     // 7 >> 3 = 0
  EXPECT_EQ(bit_ops->convert8to5(8), 1);     // 8 >> 3 = 1
  EXPECT_EQ(bit_ops->convert8to5(255), 31);  // 255 >> 3 = 31
    
    // Test 8-bit to 6-bit conversion
    EXPECT_EQ(bit_ops->convert8to6(0), 0);     // 0 >> 2 = 0
    EXPECT_EQ(bit_ops->convert8to6(3), 0);     // 3 >> 2 = 0
    EXPECT_EQ(bit_ops->convert8to6(4), 1);     // 4 >> 2 = 1
    EXPECT_EQ(bit_ops->convert8to6(255), 63);  // 255 >> 2 = 63
    
    // Test 8-bit to 4-bit conversion
    EXPECT_EQ(bit_ops->convert8to4(0), 0);     // 0 >> 4 = 0
    EXPECT_EQ(bit_ops->convert8to4(15), 0);    // 15 >> 4 = 0
    EXPECT_EQ(bit_ops->convert8to4(16), 1);    // 16 >> 4 = 1
    EXPECT_EQ(bit_ops->convert8to4(255), 15);  // 255 >> 4 = 15
}

// Test bit depth expansion
TEST_F(BitOpsTest, BitDepthExpansion) {
    // Test 5-bit to 8-bit expansion
    EXPECT_EQ(bit_ops->convert5to8(0), 0);
    EXPECT_EQ(bit_ops->convert5to8(31), 255);
    
    // Test that round-trip conversion preserves some precision
    for(int i = 0; i <= 31; ++i) {
        uint8_t expanded = bit_ops->convert5to8(i);
        uint8_t back = bit_ops->convert8to5(expanded);
        EXPECT_EQ(back, i) << "Round-trip failed for " << i;
    }
    
    // Test 6-bit to 8-bit expansion
    EXPECT_EQ(bit_ops->convert6to8(0), 0);
    EXPECT_EQ(bit_ops->convert6to8(63), 255);
}

// Test generic bit depth conversion
TEST_F(BitOpsTest, GenericBitDepthConversion) {
    // Test 8-bit to 5-bit
    EXPECT_EQ(bit_ops->convertBitDepth<8, 5>(255), 31);
    EXPECT_EQ(bit_ops->convertBitDepth<8, 5>(0), 0);
    
    // Test 5-bit to 8-bit
    EXPECT_EQ(bit_ops->convertBitDepth<5, 8>(31), 255);
    EXPECT_EQ(bit_ops->convertBitDepth<5, 8>(0), 0);
    
    // Test same bit depth (no conversion)
    EXPECT_EQ(bit_ops->convertBitDepth<8, 8>(123), 123);
}

// Test bit extraction functions
TEST_F(BitOpsTest, BitExtraction) {
    // Test getBitFromValue
    uint8_t test_value = 0b10110101; // 181
    
    EXPECT_EQ(bit_ops->getBitFromValue(test_value, 0), 1); // LSB
    EXPECT_EQ(bit_ops->getBitFromValue(test_value, 1), 0);
    EXPECT_EQ(bit_ops->getBitFromValue(test_value, 2), 1);
    EXPECT_EQ(bit_ops->getBitFromValue(test_value, 3), 0);
    EXPECT_EQ(bit_ops->getBitFromValue(test_value, 4), 1);
    EXPECT_EQ(bit_ops->getBitFromValue(test_value, 5), 1);
    EXPECT_EQ(bit_ops->getBitFromValue(test_value, 6), 0);
    EXPECT_EQ(bit_ops->getBitFromValue(test_value, 7), 1); // MSB
    
    // Test extractBits
    EXPECT_EQ(bit_ops->extractBits(test_value, 2, 3), 0b101); // Bits 2-4
    EXPECT_EQ(bit_ops->extractBits(test_value, 0, 4), 0b0101); // Bits 0-3
    
    // Test extractNibble
    uint8_t test_byte = 0xAB; // 10101011
    EXPECT_EQ(bit_ops->extractNibble(test_byte, false), 0x0B); // Low nibble
    EXPECT_EQ(bit_ops->extractNibble(test_byte, true), 0x0A);  // High nibble
}

// Test RGB packing/unpacking
TEST_F(BitOpsTest, RGBPacking) {
    // Test RGB565 packing
    uint16_t rgb565 = bit_ops->packRGB565(31, 63, 31); // Max values
    EXPECT_EQ(rgb565, 0xFFFF);
    
    rgb565 = bit_ops->packRGB565(0, 0, 0); // Min values
    EXPECT_EQ(rgb565, 0x0000);
    
    // Test RGB565 unpacking
    uint8_t r, g, b;
    bit_ops->unpackRGB565(0xFFFF, r, g, b);
    EXPECT_EQ(r, 31);
    EXPECT_EQ(g, 63);
    EXPECT_EQ(b, 31);
    
    bit_ops->unpackRGB565(0x0000, r, g, b);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(g, 0);
    EXPECT_EQ(b, 0);
    
    // Test RGB555 packing
    uint16_t rgb555 = bit_ops->packRGB555(31, 31, 31);
    EXPECT_EQ(rgb555, 0x7FFF); // No MSB in RGB555
}

// Test bit manipulation functions
TEST_F(BitOpsTest, BitManipulation) {
    uint8_t test_value = 0;
    
    // Test setBit
    bit_ops->setBit(test_value, 3);
    EXPECT_EQ(test_value, 0b00001000);
    
    bit_ops->setBit(test_value, 0);
    EXPECT_EQ(test_value, 0b00001001);
    
    // Test isBitSet
    EXPECT_TRUE(bit_ops->isBitSet(test_value, 3));
    EXPECT_TRUE(bit_ops->isBitSet(test_value, 0));
    EXPECT_FALSE(bit_ops->isBitSet(test_value, 1));
    
    // Test clearBit
    bit_ops->clearBit(test_value, 3);
    EXPECT_EQ(test_value, 0b00000001);
    EXPECT_FALSE(bit_ops->isBitSet(test_value, 3));
    
    // Test toggleBit
    bit_ops->toggleBit(test_value, 1);
    EXPECT_EQ(test_value, 0b00000011);
    bit_ops->toggleBit(test_value, 1);
    EXPECT_EQ(test_value, 0b00000001);
}

// Test bit counting functions
TEST_F(BitOpsTest, BitCounting) {
    // Test popcount (population count)
    EXPECT_EQ(bit_ops->popcount(0b00000000), 0);
    EXPECT_EQ(bit_ops->popcount(0b11111111), 8);
    EXPECT_EQ(bit_ops->popcount(0b10101010), 4);
    EXPECT_EQ(bit_ops->popcount(0b11100111), 6);
    
    // Test countLeadingZeros
    EXPECT_EQ(bit_ops->countLeadingZeros(static_cast<uint8_t>(0b00000001)), 7);
    EXPECT_EQ(bit_ops->countLeadingZeros(static_cast<uint8_t>(0b10000000)), 0);
    EXPECT_EQ(bit_ops->countLeadingZeros(static_cast<uint8_t>(0b00100000)), 2);
    
    // Test countTrailingZeros
    EXPECT_EQ(bit_ops->countTrailingZeros(static_cast<uint8_t>(0b10000000)), 7);
    EXPECT_EQ(bit_ops->countTrailingZeros(static_cast<uint8_t>(0b00000001)), 0);
    EXPECT_EQ(bit_ops->countTrailingZeros(static_cast<uint8_t>(0b00000100)), 2);
}

// Test bit rotation functions
TEST_F(BitOpsTest, BitRotation) {
    uint8_t test_value = 0b10000001;
    
    // Test rotate left
    uint8_t rotated_left = bit_ops->rotateLeft(test_value, 1);
    EXPECT_EQ(rotated_left, 0b00000011);
    
    rotated_left = bit_ops->rotateLeft(test_value, 4);
    EXPECT_EQ(rotated_left, 0b00011000);
    
    // Test rotate right
    uint8_t rotated_right = bit_ops->rotateRight(test_value, 1);
    EXPECT_EQ(rotated_right, 0b11000000);
    
    rotated_right = bit_ops->rotateRight(test_value, 4);
    EXPECT_EQ(rotated_right, 0b00011000);
}

// Test endianness functions
TEST_F(BitOpsTest, Endianness) {
    // Test 16-bit byte swap
    uint16_t test16 = 0x1234;
    uint16_t swapped16 = bit_ops->byteSwap(test16);
    EXPECT_EQ(swapped16, 0x3412);
    
    // Test 32-bit byte swap
    uint32_t test32 = 0x12345678;
    uint32_t swapped32 = bit_ops->byteSwap(test32);
    EXPECT_EQ(swapped32, 0x78563412);
    
    // Test double swap returns original
    EXPECT_EQ(bit_ops->byteSwap(swapped16), test16);
    EXPECT_EQ(bit_ops->byteSwap(swapped32), test32);
}

// Test configuration
TEST_F(BitOpsTest, Configuration) {
    // Test that configuration is preserved
    BitOpsConfig config;
    config.use_lookup_tables = false;
    BitOps configured_bit_ops(config);
    
    // Test basic conversion still works
    EXPECT_EQ(configured_bit_ops.convert8to5(255), 31);
    EXPECT_EQ(configured_bit_ops.convert8to5(0), 0);
    
    // Both instances should produce same results for pure bitwise operations
    EXPECT_EQ(bit_ops->convert8to5(128), bit_ops_gamma->convert8to5(128));
}

// Test convenience functions
TEST_F(BitOpsTest, ConvenienceFunctions) {
    // Test global convenience functions
    EXPECT_EQ(convert8to5(255), 31);
    EXPECT_EQ(convert8to6(255), 63);
    
    uint8_t test_val = 0b10110;
    EXPECT_EQ(getBitFromValue(test_val, 1), 1);
    EXPECT_EQ(getBitFromValue(test_val, 3), 0);
    
    // Test RGB565 convenience
    uint16_t packed = packRGB565(31, 63, 31);
    EXPECT_EQ(packed, 0xFFFF);
    
    uint8_t r, g, b;
    unpackRGB565(packed, r, g, b);
    EXPECT_EQ(r, 31);
    EXPECT_EQ(g, 63);
    EXPECT_EQ(b, 31);
}

// Test specialized bit plane functions
TEST_F(BitOpsTest, BitPlaneFunctions) {
    // Test RGB to bit plane conversion
    uint8_t rgb_data[] = {255, 128, 64, 192, 96, 32}; // 2 RGB pixels
    uint8_t r_output[2], g_output[2], b_output[2];
    
    rgbToBitPlane(rgb_data, 2, 4, r_output, g_output, b_output); // Bit plane 4 (MSB of 5-bit)
    
    // Verify bit extraction for MSB plane
    EXPECT_EQ(r_output[0], 1); // 255 -> 31 -> bit 4 = 1
    EXPECT_EQ(r_output[1], 1); // 192 -> 24 -> bit 4 = 1
    EXPECT_EQ(g_output[0], 1); // 128 -> 32 -> bit 4 = 1 (6-bit conversion)
    EXPECT_EQ(b_output[0], 1); // 64 -> 8 -> bit 4 = 0
}

// Test color space conversions
TEST_F(BitOpsTest, ColorSpaceConversions) {
    // Test 24-bit RGB to RGB565
    uint16_t rgb565 = rgb24ToRgb565(255, 255, 255);
    EXPECT_EQ(rgb565, 0xFFFF);
    
    // Test RGB565 to 24-bit RGB
    uint8_t r, g, b;
    rgb565ToRgb24(0xFFFF, r, g, b);
    
    // Should be close to original (some precision loss expected)
    EXPECT_GE(r, 248); // Should be very close to 255
    EXPECT_GE(g, 252); // Green has better precision in RGB565
    EXPECT_GE(b, 248); // Should be very close to 255
    
    // Test with black
    rgb565ToRgb24(0x0000, r, g, b);
    EXPECT_EQ(r, 0);
    EXPECT_EQ(g, 0);
    EXPECT_EQ(b, 0);
}

// Performance test for bit operations
TEST_F(BitOpsTest, Performance) {
    const int NUM_ITERATIONS = 100000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    volatile uint8_t result = 0; // Prevent optimization
    for(int i = 0; i < NUM_ITERATIONS; ++i) {
        uint8_t value = static_cast<uint8_t>(i & 0xFF);
        result += convert8to5(value);
        result += getBitFromValue(value, 3);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Bit operations performance (" << NUM_ITERATIONS << " iterations): " 
              << duration.count() << " μs" << std::endl;
    
    // Should complete in reasonable time (adjust threshold as needed)
    EXPECT_LT(duration.count(), 50000); // Less than 50ms
}

int main(int argc, char **argv){
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}