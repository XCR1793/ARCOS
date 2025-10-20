#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <variant>
#include <memory>

namespace arcos::processing::parser::config_owo{

// Forward declarations
class OwoValue;
class OwoSection;
class OwoParser;

// Type aliases for different value types
using OwoNumber = double;
using OwoString = std::string;
using OwoBool = bool;
using OwoArray = std::vector<std::shared_ptr<OwoValue>>;
using OwoVariableRef = std::string; // For {variableName} references

// Variant type for all possible OWO values (note: OwoVariableRef is same type as OwoString, but semantically different)
using OwoValueVariant = std::variant<OwoNumber, OwoString, OwoBool, OwoArray>;

/**
 * @brief Represents a single value in OWO config format
 * 
 * Can hold numbers, strings, booleans, arrays, or variable references
 */
class OwoValue{
public:
  enum class Type{
    Number,
    String,
    Bool,
    Array,
    VariableRef  // Special case of String
  };

private:
  OwoValueVariant value_;
  Type type_;
  bool is_variable_ref_;

public:
  OwoValue(const OwoValueVariant& value, bool is_var_ref = false);
  
  // Type checking methods
  bool isNumber() const;
  bool isString() const;
  bool isBool() const;
  bool isArray() const;
  bool isVariableRef() const;
  
  // Value getters with type safety
  OwoNumber asNumber() const;
  const OwoString& asString() const;
  OwoBool asBool() const;
  const OwoArray& asArray() const;
  const OwoString& asVariableRef() const;
  
  Type getType() const;
  
  // String representation for debugging
  std::string toString() const;
};

/**
 * @brief Variable scope enumeration
 */
enum class VariableScope{
  Local,    // Default - local to current section
  Scoped,   // Available to sub-sections
  Global    // Available globally
};

/**
 * @brief Represents a configuration section with variables and sub-sections
 */
class OwoSection{
private:
  std::string name_;
  std::unordered_map<std::string, std::shared_ptr<OwoValue>> variables_;
  std::unordered_map<std::string, VariableScope> variable_scopes_;
  std::unordered_map<std::string, std::shared_ptr<OwoSection>> sub_sections_;
  OwoSection* parent_;

public:
  explicit OwoSection(const std::string& name, OwoSection* parent = nullptr);
  
  // Variable management
  void setVariable(const std::string& name, std::shared_ptr<OwoValue> value, VariableScope scope = VariableScope::Local);
  std::shared_ptr<OwoValue> getVariable(const std::string& name) const;
  bool hasVariable(const std::string& name) const;
  
  // Sub-section management
  std::shared_ptr<OwoSection> createSubSection(const std::string& name);
  std::shared_ptr<OwoSection> getSubSection(const std::string& name) const;
  bool hasSubSection(const std::string& name) const;
  
  // Navigation
  std::shared_ptr<OwoSection> getSection(const std::string& path) const;
  
  // Getters
  const std::string& getName() const;
  const std::unordered_map<std::string, std::shared_ptr<OwoValue>>& getVariables() const;
  const std::unordered_map<std::string, std::shared_ptr<OwoSection>>& getSubSections() const;
  
  // Parent access
  OwoSection* getParent() const;
  
  // Utility methods
  std::string getFullPath() const;
  std::vector<std::string> getVariableNames() const;
  std::vector<std::string> getSubSectionNames() const;
};

/**
 * @brief Main parser class for OWO configuration files
 */
class OwoParser{
private:
  std::shared_ptr<OwoSection> root_section_;
  std::unordered_map<std::string, std::string> firmware_variables_;
  std::string current_line_;
  size_t line_number_;
  std::string file_path_;

public:
  OwoParser();
  
  // Main parsing methods
  bool parseFile(const std::string& file_path);
  bool parseString(const std::string& content);
  
  // Firmware variable management
  void setFirmwareVariable(const std::string& name, const std::string& value);
  void setFirmwareVariables(const std::unordered_map<std::string, std::string>& variables);
  
  // Access methods
  std::shared_ptr<OwoSection> getRootSection() const;
  std::shared_ptr<OwoSection> getSection(const std::string& path) const;
  std::shared_ptr<OwoValue> getValue(const std::string& path) const;
  
  // Utility methods
  bool isValid() const;
  std::string getLastError() const;
  void reset();

private:
  // Internal parsing methods - see implementation file
  bool parseLine(const std::string& line);
  bool parseSection(const std::string& line);
  bool parseVariable(const std::string& line, std::shared_ptr<OwoSection> current_section);
  
  std::shared_ptr<OwoValue> parseValue(const std::string& value_str);
  std::shared_ptr<OwoValue> parseNumber(const std::string& str);
  std::shared_ptr<OwoValue> parseStringValue(const std::string& str);
  std::shared_ptr<OwoValue> parseBool(const std::string& str);
  std::shared_ptr<OwoValue> parseArray(const std::string& str);
  std::shared_ptr<OwoValue> parseExpression(const std::string& str, std::shared_ptr<OwoSection> context);
  
  std::string resolveFirmwareVariables(const std::string& str);
  std::string removeComment(const std::string& line);
  std::string trim(const std::string& str);
  
  VariableScope parseVariableScope(const std::string& variable_name, std::string& clean_name);
  
  std::string last_error_;
};

} // namespace arcos::processing::parser::config_owo

// Include implementation
#include "owo_parser_impl.hpp"