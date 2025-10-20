#pragma once

#include "owo_parser.hpp"

namespace arcos::processing::parser::config_owo{

/**
 * @brief Utility functions for working with OWO config files
 */
class OwoUtils{
public:
  /**
   * @brief Convert OwoValue to common C++ types with error handling
   */
  template<typename T>
  static bool getValue(std::shared_ptr<OwoValue> value, T& result);
  
  /**
   * @brief Get value by path with type conversion
   */
  template<typename T>
  static bool getValueByPath(std::shared_ptr<OwoParser> parser, const std::string& path, T& result);
  
  /**
   * @brief Print configuration tree for debugging
   */
  static void printConfigTree(std::shared_ptr<OwoSection> section, int indent = 0);
  
  /**
   * @brief Validate configuration against expected structure
   */
  static bool validateConfig(std::shared_ptr<OwoParser> parser, const std::vector<std::string>& required_paths);
  
  /**
   * @brief Export configuration to JSON format
   */
  static std::string toJson(std::shared_ptr<OwoSection> section);
  
  /**
   * @brief Create a simple config builder for testing
   */
  static std::shared_ptr<OwoParser> createTestConfig();

private:
  static void printSection(std::shared_ptr<OwoSection> section, int indent);
  static std::string sectionToJson(std::shared_ptr<OwoSection> section, int indent = 0);
  static std::string valueToJson(std::shared_ptr<OwoValue> value);
};

/**
 * @brief Configuration manager for OWO files with caching and validation
 */
class OwoConfigManager{
private:
  std::unordered_map<std::string, std::shared_ptr<OwoParser>> cached_configs_;
  std::unordered_map<std::string, std::string> global_firmware_vars_;

public:
  /**
   * @brief Load configuration file with caching
   */
  std::shared_ptr<OwoParser> loadConfig(const std::string& file_path, bool force_reload = false);
  
  /**
   * @brief Set global firmware variables that apply to all configs
   */
  void setGlobalFirmwareVariable(const std::string& name, const std::string& value);
  void setGlobalFirmwareVariables(const std::unordered_map<std::string, std::string>& variables);
  
  /**
   * @brief Clear cache and reload all configurations
   */
  void clearCache();
  
  /**
   * @brief Get all loaded configuration file paths
   */
  std::vector<std::string> getLoadedConfigs() const;
  
  /**
   * @brief Validate all loaded configurations
   */
  bool validateAllConfigs() const;
};

} // namespace arcos::processing::parser::config_owo

#include "owo_utils_impl.hpp"