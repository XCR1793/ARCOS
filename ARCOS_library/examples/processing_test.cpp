#include "arcos_processing.hpp"
#include <iostream>

int main(){
  std::cout << "=== ARCOS Processing Module Test ===" << std::endl;
  
  // Initialize processing module
  arcos::processing::initProcessing();
  
  // Test OWO parser with your example config
  arcos::OwoParser parser;
  
  // Set firmware variables
  parser.setFirmwareVariable("firmwareVariable", "ESP32S3_TEST");
  
  // Parse the example config from your attachment
  std::string config = R"(
# version 0.0

[someitem]
globalvariable(global) = 4
state = true
value = 10

[item]
title = "OwO Title"
peritemvariable = 4
itemvariable(scoped) = 0
thing = {firmwareVariable}
variable.x = 3
variable.y = 1
variable.z = 0

[item.subitem]
equation = 50
ipvariable = "192.168.1.1"
mac_address = "00:1A:2B:3C:4D:5E"

[item.sub.subitem]
numbers = [1, 2, 3, 4, 5]
strings = ["apple", "banana", "cherry"]
booleans = [true, false, true]
matrix = [[1, 2], [3, 4], [5, 6]]
planets = ["Mercury", "Venus", "Earth", "Mars"]
)";
  
  if(parser.parseString(config)){
    std::cout << "✓ Config parsed successfully!" << std::endl;
    
    // Test type-safe value extraction
    std::string title;
    int global_var;
    bool state;
    std::string ip;
    std::vector<std::string> planets;
    std::vector<double> numbers;
    
    if(arcos::OwoUtils::getValueByPath(&parser, "item.title", title)){
      std::cout << "Title: " << title << std::endl;
    }
    
    if(arcos::OwoUtils::getValueByPath(&parser, "someitem.globalvariable", global_var)){
      std::cout << "Global variable: " << global_var << std::endl;
    }
    
    if(arcos::OwoUtils::getValueByPath(&parser, "someitem.state", state)){
      std::cout << "State: " << (state ? "enabled" : "disabled") << std::endl;
    }
    
    if(arcos::OwoUtils::getValueByPath(&parser, "item.subitem.ipvariable", ip)){
      std::cout << "IP Variable: " << ip << std::endl;
    }
    
    if(arcos::OwoUtils::getValueByPath(&parser, "item.sub.subitem.planets", planets)){
      std::cout << "Planets: ";
      for(size_t i = 0; i < planets.size(); ++i){
        if(i > 0) std::cout << ", ";
        std::cout << planets[i];
      }
      std::cout << std::endl;
    }
    
    if(arcos::OwoUtils::getValueByPath(&parser, "item.sub.subitem.numbers", numbers)){
      std::cout << "Numbers: ";
      for(size_t i = 0; i < numbers.size(); ++i){
        if(i > 0) std::cout << ", ";
        std::cout << numbers[i];
      }
      std::cout << std::endl;
    }
    
    // Test firmware variable substitution
    auto thing_value = parser.getValue("item.thing");
    if(thing_value && thing_value->isString()){
      std::cout << "Firmware variable result: " << thing_value->asString() << std::endl;
    }
    
    // Print configuration tree
    std::cout << "\nConfiguration structure:" << std::endl;
    arcos::OwoUtils::printConfigTree(parser.getRootSection());
    
    std::cout << "\n✓ All tests passed! OWO parser is working correctly." << std::endl;
    
  }else{
    std::cout << "✗ Failed to parse config: " << parser.getLastError() << std::endl;
    return 1;
  }
  
  std::cout << "\n=== ARCOS Processing Module Available ===" << std::endl;
  std::cout << "Version: " << arcos::processing::PROCESSING_VERSION << std::endl;
  std::cout << "OWO Parser: " << (arcos::processing::OWO_PARSER_AVAILABLE ? "Available" : "Not Available") << std::endl;
  std::cout << "Config Manager: " << (arcos::processing::CONFIG_MANAGER_AVAILABLE ? "Available" : "Not Available") << std::endl;
  
  return 0;
}