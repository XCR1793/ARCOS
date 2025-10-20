#pragma once

/**
 * @file owo_examples.hpp
 * @brief Example usage patterns for the OWO configuration parser
 * 
 * This file contains example functions showing how to use the OWO parser
 * for various common configuration tasks.
 */

#include "config_owo.hpp"
#include <iostream>

namespace arcos::processing::parser::config_owo::examples{

/**
 * @brief Basic parsing example
 */
inline void basicParsingExample(){
  std::cout << "=== Basic OWO Parsing Example ===" << std::endl;
  
  // Create parser
  OwoParser parser;
  
  // Set firmware variables
  parser.setFirmwareVariable("deviceType", "ESP32S3");
  parser.setFirmwareVariable("firmwareVersion", "1.2.3");
  
  // Parse example config
  std::string config = R"(
# version 0.0

[device]
name = "My Device"
type = {deviceType}
version = {firmwareVersion}
enabled = true
max_connections = 10

[network]
ip = "192.168.1.100"
port = 8080
ssid = "MyNetwork"

[network.wifi]
channel = 6
power = 20
encryption = "WPA2"

[sensors]
temperature_enabled = true
humidity_enabled = false
sample_rate = 1000

[sensors.calibration]
temperature_offset = -2.5
humidity_offset = 1.0
coefficients = [1.0, 0.95, 0.02]
)";
  
  if(parser.parseString(config)){
    std::cout << "✓ Config parsed successfully!" << std::endl;
    
    // Access values by path
    auto device_name = parser.getValue("device.name");
    if(device_name && device_name->isString()){
      std::cout << "Device name: " << device_name->asString() << std::endl;
    }
    
    // Access sections
    auto network_section = parser.getSection("network");
    if(network_section){
      std::cout << "Network configuration:" << std::endl;
      auto vars = network_section->getVariableNames();
      for(const auto& var_name : vars){
        auto var = network_section->getVariable(var_name);
        std::cout << "  " << var_name << " = " << var->toString() << std::endl;
      }
    }
    
    // Access arrays
    auto coefficients = parser.getValue("sensors.calibration.coefficients");
    if(coefficients && coefficients->isArray()){
      std::cout << "Calibration coefficients: ";
      const auto& array = coefficients->asArray();
      for(size_t i = 0; i < array.size(); ++i){
        if(i > 0) std::cout << ", ";
        std::cout << array[i]->asNumber();
      }
      std::cout << std::endl;
    }
    
  }else{
    std::cout << "✗ Failed to parse config: " << parser.getLastError() << std::endl;
  }
}

/**
 * @brief Type-safe value extraction example
 */
inline void typeSafeExample(){
  std::cout << "\n=== Type-Safe Value Extraction Example ===" << std::endl;
  
  auto parser = OwoUtils::createTestConfig();
  
  // Extract values with type safety
  std::string title;
  if(OwoUtils::getValueByPath(parser, "item.title", title)){
    std::cout << "Title: " << title << std::endl;
  }
  
  int global_var;
  if(OwoUtils::getValueByPath(parser, "someitem.globalvariable", global_var)){
    std::cout << "Global variable: " << global_var << std::endl;
  }
  
  bool state;
  if(OwoUtils::getValueByPath(parser, "someitem.state", state)){
    std::cout << "State: " << (state ? "enabled" : "disabled") << std::endl;
  }
  
  std::vector<std::string> planets;
  if(OwoUtils::getValueByPath(parser, "item.sub.subitem.planets", planets)){
    std::cout << "Planets: ";
    for(size_t i = 0; i < planets.size(); ++i){
      if(i > 0) std::cout << ", ";
      std::cout << planets[i];
    }
    std::cout << std::endl;
  }
  
  std::vector<double> numbers;
  if(OwoUtils::getValueByPath(parser, "item.sub.subitem.numbers", numbers)){
    std::cout << "Numbers: ";
    for(size_t i = 0; i < numbers.size(); ++i){
      if(i > 0) std::cout << ", ";
      std::cout << numbers[i];
    }
    std::cout << std::endl;
  }
}

/**
 * @brief Configuration validation example
 */
inline void validationExample(){
  std::cout << "\n=== Configuration Validation Example ===" << std::endl;
  
  auto parser = OwoUtils::createTestConfig();
  
  std::vector<std::string> required_paths = {
    "someitem.globalvariable",
    "item.title",
    "item.subitem.ipvariable",
    "item.sub.subitem.numbers"
  };
  
  if(OwoUtils::validateConfig(parser, required_paths)){
    std::cout << "✓ All required configuration paths are present" << std::endl;
  }else{
    std::cout << "✗ Some required configuration paths are missing" << std::endl;
  }
  
  // Test with invalid path
  required_paths.push_back("nonexistent.path");
  
  if(OwoUtils::validateConfig(parser, required_paths)){
    std::cout << "✓ All required configuration paths are present" << std::endl;
  }else{
    std::cout << "✗ Some required configuration paths are missing (expected)" << std::endl;
  }
}

/**
 * @brief Configuration manager example
 */
inline void configManagerExample(){
  std::cout << "\n=== Configuration Manager Example ===" << std::endl;
  
  OwoConfigManager manager;
  
  // Set global firmware variables
  manager.setGlobalFirmwareVariable("device_id", "12345");
  manager.setGlobalFirmwareVariable("firmware_build", "DEBUG");
  
  std::cout << "Configuration manager created with global firmware variables" << std::endl;
  std::cout << "Ready to load .owo files with automatic variable substitution" << std::endl;
}

/**
 * @brief Print configuration tree example
 */
inline void printTreeExample(){
  std::cout << "\n=== Configuration Tree Example ===" << std::endl;
  
  auto parser = OwoUtils::createTestConfig();
  
  std::cout << "Configuration tree:" << std::endl;
  OwoUtils::printConfigTree(parser->getRootSection());
}

/**
 * @brief JSON export example
 */
inline void jsonExportExample(){
  std::cout << "\n=== JSON Export Example ===" << std::endl;
  
  auto parser = OwoUtils::createTestConfig();
  
  std::string json = OwoUtils::toJson(parser->getRootSection());
  std::cout << "Configuration as JSON:" << std::endl;
  std::cout << json << std::endl;
}

/**
 * @brief Run all examples
 */
inline void runAllExamples(){
  basicParsingExample();
  typeSafeExample();
  validationExample();
  configManagerExample();
  printTreeExample();
  jsonExportExample();
  
  std::cout << "\n=== All examples completed ===" << std::endl;
}

} // namespace arcos::processing::parser::config_owo::examples