#pragma once

/**
 * @file config_owo.hpp
 * @brief Main header for OWO Configuration Parser
 * 
 * This library provides a parser for .owo configuration files with support for:
 * - Hierarchical sections using dot notation
 * - Variable scoping (local, scoped, global)
 * - Multiple data types (numbers, strings, booleans, arrays)
 * - Firmware variable substitution
 * - Simple expression evaluation
 * - Comments and version headers
 * 
 * Example usage:
 * 
 * @code
 * #include "processing/parser/config_owo/config_owo.hpp"
 * 
 * using namespace arcos::processing::parser::config_owo;
 * 
 * // Parse a config file
 * OwoParser parser;
 * parser.setFirmwareVariable("deviceId", "ESP32");
 * 
 * if(parser.parseFile("config.owo")){
 *   // Get values
 *   auto section = parser.getSection("item.subitem");
 *   auto value = section->getVariable("ipvariable");
 *   
 *   if(value && value->isString()){
 *     std::string ip = value->asString();
 *     // Use IP address...
 *   }
 * }
 * @endcode
 * 
 * @author ARCOS Development Team
 * @version 0.0
 */

// Core parser functionality
#include "owo_parser.hpp"

// Utility functions and configuration management
#include "owo_utils.hpp"

namespace arcos::processing::parser{
  // Alias for easier access
  namespace owo = config_owo;
} // namespace arcos::processing::parser

// Global convenience aliases
namespace arcos{
  using OwoParser = processing::parser::config_owo::OwoParser;
  using OwoSection = processing::parser::config_owo::OwoSection;
  using OwoValue = processing::parser::config_owo::OwoValue;
  using OwoUtils = processing::parser::config_owo::OwoUtils;
  using OwoConfigManager = processing::parser::config_owo::OwoConfigManager;
} // namespace arcos