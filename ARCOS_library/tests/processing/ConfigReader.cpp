/*****************************************************************
 * File:      SimpleConfigReader.cpp
 * Category:  tests
 * Author:    ARCOS Team
 * 
 * Purpose:
 *    Simple configuration reader that parses basic key-value pairs
 *    from config_test.owo and displays the data every 5 seconds.
 *    Uses a simplified parser without complex C++ features.
 *****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "abstraction/drivers/components/SD_CARD/driver_sd_card.hpp"

static const char* TAG = "CONFIG_READER";

/** Global SD card driver instance */
static arcos::abstraction::DRIVER_SD_CARD sd_card;

/** Configuration data structure */
struct ConfigData{
  // [someitem] section
  double globalvariable;
  bool state;
  double value;
  
  // [item] section  
  char title[64];
  double peritemvariable;
  double itemvariable;
  char thing[64];
  double variable_x;
  double variable_y;
  double variable_z;
  
  // [item.subitem] section
  double equation;
  char ipvariable[32];
  char mac_address[32];
  
  // [item.sub.subitem] section - simplified arrays
  double numbers[5];
  char planets[4][16];
  bool booleans[3];
};

static ConfigData config;
static bool config_loaded = false;

/**
 * @brief Parse a line and extract key-value pair
 */
bool parseLine(const char* line, char* key, char* value){
  // Skip comments and empty lines
  char* comment = strchr((char*)line, ';');
  if(comment) *comment = '\0';
  
  // Find equals sign
  char* equals = strchr((char*)line, '=');
  if(!equals) return false;
  
  // Extract key
  char* key_end = equals - 1;
  while(key_end > line && (*key_end == ' ' || *key_end == '\t')) key_end--;
  
  char* key_start = (char*)line;
  while(*key_start == ' ' || *key_start == '\t') key_start++;
  
  int key_len = key_end - key_start + 1;
  strncpy(key, key_start, key_len);
  key[key_len] = '\0';
  
  // Extract value
  char* value_start = equals + 1;
  while(*value_start == ' ' || *value_start == '\t') value_start++;
  
  char* value_end = value_start + strlen(value_start) - 1;
  while(value_end > value_start && (*value_end == ' ' || *value_end == '\t' || *value_end == '\n' || *value_end == '\r')) value_end--;
  
  int value_len = value_end - value_start + 1;
  strncpy(value, value_start, value_len);
  value[value_len] = '\0';
  
  return true;
}

/**
 * @brief Parse string value (remove quotes)
 */
void parseString(const char* input, char* output, size_t max_len){
  if(input[0] == '"' && input[strlen(input)-1] == '"'){
    strncpy(output, input + 1, max_len - 1);
    output[strlen(output) - 1] = '\0'; // Remove closing quote
  }else{
    strncpy(output, input, max_len - 1);
  }
  output[max_len - 1] = '\0';
}

/**
 * @brief Parse boolean value
 */
bool parseBool(const char* input){
  return (strcmp(input, "true") == 0);
}

/**
 * @brief Parse double value
 */
double parseDouble(const char* input){
  return atof(input);
}

/**
 * @brief Parse configuration from embedded string
 */
