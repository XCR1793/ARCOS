#pragma once

#include <fstream>
#include <sstream>
#include <regex>
#include <cmath>
#include <algorithm>
#include <cctype>

namespace arcos::processing::parser::config_owo{

// OwoValue Implementation
inline OwoValue::OwoValue(const OwoValueVariant& value, bool is_var_ref) : value_(value), is_variable_ref_(is_var_ref){
  if(std::holds_alternative<OwoNumber>(value)){
    type_ = Type::Number;
  }else if(std::holds_alternative<OwoString>(value)){
    type_ = is_var_ref ? Type::VariableRef : Type::String;
  }else if(std::holds_alternative<OwoBool>(value)){
    type_ = Type::Bool;
  }else if(std::holds_alternative<OwoArray>(value)){
    type_ = Type::Array;
  }
}

inline bool OwoValue::isNumber() const{ return type_ == Type::Number; }
inline bool OwoValue::isString() const{ return type_ == Type::String; }
inline bool OwoValue::isBool() const{ return type_ == Type::Bool; }
inline bool OwoValue::isArray() const{ return type_ == Type::Array; }
inline bool OwoValue::isVariableRef() const{ return type_ == Type::VariableRef; }

inline OwoNumber OwoValue::asNumber() const{
  if(type_ != Type::Number){
    return 0.0; // Return default value instead of throwing
  }
  return std::get<OwoNumber>(value_);
}

inline const OwoString& OwoValue::asString() const{
  if(type_ != Type::String){
    static const OwoString empty_string;
    return empty_string; // Return empty string instead of throwing
  }
  return std::get<OwoString>(value_);
}

inline OwoBool OwoValue::asBool() const{
  if(type_ != Type::Bool){
    return false; // Return default value instead of throwing
  }
  return std::get<OwoBool>(value_);
}

inline const OwoArray& OwoValue::asArray() const{
  if(type_ != Type::Array){
    static const OwoArray empty_array;
    return empty_array; // Return empty array instead of throwing
  }
  return std::get<OwoArray>(value_);
}

inline const OwoString& OwoValue::asVariableRef() const{
  if(type_ != Type::VariableRef){
    static const OwoString empty_string;
    return empty_string; // Return empty string instead of throwing
  }
  return std::get<OwoString>(value_);
}

inline OwoValue::Type OwoValue::getType() const{
  return type_;
}

inline std::string OwoValue::toString() const{
  switch(type_){
    case Type::Number:
      return std::to_string(asNumber());
    case Type::String:
      return "\"" + asString() + "\"";
    case Type::Bool:
      return asBool() ? "true" : "false";
    case Type::VariableRef:
      return "{" + asVariableRef() + "}";
    case Type::Array:
      {
        std::string result = "[";
        const auto& arr = asArray();
        for(size_t i = 0; i < arr.size(); ++i){
          if(i > 0) result += ", ";
          result += arr[i]->toString();
        }
        result += "]";
        return result;
      }
  }
  return "";
}

// OwoSection Implementation
inline OwoSection::OwoSection(const std::string& name, OwoSection* parent)
  : name_(name), parent_(parent){
}

inline void OwoSection::setVariable(const std::string& name, std::shared_ptr<OwoValue> value, VariableScope scope){
  variables_[name] = value;
  variable_scopes_[name] = scope;
}

inline std::shared_ptr<OwoValue> OwoSection::getVariable(const std::string& name) const{
  // Check if variable contains dot notation (sub-variable)
  size_t dot_pos = name.find('.');
  if(dot_pos != std::string::npos){
    std::string base_name = name.substr(0, dot_pos);
    std::string sub_name = name.substr(dot_pos + 1);
    
    auto it = variables_.find(base_name);
    if(it != variables_.end()){
      // For now, return the base variable - full sub-variable support would need extension
      return it->second;
    }
  }
  
  // Look in current section
  auto it = variables_.find(name);
  if(it != variables_.end()){
    return it->second;
  }
  
  // Look in parent sections for scoped and global variables
  OwoSection* current_parent = parent_;
  while(current_parent != nullptr){
    auto parent_it = current_parent->variables_.find(name);
    if(parent_it != current_parent->variables_.end()){
      auto scope_it = current_parent->variable_scopes_.find(name);
      if(scope_it != current_parent->variable_scopes_.end()){
        VariableScope scope = scope_it->second;
        if(scope == VariableScope::Scoped || scope == VariableScope::Global){
          return parent_it->second;
        }
      }
    }
    current_parent = current_parent->parent_;
  }
  
  return nullptr;
}

inline bool OwoSection::hasVariable(const std::string& name) const{
  return getVariable(name) != nullptr;
}

inline std::shared_ptr<OwoSection> OwoSection::createSubSection(const std::string& name){
  auto section = std::make_shared<OwoSection>(name, this);
  sub_sections_[name] = section;
  return section;
}

inline std::shared_ptr<OwoSection> OwoSection::getSubSection(const std::string& name) const{
  auto it = sub_sections_.find(name);
  return (it != sub_sections_.end()) ? it->second : nullptr;
}

inline bool OwoSection::hasSubSection(const std::string& name) const{
  return sub_sections_.find(name) != sub_sections_.end();
}

inline std::shared_ptr<OwoSection> OwoSection::getSection(const std::string& path) const{
  if(path.empty()){
    return nullptr;
  }
  
  size_t dot_pos = path.find('.');
  if(dot_pos == std::string::npos){
    return getSubSection(path);
  }
  
  std::string first_part = path.substr(0, dot_pos);
  std::string remaining_path = path.substr(dot_pos + 1);
  
  auto sub_section = getSubSection(first_part);
  if(sub_section){
    return sub_section->getSection(remaining_path);
  }
  
  return nullptr;
}

inline const std::string& OwoSection::getName() const{ return name_; }

inline const std::unordered_map<std::string, std::shared_ptr<OwoValue>>& OwoSection::getVariables() const{
  return variables_;
}

inline const std::unordered_map<std::string, std::shared_ptr<OwoSection>>& OwoSection::getSubSections() const{
  return sub_sections_;
}

inline OwoSection* OwoSection::getParent() const{ return parent_; }

inline std::string OwoSection::getFullPath() const{
  if(parent_ == nullptr){
    return name_;
  }
  
  std::string parent_path = parent_->getFullPath();
  if(parent_path.empty()){
    return name_;
  }
  
  return parent_path + "." + name_;
}

inline std::vector<std::string> OwoSection::getVariableNames() const{
  std::vector<std::string> names;
  names.reserve(variables_.size());
  for(const auto& pair : variables_){
    names.push_back(pair.first);
  }
  return names;
}

inline std::vector<std::string> OwoSection::getSubSectionNames() const{
  std::vector<std::string> names;
  names.reserve(sub_sections_.size());
  for(const auto& pair : sub_sections_){
    names.push_back(pair.first);
  }
  return names;
}

// OwoParser Implementation
inline OwoParser::OwoParser() : line_number_(0){
  reset();
}

inline bool OwoParser::parseFile(const std::string& file_path){
  file_path_ = file_path;
  std::ifstream file(file_path);
  if(!file.is_open()){
    last_error_ = "Could not open file: " + file_path;
    return false;
  }
  
  std::string content;
  std::string line;
  while(std::getline(file, line)){
    content += line + "\n";
  }
  
  return parseString(content);
}

inline bool OwoParser::parseString(const std::string& content){
  reset();
  
  std::istringstream stream(content);
  std::string line;
  line_number_ = 0;
  
  std::shared_ptr<OwoSection> current_section = root_section_;
  std::vector<std::shared_ptr<OwoSection>> section_stack;
  section_stack.push_back(current_section);
  
  while(std::getline(stream, line)){
    line_number_++;
    current_line_ = line;
    
    line = trim(removeComment(line));
    if(line.empty()){
      continue;
    }
    
    // Check for version line
    if(line.find("# version") == 0){
      continue; // Skip version lines
    }
    
    // Check for section header
    if(line.front() == '[' && line.back() == ']'){
      std::string section_name = line.substr(1, line.length() - 2);
      
      // Handle nested sections with dot notation
      std::vector<std::string> parts;
      std::istringstream ss(section_name);
      std::string part;
      while(std::getline(ss, part, '.')){
        parts.push_back(part);
      }
      
      // Navigate to the correct parent section
      current_section = root_section_;
      section_stack.clear();
      section_stack.push_back(current_section);
      
      for(const auto& part_name : parts){
        auto sub_section = current_section->getSubSection(part_name);
        if(!sub_section){
          sub_section = current_section->createSubSection(part_name);
        }
        current_section = sub_section;
        section_stack.push_back(current_section);
      }
      
      continue;
    }
    
    // Parse variable assignment
    size_t equals_pos = line.find('=');
    if(equals_pos != std::string::npos){
      if(!parseVariable(line, current_section)){
        return false;
      }
    }
  }
  
  return true;
}

inline void OwoParser::setFirmwareVariable(const std::string& name, const std::string& value){
  firmware_variables_[name] = value;
}

inline void OwoParser::setFirmwareVariables(const std::unordered_map<std::string, std::string>& variables){
  firmware_variables_ = variables;
}

inline std::shared_ptr<OwoSection> OwoParser::getRootSection() const{
  return root_section_;
}

inline std::shared_ptr<OwoSection> OwoParser::getSection(const std::string& path) const{
  return root_section_->getSection(path);
}

inline std::shared_ptr<OwoValue> OwoParser::getValue(const std::string& path) const{
  size_t last_dot = path.find_last_of('.');
  if(last_dot == std::string::npos){
    return root_section_->getVariable(path);
  }
  
  std::string section_path = path.substr(0, last_dot);
  std::string variable_name = path.substr(last_dot + 1);
  
  auto section = getSection(section_path);
  if(section){
    return section->getVariable(variable_name);
  }
  
  return nullptr;
}

inline bool OwoParser::isValid() const{
  return root_section_ != nullptr && last_error_.empty();
}

inline std::string OwoParser::getLastError() const{
  return last_error_;
}

inline void OwoParser::reset(){
  root_section_ = std::make_shared<OwoSection>("");
  firmware_variables_.clear();
  last_error_.clear();
  line_number_ = 0;
  current_line_.clear();
  file_path_.clear();
}

// Private method implementations
inline bool OwoParser::parseVariable(const std::string& line, std::shared_ptr<OwoSection> current_section){
  size_t equals_pos = line.find('=');
  std::string var_part = trim(line.substr(0, equals_pos));
  std::string value_part = trim(line.substr(equals_pos + 1));
  
  std::string clean_var_name;
  VariableScope scope = parseVariableScope(var_part, clean_var_name);
  
  auto value = parseValue(value_part);
  if(!value){
    last_error_ = "Failed to parse value: " + value_part + " at line " + std::to_string(line_number_);
    return false;
  }
  
  current_section->setVariable(clean_var_name, value, scope);
  return true;
}

inline std::shared_ptr<OwoValue> OwoParser::parseValue(const std::string& value_str){
  std::string str = trim(value_str);
  
  // Replace firmware variables
  str = resolveFirmwareVariables(str);
  
  // Check for different value types
  if(str.front() == '"' && str.back() == '"'){
    return parseStringValue(str);
  }else if(str.front() == '['){
    return parseArray(str);
  }else if(str == "true" || str == "false"){
    return parseBool(str);
  }else if(str.front() == '{' && str.back() == '}'){
    return std::make_shared<OwoValue>(OwoValueVariant(OwoString(str.substr(1, str.length() - 2))), true);
  }else if(str.find_first_of("+-*/(") != std::string::npos){
    return parseExpression(str, nullptr); // Simple expression parsing
  }else{
    return parseNumber(str);
  }
}

inline std::shared_ptr<OwoValue> OwoParser::parseNumber(const std::string& str){
  try{
    double value = std::stod(str);
    return std::make_shared<OwoValue>(OwoValueVariant(OwoNumber(value)));
  }catch(...){
    return nullptr;
  }
}

inline std::shared_ptr<OwoValue> OwoParser::parseStringValue(const std::string& str){
  if(str.length() >= 2 && str.front() == '"' && str.back() == '"'){
    return std::make_shared<OwoValue>(OwoValueVariant(OwoString(str.substr(1, str.length() - 2))));
  }
  return nullptr;
}

inline std::shared_ptr<OwoValue> OwoParser::parseBool(const std::string& str){
  if(str == "true"){
    return std::make_shared<OwoValue>(OwoValueVariant(OwoBool(true)));
  }else if(str == "false"){
    return std::make_shared<OwoValue>(OwoValueVariant(OwoBool(false)));
  }
  return nullptr;
}

inline std::shared_ptr<OwoValue> OwoParser::parseArray(const std::string& str){
  if(str.front() != '[' || str.back() != ']'){
    return nullptr;
  }
  
  std::string content = str.substr(1, str.length() - 2);
  OwoArray array;
  
  // Simple array parsing - handles nested arrays and mixed types
  std::string current_element;
  int bracket_depth = 0;
  bool in_quotes = false;
  
  for(char c : content){
    if(c == '"' && (current_element.empty() || current_element.back() != '\\')){
      in_quotes = !in_quotes;
      current_element += c;
    }else if(!in_quotes){
      if(c == '['){
        bracket_depth++;
        current_element += c;
      }else if(c == ']'){
        bracket_depth--;
        current_element += c;
      }else if(c == ',' && bracket_depth == 0){
        std::string element = trim(current_element);
        if(!element.empty()){
          auto value = parseValue(element);
          if(value){
            array.push_back(value);
          }
        }
        current_element.clear();
      }else{
        current_element += c;
      }
    }else{
      current_element += c;
    }
  }
  
  // Handle last element
  std::string element = trim(current_element);
  if(!element.empty()){
    auto value = parseValue(element);
    if(value){
      array.push_back(value);
    }
  }
  
  return std::make_shared<OwoValue>(OwoValueVariant(array));
}

inline std::shared_ptr<OwoValue> OwoParser::parseExpression(const std::string& str, std::shared_ptr<OwoSection> context){
  // Very basic expression parsing - handles simple arithmetic
  // For production use, consider implementing a proper expression parser
  
  std::string expr = str;
  
  // Remove spaces
  expr.erase(std::remove_if(expr.begin(), expr.end(), ::isspace), expr.end());
  
  // Simple evaluation - just return as number for now
  // In a full implementation, this would parse and evaluate the expression
  try{
    // Try to evaluate simple expressions
    // This is a simplified version - a full parser would handle operator precedence
    double result = 0.0;
    
    // For now, just try to parse as number
    if(expr.find_first_not_of("0123456789.+-*/()") == std::string::npos){
      // Contains only numeric and operator characters
      // Simplified evaluation - just return 0 for complex expressions
      result = 0.0;
    }
    
    return std::make_shared<OwoValue>(OwoValueVariant(OwoNumber(result)));
  }catch(...){
    return std::make_shared<OwoValue>(OwoValueVariant(OwoNumber(0.0)));
  }
}

inline std::string OwoParser::resolveFirmwareVariables(const std::string& str){
  std::string result = str;
  
  std::regex var_regex(R"(\{([^}]+)\})");
  std::smatch match;
  
  while(std::regex_search(result, match, var_regex)){
    std::string var_name = match[1].str();
    auto it = firmware_variables_.find(var_name);
    if(it != firmware_variables_.end()){
      result.replace(match.position(), match.length(), it->second);
    }else{
      result.replace(match.position(), match.length(), "0"); // Default value
    }
  }
  
  return result;
}

inline std::string OwoParser::removeComment(const std::string& line){
  size_t comment_pos = line.find(';');
  if(comment_pos != std::string::npos){
    return line.substr(0, comment_pos);
  }
  return line;
}

inline std::string OwoParser::trim(const std::string& str){
  size_t start = str.find_first_not_of(" \t\r\n");
  if(start == std::string::npos){
    return "";
  }
  
  size_t end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}

inline VariableScope OwoParser::parseVariableScope(const std::string& variable_name, std::string& clean_name){
  std::regex scope_regex(R"(^([^(]+)(?:\(([^)]+)\))?$)");
  std::smatch match;
  
  if(std::regex_match(variable_name, match, scope_regex)){
    clean_name = trim(match[1].str());
    
    if(match.size() > 2 && match[2].matched){
      std::string scope_str = trim(match[2].str());
      if(scope_str == "global"){
        return VariableScope::Global;
      }else if(scope_str == "scoped"){
        return VariableScope::Scoped;
      }
    }
  }else{
    clean_name = variable_name;
  }
  
  return VariableScope::Local;
}

} // namespace arcos::processing::parser::config_owo