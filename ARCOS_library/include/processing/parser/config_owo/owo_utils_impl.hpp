#pragma once

#include <iostream>
#include <iomanip>

namespace arcos::processing::parser::config_owo{

// Template specializations for getValue
template<>
inline bool OwoUtils::getValue<double>(std::shared_ptr<OwoValue> value, double& result){
  if(!value || !value->isNumber()){
    return false;
  }
  result = value->asNumber();
  return true;
}

template<>
inline bool OwoUtils::getValue<float>(std::shared_ptr<OwoValue> value, float& result){
  if(!value || !value->isNumber()){
    return false;
  }
  result = static_cast<float>(value->asNumber());
  return true;
}

template<>
inline bool OwoUtils::getValue<int>(std::shared_ptr<OwoValue> value, int& result){
  if(!value || !value->isNumber()){
    return false;
  }
  result = static_cast<int>(value->asNumber());
  return true;
}

template<>
inline bool OwoUtils::getValue<bool>(std::shared_ptr<OwoValue> value, bool& result){
  if(!value || !value->isBool()){
    return false;
  }
  result = value->asBool();
  return true;
}

template<>
inline bool OwoUtils::getValue<std::string>(std::shared_ptr<OwoValue> value, std::string& result){
  if(!value || !value->isString()){
    return false;
  }
  result = value->asString();
  return true;
}

template<>
inline bool OwoUtils::getValue<std::vector<double>>(std::shared_ptr<OwoValue> value, std::vector<double>& result){
  if(!value || !value->isArray()){
    return false;
  }
  
  const auto& array = value->asArray();
  result.clear();
  result.reserve(array.size());
  
  for(const auto& element : array){
    if(!element->isNumber()){
      return false;
    }
    result.push_back(element->asNumber());
  }
  
  return true;
}

template<>
inline bool OwoUtils::getValue<std::vector<std::string>>(std::shared_ptr<OwoValue> value, std::vector<std::string>& result){
  if(!value || !value->isArray()){
    return false;
  }
  
  const auto& array = value->asArray();
  result.clear();
  result.reserve(array.size());
  
  for(const auto& element : array){
    if(!element->isString()){
      return false;
    }
    result.push_back(element->asString());
  }
  
  return true;
}

// Generic getValueByPath implementation
template<typename T>
inline bool OwoUtils::getValueByPath(std::shared_ptr<OwoParser> parser, const std::string& path, T& result){
  if(!parser){
    return false;
  }
  
  auto value = parser->getValue(path);
  return getValue(value, result);
}

inline void OwoUtils::printConfigTree(std::shared_ptr<OwoSection> section, int indent){
  if(!section){
    return;
  }
  
  printSection(section, indent);
}

inline void OwoUtils::printSection(std::shared_ptr<OwoSection> section, int indent){
  std::string indentStr(indent * 2, ' ');
  
  if(!section->getName().empty()){
    std::cout << indentStr << "[" << section->getName() << "]" << std::endl;
  }
  
  // Print variables
  const auto& variables = section->getVariables();
  for(const auto& pair : variables){
    std::cout << indentStr << "  " << pair.first << " = " << pair.second->toString() << std::endl;
  }
  
  // Print sub-sections
  const auto& subSections = section->getSubSections();
  for(const auto& pair : subSections){
    printSection(pair.second, indent + 1);
  }
}

inline bool OwoUtils::validateConfig(std::shared_ptr<OwoParser> parser, const std::vector<std::string>& required_paths){
  if(!parser){
    return false;
  }
  
  for(const std::string& path : required_paths){
    auto value = parser->getValue(path);
    if(!value){
      return false;
    }
  }
  
  return true;
}

inline std::string OwoUtils::toJson(std::shared_ptr<OwoSection> section){
  if(!section){
    return "{}";
  }
  
  return sectionToJson(section);
}

inline std::string OwoUtils::sectionToJson(std::shared_ptr<OwoSection> section, int indent){
  std::string indentStr(indent * 2, ' ');
  std::string result = "{\n";
  
  // Add variables
  const auto& variables = section->getVariables();
  bool first = true;
  
  for(const auto& pair : variables){
    if(!first){
      result += ",\n";
    }
    first = false;
    
    result += indentStr + "  \"" + pair.first + "\": " + valueToJson(pair.second);
  }
  
  // Add sub-sections
  const auto& subSections = section->getSubSections();
  for(const auto& pair : subSections){
    if(!first){
      result += ",\n";
    }
    first = false;
    
    result += indentStr + "  \"" + pair.first + "\": " + sectionToJson(pair.second, indent + 1);
  }
  
  result += "\n" + indentStr + "}";
  return result;
}

inline std::string OwoUtils::valueToJson(std::shared_ptr<OwoValue> value){
  if(!value){
    return "null";
  }
  
  switch(value->getType()){
    case OwoValue::Type::Number:
      return std::to_string(value->asNumber());
      
    case OwoValue::Type::String:
      return "\"" + value->asString() + "\"";
      
    case OwoValue::Type::Bool:
      return value->asBool() ? "true" : "false";
      
    case OwoValue::Type::VariableRef:
      return "\"${" + value->asVariableRef() + "}\"";
      
    case OwoValue::Type::Array:
      {
        std::string result = "[";
        const auto& array = value->asArray();
        
        for(size_t i = 0; i < array.size(); ++i){
          if(i > 0){
            result += ", ";
          }
          result += valueToJson(array[i]);
        }
        
        result += "]";
        return result;
      }
  }
  
  return "null";
}

inline std::shared_ptr<OwoParser> OwoUtils::createTestConfig(){
  auto parser = std::make_shared<OwoParser>();
  
  std::string testConfig = R"(
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
  
  // Set some test firmware variables
  parser->setFirmwareVariable("firmwareVariable", "testValue");
  
  parser->parseString(testConfig);
  return parser;
}

// OwoConfigManager Implementation
inline std::shared_ptr<OwoParser> OwoConfigManager::loadConfig(const std::string& file_path, bool force_reload){
  if(!force_reload){
    auto it = cached_configs_.find(file_path);
    if(it != cached_configs_.end()){
      return it->second;
    }
  }
  
  auto parser = std::make_shared<OwoParser>();
  
  // Apply global firmware variables
  parser->setFirmwareVariables(global_firmware_vars_);
  
  if(parser->parseFile(file_path)){
    cached_configs_[file_path] = parser;
    return parser;
  }
  
  return nullptr;
}

inline void OwoConfigManager::setGlobalFirmwareVariable(const std::string& name, const std::string& value){
  global_firmware_vars_[name] = value;
  
  // Update all cached configs
  for(auto& pair : cached_configs_){
    pair.second->setFirmwareVariable(name, value);
  }
}

inline void OwoConfigManager::setGlobalFirmwareVariables(const std::unordered_map<std::string, std::string>& variables){
  global_firmware_vars_ = variables;
  
  // Update all cached configs
  for(auto& pair : cached_configs_){
    pair.second->setFirmwareVariables(variables);
  }
}

inline void OwoConfigManager::clearCache(){
  cached_configs_.clear();
}

inline std::vector<std::string> OwoConfigManager::getLoadedConfigs() const{
  std::vector<std::string> paths;
  paths.reserve(cached_configs_.size());
  
  for(const auto& pair : cached_configs_){
    paths.push_back(pair.first);
  }
  
  return paths;
}

inline bool OwoConfigManager::validateAllConfigs() const{
  for(const auto& pair : cached_configs_){
    if(!pair.second->isValid()){
      return false;
    }
  }
  
  return true;
}

} // namespace arcos::processing::parser::config_owo