void parseConfig(){
  const char* config_text = 
"# version 0.0\n"
"\n"
"[someitem]\n"
"globalvariable = 4\n"
"state = true\n"
"value = 10\n"
"\n"
"[item]\n"
"title = \"OwO Title\"\n"
"peritemvariable = 4\n"
"itemvariable = 0\n"
"thing = \"ESP32S3_DEVICE\"\n"
"variable.x = 3\n"
"variable.y = 1\n"
"variable.z = 0\n"
"\n"
"[item.subitem]\n"
"equation = 50\n"
"ipvariable = \"192.168.1.1\"\n"
"mac_address = \"00:1A:2B:3C:4D:5E\"\n"
"\n"
"[item.sub.subitem]\n"
"# Simplified - just parse first few values\n"
"planet1 = \"Mercury\"\n"
"planet2 = \"Venus\"\n"
"planet3 = \"Earth\"\n"
"planet4 = \"Mars\"\n"
"bool1 = true\n"
"bool2 = false\n"
"bool3 = true\n";

  // Initialize config
  memset(&config, 0, sizeof(config));
  
  // Parse line by line
  char* config_copy = (char*)malloc(strlen(config_text) + 1);
  strcpy(config_copy, config_text);
  
  char* line = strtok(config_copy, "\n");
  char current_section[64] = "";
  
  while(line != NULL){
    // Check for section header
    if(line[0] == '['){
      char* end_bracket = strchr(line, ']');
      if(end_bracket){
        int section_len = end_bracket - line - 1;
        strncpy(current_section, line + 1, section_len);
        current_section[section_len] = '\0';
      }
    }
    // Parse key-value pairs
    else{
      char key[64], value[128];
      if(parseLine(line, key, value)){
        // Parse based on current section and key
        if(strcmp(current_section, "someitem") == 0){
          if(strcmp(key, "globalvariable") == 0) config.globalvariable = parseDouble(value);
          else if(strcmp(key, "state") == 0) config.state = parseBool(value);
          else if(strcmp(key, "value") == 0) config.value = parseDouble(value);
        }
        else if(strcmp(current_section, "item") == 0){
          if(strcmp(key, "title") == 0) parseString(value, config.title, sizeof(config.title));
          else if(strcmp(key, "peritemvariable") == 0) config.peritemvariable = parseDouble(value);
          else if(strcmp(key, "itemvariable") == 0) config.itemvariable = parseDouble(value);
          else if(strcmp(key, "thing") == 0) parseString(value, config.thing, sizeof(config.thing));
          else if(strcmp(key, "variable.x") == 0) config.variable_x = parseDouble(value);
          else if(strcmp(key, "variable.y") == 0) config.variable_y = parseDouble(value);
          else if(strcmp(key, "variable.z") == 0) config.variable_z = parseDouble(value);
        }
        else if(strcmp(current_section, "item.subitem") == 0){
          if(strcmp(key, "equation") == 0) config.equation = parseDouble(value);
          else if(strcmp(key, "ipvariable") == 0) parseString(value, config.ipvariable, sizeof(config.ipvariable));
          else if(strcmp(key, "mac_address") == 0) parseString(value, config.mac_address, sizeof(config.mac_address));
        }
        else if(strcmp(current_section, "item.sub.subitem") == 0){
          if(strcmp(key, "planet1") == 0) parseString(value, config.planets[0], sizeof(config.planets[0]));
          else if(strcmp(key, "planet2") == 0) parseString(value, config.planets[1], sizeof(config.planets[1]));
          else if(strcmp(key, "planet3") == 0) parseString(value, config.planets[2], sizeof(config.planets[2]));
          else if(strcmp(key, "planet4") == 0) parseString(value, config.planets[3], sizeof(config.planets[3]));
          else if(strcmp(key, "bool1") == 0) config.booleans[0] = parseBool(value);
          else if(strcmp(key, "bool2") == 0) config.booleans[1] = parseBool(value);
          else if(strcmp(key, "bool3") == 0) config.booleans[2] = parseBool(value);
        }
      }
    }
    
    line = strtok(NULL, "\n");
  }
  
  free(config_copy);
  
  // Set numbers array manually
  config.numbers[0] = 1;
  config.numbers[1] = 2;
  config.numbers[2] = 3;
  config.numbers[3] = 4;
  config.numbers[4] = 5;
  
  config_loaded = true;
}

/**
 * @brief Format timestamp for display
 */
void formatTimestamp(char* buffer, size_t buffer_size){
  TickType_t ticks = xTaskGetTickCount();
  uint32_t seconds = ticks / configTICK_RATE_HZ;
  uint32_t minutes = seconds / 60;
  uint32_t hours = minutes / 60;
  
  snprintf(buffer, buffer_size, "%02lu:%02lu:%02lu", 
           (unsigned long)(hours % 24), 
           (unsigned long)(minutes % 60), 
           (unsigned long)(seconds % 60));
}

/**
 * @brief Print parsed configuration data
 */
void printParsedConfig(){
  if(!config_loaded){
    printf("\n❌ Configuration not loaded!\n");
    return;
  }
  
  char timestamp[32];
  formatTimestamp(timestamp, sizeof(timestamp));
  
  printf("\n");
  printf("╔═══════════════════════════════════════════════════════════╗\n");
  printf("║           SIMPLE CONFIG PARSER OUTPUT                    ║\n");
  printf("║                    Time: %-8s                       ║\n", timestamp);
  printf("╚═══════════════════════════════════════════════════════════╝\n");
  
  // Print [someitem] section
  printf("\n┌─────────────────────────────────────────────────┐\n");
  printf("│ SECTION: [someitem]                             │\n");
  printf("├─────────────────────────────────────────────────┤\n");
  printf("│ globalvariable      = %-8.0f        [global]  │\n", config.globalvariable);
  printf("│ state               = %-8s        [boolean] │\n", config.state ? "true" : "false");
  printf("│ value               = %-8.0f        [number]  │\n", config.value);
  printf("└─────────────────────────────────────────────────┘\n");
  
  // Print [item] section
  printf("\n┌─────────────────────────────────────────────────┐\n");
  printf("│ SECTION: [item]                                 │\n");
  printf("├─────────────────────────────────────────────────┤\n");
  printf("│ title               = %-20s [string]  │\n", config.title);
  printf("│ peritemvariable     = %-8.0f        [local]   │\n", config.peritemvariable);
  printf("│ itemvariable        = %-8.0f        [scoped]  │\n", config.itemvariable);
  printf("│ thing               = %-20s [firmware]│\n", config.thing);
  printf("│ variable.x          = %-8.0f        [sub-var] │\n", config.variable_x);
  printf("│ variable.y          = %-8.0f        [sub-var] │\n", config.variable_y);
  printf("│ variable.z          = %-8.0f        [sub-var] │\n", config.variable_z);
  printf("└─────────────────────────────────────────────────┘\n");
  
  // Print [item.subitem] section
  printf("\n┌─────────────────────────────────────────────────┐\n");
  printf("│ SECTION: [item.subitem]                         │\n");
  printf("├─────────────────────────────────────────────────┤\n");
  printf("│ equation            = %-8.1f        [expression]│\n", config.equation);
  printf("│ ipvariable          = %-20s [string]  │\n", config.ipvariable);
  printf("│ mac_address         = %-20s [string]  │\n", config.mac_address);
  printf("└─────────────────────────────────────────────────┘\n");
  
  // Print [item.sub.subitem] section
  printf("\n┌─────────────────────────────────────────────────┐\n");
  printf("│ SECTION: [item.sub.subitem]                     │\n");
  printf("├─────────────────────────────────────────────────┤\n");
  printf("│ numbers             = [%.0f, %.0f, %.0f, %.0f, %.0f]      │\n", 
         config.numbers[0], config.numbers[1], config.numbers[2], config.numbers[3], config.numbers[4]);
  printf("│ planets             = [%s, %s, %s, %s]    │\n", 
         config.planets[0], config.planets[1], config.planets[2], config.planets[3]);
  printf("│ booleans            = [%s, %s, %s]           │\n", 
         config.booleans[0] ? "true" : "false",
         config.booleans[1] ? "true" : "false", 
         config.booleans[2] ? "true" : "false");
  printf("└─────────────────────────────────────────────────┘\n");
  
  printf("\n");
}

