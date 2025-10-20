/*****************************************************************
 * File:      bit_ops.hpp
 * Category:  core
 * Author:    ARCOS Team
 * 
 * Purpose:
 *    Bit operations and conversion utilities for ARCOS.
 *    Simple namespace functions for bit manipulation, conversion,
 *    and extraction commonly used in embedded systems and graphics.
 *****************************************************************/

#ifndef ARCOS_CORE_BIT_OPS_HPP_
#define ARCOS_CORE_BIT_OPS_HPP_

#include <cstdint>
#include <type_traits>

namespace arcos{
namespace core{
namespace bit_ops{

// ===========================================
// Bit Depth Conversion Functions
// ===========================================

/** Convert 8-bit value to 5-bit
 * @param value Input 8-bit value (0-255)
 * @return 5-bit value (0-31)
 */
constexpr uint8_t convert8to5(uint8_t value){
  return value >> 3;
}

/** Convert 8-bit value to 6-bit
 * @param value Input 8-bit value (0-255)
 * @return 6-bit value (0-63)
 */
constexpr uint8_t convert8to6(uint8_t value){
  return value >> 2;
}

/** Convert 8-bit value to 4-bit
 * @param value Input 8-bit value (0-255)
 * @return 4-bit value (0-15)
 */
constexpr uint8_t convert8to4(uint8_t value){
  return value >> 4;
}

/** Convert 5-bit back to 8-bit (expand)
 * @param value Input 5-bit value (0-31)
 * @return 8-bit value (0-255)
 */
constexpr uint8_t convert5to8(uint8_t value){
  return (value << 3) | (value >> 2);
}

/** Convert 6-bit back to 8-bit (expand)
 * @param value Input 6-bit value (0-63)
 * @return 8-bit value (0-255)
 */
constexpr uint8_t convert6to8(uint8_t value){
  return (value << 2) | (value >> 4);
}

// ===========================================
// Bit Extraction and Manipulation
// ===========================================

/** Extract specific bit from value (for bit-plane operations)
 * @param value Input value
 * @param bit_plane Which bit to extract (0-7 for uint8_t)
 * @return 0 or 1
 */
template<typename T>
constexpr uint8_t getBitFromValue(T value, int bit_plane){
  return (value >> bit_plane) & 1;
}

/** Extract multiple bits from value
 * @param value Input value  
 * @param start_bit Starting bit position
 * @param num_bits Number of bits to extract
 * @return Extracted bits
 */
template<typename T>
constexpr uint32_t extractBits(T value, int start_bit, int num_bits){
  uint32_t mask = (1U << num_bits) - 1;
  return (value >> start_bit) & mask;
}

/** Extract nibble (4 bits) from byte
 * @param value Input byte
 * @param high_nibble True for high nibble (bits 4-7), false for low (bits 0-3)
 * @return Nibble value (0-15)
 */
constexpr uint8_t extractNibble(uint8_t value, bool high_nibble = false){
  return high_nibble ? (value >> 4) : (value & 0x0F);
}

// ===========================================
// RGB Color Packing Functions  
// ===========================================

/** Pack RGB888 to RGB565 format
 * @param r Red component (0-255)
 * @param g Green component (0-255)  
 * @param b Blue component (0-255)
 * @return Packed RGB565 value
 */
constexpr uint16_t packRGB565(uint8_t r, uint8_t g, uint8_t b){
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/** Unpack RGB565 to RGB888 format
 * @param rgb565 Packed RGB565 value
 * @param r Reference to red component output
 * @param g Reference to green component output  
 * @param b Reference to blue component output
 */
constexpr void unpackRGB565(uint16_t rgb565, uint8_t& r, uint8_t& g, uint8_t& b){
  r = convert5to8((rgb565 >> 11) & 0x1F);
  g = convert6to8((rgb565 >> 5) & 0x3F); 
  b = convert5to8(rgb565 & 0x1F);
}

/** Pack RGB888 to RGB555 format
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255) 
 * @return Packed RGB555 value
 */
constexpr uint16_t packRGB555(uint8_t r, uint8_t g, uint8_t b){
  return ((r & 0xF8) << 7) | ((g & 0xF8) << 2) | (b >> 3);
}

// ===========================================
// Bit Manipulation Functions
// ===========================================

/** Set specific bit in value
 * @param value Reference to value to modify
 * @param bit_position Bit position to set (0-based)
 */
template<typename T>
constexpr void setBit(T& value, int bit_position){
  value |= (T(1) << bit_position);
}

/** Clear specific bit in value  
 * @param value Reference to value to modify
 * @param bit_position Bit position to clear (0-based)
 */
template<typename T>
constexpr void clearBit(T& value, int bit_position){
  value &= ~(T(1) << bit_position);
}

/** Toggle specific bit in value
 * @param value Reference to value to modify
 * @param bit_position Bit position to toggle (0-based)
 */
template<typename T>
constexpr void toggleBit(T& value, int bit_position){
  value ^= (T(1) << bit_position);
}

/** Check if specific bit is set
 * @param value Value to check
 * @param bit_position Bit position to check (0-based)
 * @return true if bit is set, false otherwise
 */
template<typename T>
constexpr bool isBitSet(T value, int bit_position){
  return (value >> bit_position) & 1;
}

/** Swap endianness of 16-bit value
 * @param value 16-bit value to swap
 * @return Value with bytes swapped
 */
constexpr uint16_t swapBytes16(uint16_t value){
  return (value >> 8) | (value << 8);
}

/** Swap endianness of 32-bit value  
 * @param value 32-bit value to swap
 * @return Value with bytes swapped
 */
constexpr uint32_t swapBytes32(uint32_t value){
  return ((value >> 24) & 0x000000FF) |
         ((value >> 8)  & 0x0000FF00) |
         ((value << 8)  & 0x00FF0000) |
         ((value << 24) & 0xFF000000);
}

} // namespace bit_ops
} // namespace core  
} // namespace arcos

#endif // ARCOS_CORE_BIT_OPS_HPP_