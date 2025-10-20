#pragma once

/**
 * @file processing.hpp
 * @brief Main header for ARCOS processing module
 * 
 * This module provides data processing utilities including:
 * - Configuration file parsers
 * - Data format converters
 * - Stream processors
 */

// Configuration parsers
#include "parser/config_owo/config_owo.hpp"

namespace arcos::processing{

/**
 * @brief Processing module version information
 */
constexpr const char* VERSION = "0.1.0";

/**
 * @brief Initialize processing module
 */
inline void init(){
  // Module initialization code if needed
}

} // namespace arcos::processing