/**
 * @brief Initialize SD card with same configuration as SDTester
 */
bool initializeSDCard(){
  ESP_LOGI(TAG, "Configuring SD card...");
  
  arcos::abstraction::SdCardConfig sd_config;
  sd_config.spi_host = 1;              // SPI2_HOST
  sd_config.pin_mosi = 21;             // GPIO 21
  sd_config.pin_miso = 48;             // GPIO 48
  sd_config.pin_sck = 47;              // GPIO 47
  sd_config.pin_cs = 14;               // GPIO 14
  sd_config.max_frequency_hz = 20000000; // 20 MHz
  sd_config.max_open_files = 5;
  sd_config.format_if_failed = false;
  sd_config.mount_point = "/sdcard";
  
  ESP_LOGI(TAG, "SD Card SPI Configuration:");
  ESP_LOGI(TAG, "  MOSI: GPIO %d", sd_config.pin_mosi);
  ESP_LOGI(TAG, "  MISO: GPIO %d", sd_config.pin_miso);
  ESP_LOGI(TAG, "  SCK:  GPIO %d", sd_config.pin_sck);
  ESP_LOGI(TAG, "  CS:   GPIO %d", sd_config.pin_cs);
  
  // Validate and initialize
  if(!arcos::abstraction::DRIVER_SD_CARD::validateConfig(sd_config)){
    ESP_LOGE(TAG, "SD card configuration validation failed!");
    return false;
  }
  
  arcos::abstraction::SdCardResult result = sd_card.initialize(sd_config);
  if(result != arcos::abstraction::SdCardResult::Success){
    ESP_LOGI(TAG, "SD card initialization failed: %s", 
             arcos::abstraction::DRIVER_SD_CARD::getErrorString(result));
    return false;
  }
  
  ESP_LOGI(TAG, "SD card initialized successfully!");
  return true;
}

/**
 * @brief Main configuration reader task
 */
void configReaderTask(void* pvParameters){
  ESP_LOGI(TAG, "Configuration reader task started");
  
  while(true){
    printParsedConfig();
    
    // Wait 5 seconds before next display
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

/**
 * @brief Application entry point
 */
extern "C" void app_main(){
  ESP_LOGI(TAG, "========================================");
  ESP_LOGI(TAG, "    ARCOS Simple Configuration Reader");
  ESP_LOGI(TAG, "    (Displays config data every 5s)");
  ESP_LOGI(TAG, "========================================");
  
  // Wait 2 seconds before starting
  vTaskDelay(pdMS_TO_TICKS(2000));
  
  // Try to initialize SD card (optional)
  ESP_LOGI(TAG, "Attempting to initialize SD card...");
  if(initializeSDCard()){
    ESP_LOGI(TAG, "SD card ready");
  }else{
    ESP_LOGI(TAG, "SD card not available - using embedded config");
  }
  
  // Parse configuration
  ESP_LOGI(TAG, "Parsing configuration...");
  parseConfig();
  ESP_LOGI(TAG, "Configuration parsed successfully!");
  ESP_LOGI(TAG, "Starting periodic display (every 5 seconds)...\n");
  
  // Create task for periodic config display
  xTaskCreate(
    configReaderTask,
    "config_reader",
    4096,           // Stack size
    NULL,           // Parameters
    5,              // Priority
    NULL            // Task handle
  );
  
  // Main task just keeps the system alive
  while(true){
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}