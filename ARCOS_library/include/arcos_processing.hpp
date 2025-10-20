/*****************************************************************
 * File:      arcos_processing.hpp
 * Category:  processing
 * Author:    ARCOS Development Team
 * 
 * Purpose:
 *    Processing module header for the ARCOS Hardware Abstraction Framework.
 *    Provides data processing utilities including configuration parsers,
 *    data format converters, and stream processors.
 *    
 *    Key Components:
 *    - OWO Configuration Parser (.owo file format)
 *    - Type-safe configuration management
 *    - Firmware variable substitution
 *    - Hierarchical configuration sections
 *****************************************************************/

#ifndef ARCOS_PROCESSING_HPP_
#define ARCOS_PROCESSING_HPP_

// Include the main processing module
#include "processing/processing.hpp"

/** 
 * @brief ARCOS Processing Module
 * 
 * This header provides access to ARCOS processing functionality:
 * 
 * **Configuration Parsing:**
 * - OWO format parser for hierarchical configurations
 * - Type-safe value extraction with automatic conversion
 * - Variable scoping (local, scoped, global)
 * - Firmware variable substitution
 * - Support for numbers, strings, booleans, arrays
 * 
 * **Data Processing:**
 * - Stream processing utilities (future expansion)
 * - Data format converters (future expansion)
 * - Configuration validation and management
 * 
 * **Usage Examples:**
 * 
 * 1. **Basic OWO Configuration Parsing:**
 * ```cpp
 * #include <arcos_processing.hpp>
 * 
 * using namespace arcos::processing::parser::config_owo;
 * 
 * OwoParser parser;
 * parser.setFirmwareVariable("deviceId", "ESP32S3");
 * 
 * if(parser.parseFile("config.owo")){
 *   std::string device_name;
 *   OwoUtils::getValueByPath(parser, "device.name", device_name);
 * }
 * ```
 * 
 * 2. **Configuration Management:**
 * ```cpp
 * OwoConfigManager manager;
 * manager.setGlobalFirmwareVariable("firmware_version", "1.2.3");
 * auto config = manager.loadConfig("app.owo");
 * ```
 * 
 * 3. **Type-Safe Value Access:**
 * ```cpp
 * // Extract different types safely
 * std::string ip_address;
 * int port_number;
 * std::vector<std::string> server_list;
 * bool enabled;
 * 
 * OwoUtils::getValueByPath(parser, "network.ip", ip_address);
 * OwoUtils::getValueByPath(parser, "network.port", port_number);
 * OwoUtils::getValueByPath(parser, "servers", server_list);
 * OwoUtils::getValueByPath(parser, "network.enabled", enabled);
 * ```
 * 
 * **OWO File Format Example:**
 * ```
 * # version 0.0
 * 
 * [device]
 * name = "ARCOS Device"
 * type = {DEVICE_TYPE}
 * enabled = true
 * 
 * [network]
 * ip = "192.168.1.100"
 * ports = [80, 443, 8080]
 * 
 * [network.wifi]
 * ssid = "MyNetwork"
 * channel = 6
 * ```
 * 
 * @note This is a header-only module - no separate compilation required
 * @version 0.1.0
 * @author ARCOS Development Team
 */
namespace arcos::processing {
  /** Processing module version information */
  constexpr const char* PROCESSING_VERSION = "0.1.0";
  constexpr int PROCESSING_VERSION_MAJOR = 0;
  constexpr int PROCESSING_VERSION_MINOR = 1;
  constexpr int PROCESSING_VERSION_PATCH = 0;
  
  /** Feature availability flags */
  constexpr bool OWO_PARSER_AVAILABLE = true;
  constexpr bool CONFIG_MANAGER_AVAILABLE = true;
  
  /** 
   * @brief Initialize the processing module
   * Call this function before using processing functionality
   */
  inline void initProcessing(){
    init(); // Call the processing module init function
  }
}

// Global convenience aliases for easier access
namespace arcos {
  /** Processing module aliases */
  using OwoParser = processing::parser::config_owo::OwoParser;
  using OwoSection = processing::parser::config_owo::OwoSection;
  using OwoValue = processing::parser::config_owo::OwoValue;
  using OwoUtils = processing::parser::config_owo::OwoUtils;
  using OwoConfigManager = processing::parser::config_owo::OwoConfigManager;
  using VariableScope = processing::parser::config_owo::VariableScope;
  
  /** Processing module version flag */
  constexpr bool PROCESSING_AVAILABLE = true;
}

#endif // ARCOS_PROCESSING_HPP_