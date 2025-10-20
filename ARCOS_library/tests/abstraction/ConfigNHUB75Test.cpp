/*****************************************************************
 * File:      main.cpp  
 * Category:  tests/abstraction
 * Author:    ARCOS Team
 * 
 * Purpose:
 *    Advanced HUB75 LED matrix driver with configurable animations
 *    loaded from config_test.owo file via SD card integration.
 *    Based on the WORKING HUB75RGBTest.cpp coordinate system.
 *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_random.h"

// ARCOS abstraction framework
#include "abstraction/hal.hpp"
#include "abstraction/drivers/components/HUB75/driver_hub75.hpp"
#include "abstraction/drivers/components/HUB75/driver_hub75_i2s.hpp"
#include "abstraction/drivers/components/SD_CARD/driver_sd_card.hpp"

// ARCOS fast math libraries for performance
#include "core/maths/fast_trig.hpp"

using namespace arcos::abstraction;
using namespace arcos::abstraction::drivers;
using namespace arcos::core::maths;

static const char* TAG = "ANIMATED_HUB75";

/** Platform implementations */
static HAL_PARALLEL_DEFAULT hardware;
static ParallelBuffer bufferManager;
static HUB75_I2S_Protocol i2sProtocol;
static HUB75Driver display;
static arcos::abstraction::DRIVER_SD_CARD sd_card;

/** Fast trigonometry for high-performance animations */
static FastTrig fast_trig(FastTrig::Precision::DEG_1, 
                          static_cast<uint32_t>(FastTrig::FunctionType::SIN) | 
                          static_cast<uint32_t>(FastTrig::FunctionType::COS));

/** RGB color structure (same as HUB75RGBTest.cpp) */
struct CRGB {
    uint8_t r, g, b;
    CRGB() : r(0), g(0), b(0) {}
    CRGB(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
};

/** Animation configuration */
struct AnimationConfig {
    // Display settings
    int width, height, panels;
    uint8_t brightness;
    bool gamma_correction;
    float gamma_value;
    
    // Animation globals
    float speed_multiplier;
    bool auto_pattern_switch;
    float pattern_switch_interval;
    
    // Pattern enables
    bool rainbow_wave_enabled;
    bool plasma_field_enabled;
    bool matrix_rain_enabled;
    bool fire_simulation_enabled;
    bool spiral_galaxy_enabled;
    
    // Pattern parameters
    float wave_frequency, saturation, lightness;
    float plasma_speed, plasma_scale;
    float drop_probability, rain_hue;
    float cooling_factor;
    int spiral_arms;
    float rotation_speed;
    int center_x, center_y;
    
    // Hardware pins (using int for compatibility)
    int r0_pin, g0_pin, b0_pin, r1_pin, g1_pin, b1_pin;
    int a_pin, b_pin, c_pin, d_pin, e_pin;
    int lat_pin, oe_pin, oe_pin2, clock_pin;
    
    int frame_time_ms;
};

/** Global variables */
static AnimationConfig config;
static int current_pattern = 0;
static float animation_time = 0.0f;
static uint64_t last_pattern_switch = 0;

/** HSL to RGB conversion (EXACT same as HUB75RGBTest.cpp) */
CRGB hslToRgb(float h, float s, float l) {
    h = fmodf(h, 360.0f);
    if(h < 0) h += 360.0f;
    s = fminf(fmaxf(s, 0.0f), 100.0f) / 100.0f;
    l = fminf(fmaxf(l, 0.0f), 100.0f) / 100.0f;
    
    float c = (1.0f - fabsf(2.0f * l - 1.0f)) * s;
    float h_prime = h / 60.0f;
    float x = c * (1.0f - fabsf(fmodf(h_prime, 2.0f) - 1.0f));
    float m = l - c / 2.0f;
    
    float r1, g1, b1;
    if(h_prime >= 0 && h_prime < 1) {
        r1 = c; g1 = x; b1 = 0;
    } else if(h_prime >= 1 && h_prime < 2) {
        r1 = x; g1 = c; b1 = 0;
    } else if(h_prime >= 2 && h_prime < 3) {
        r1 = 0; g1 = c; b1 = x;
    } else if(h_prime >= 3 && h_prime < 4) {
        r1 = 0; g1 = x; b1 = c;
    } else if(h_prime >= 4 && h_prime < 5) {
        r1 = x; g1 = 0; b1 = c;
    } else {
        r1 = c; g1 = 0; b1 = x;
    }
    
    uint8_t r = (uint8_t)roundf((r1 + m) * 255.0f);
    uint8_t g = (uint8_t)roundf((g1 + m) * 255.0f);
    uint8_t b = (uint8_t)roundf((b1 + m) * 255.0f);
    
    return CRGB(r, g, b);
}

/** Fire color mapping */
CRGB heatColor(uint8_t temperature) {
    uint8_t heatramp = (temperature * 192) / 255;
    
    if(heatramp > 128) {
        return CRGB(255, 255, heatramp - 128);
    } else if(heatramp > 64) {
        return CRGB(255, heatramp - 64, 0);
    } else {
        return CRGB(heatramp * 4, 0, 0);
    }
}

/** Draw pattern on panel (EXACT same coordinate system as HUB75RGBTest.cpp) */
void drawPatternOnPanel(int panel_index, int pattern_type) {
    const int panel_width = 64;
    const int panel_height = 32;
    int panel_x_offset = panel_index * panel_width;  // Panel 0: 0-63, Panel 1: 64-127
    
    // Pre-calculate common values for performance
    float time_factor = animation_time * 5.0f;
    float hue_offset = animation_time * 50.0f;
    
    for(int y = 0; y < panel_height; y++) {
        for(int x = 0; x < panel_width; x++) {
            int logical_x = panel_x_offset + x;  // Global X coordinate
            CRGB color = CRGB(0, 0, 0);
            
            switch(pattern_type) {
                case 0: { // Rainbow Wave (optimized with fast trig)
                    float wave = fast_trig.sin(logical_x * config.wave_frequency + time_factor);
                    float hue = fmodf((logical_x * 2.8125f) + hue_offset, 360.0f); // logical_x / 128 * 360 optimized
                    hue += wave * 120.0f;
                    color = hslToRgb(hue, config.saturation, config.lightness);
                    break;
                }
                case 1: { // Plasma Field (optimized with fast trig)
                    float plasma1 = fast_trig.sin(logical_x * config.plasma_scale + animation_time * config.plasma_speed);
                    float plasma2 = fast_trig.cos(y * config.plasma_scale + animation_time * config.plasma_speed * 0.7f);
                    float combined = (plasma1 + plasma2) * 0.5f; // Slightly optimized
                    float hue = fmodf((combined + 1.0f) * 180.0f + animation_time * 8.0f, 360.0f);
                    color = hslToRgb(hue, 100.0f, 50.0f);
                    break;
                }
                case 2: { // Matrix Rain (optimized simple green effect)
                    float intensity = fast_trig.sin(y * 0.5f - animation_time * 10.0f) * 0.5f + 0.5f;
                    if(((int)(animation_time * 100 + logical_x * 10) & 127) < 10) intensity *= 2.0f; // Bitwise optimization
                    color = hslToRgb(config.rain_hue, 100.0f, intensity * 60.0f);
                    break;
                }
                case 3: { // Fire Simulation (optimized simple version)
                    float heat = 1.0f - (y * 0.03125f); // 1/32 pre-calculated
                    heat += fast_trig.sin(logical_x * 0.3f + animation_time * 10.0f) * 0.3f;
                    heat = fmaxf(0.0f, fminf(1.0f, heat));
                    color = heatColor((uint8_t)(heat * 255));
                    break;
                }
                case 4: { // Spiral Galaxy (HEAVILY optimized - pre-calculate expensive operations)
                    float dx = logical_x - config.center_x;
                    float dy = y - config.center_y;
                    // Use fast approximation instead of sqrt for distance
                    float r_approx = fmaxf(fabsf(dx), fabsf(dy)) + 0.5f * fminf(fabsf(dx), fabsf(dy));
                    // Simplified spiral calculation avoiding atan2
                    float spiral_phase = (dx * 0.1f + dy * 0.05f) * config.spiral_arms - animation_time * config.rotation_speed;
                    float spiral = fast_trig.sin(spiral_phase) * 0.5f + 0.5f;
                    float intensity = spiral * fmaxf(0.0f, 1.0f - r_approx * 0.015625f); // 1/64 pre-calculated
                    float hue = fmodf(r_approx * 2.0f + animation_time * 30.0f, 360.0f);
                    color = hslToRgb(hue, 100.0f, intensity * 80.0f);
                    break;
                }
            }
            
            // Set pixel using logical coordinates (EXACT same as HUB75RGBTest.cpp)
            display.setPixel(logical_x, y, RGB(color.r, color.g, color.b));
        }
    }
}

/** Render current pattern to all panels */
void renderCurrentPattern() {
    // Draw pattern on both panels (optimized for smooth rendering)
    drawPatternOnPanel(0, current_pattern);  // Panel 0
    drawPatternOnPanel(1, current_pattern);  // Panel 1
    
    // Sync buffer swap for smooth double buffering (reduces flickering)
    display.show();
}

/** Update animation and handle pattern switching */
void updateAnimation() {
    animation_time += 0.016f * config.speed_multiplier;  // Optimized time step for 60 FPS
    
    // Handle pattern switching (less frequent for performance)
    if(config.auto_pattern_switch) {
        uint64_t current_time = esp_timer_get_time() / 1000000;
        if(current_time - last_pattern_switch > config.pattern_switch_interval) {
            // Find next enabled pattern
            int next_pattern = current_pattern;
            for(int i = 0; i < 5; i++) {
                next_pattern = (next_pattern + 1) % 5;
                if((next_pattern == 0 && config.rainbow_wave_enabled) ||
                   (next_pattern == 1 && config.plasma_field_enabled) ||
                   (next_pattern == 2 && config.matrix_rain_enabled) ||
                   (next_pattern == 3 && config.fire_simulation_enabled) ||
                   (next_pattern == 4 && config.spiral_galaxy_enabled)) {
                    break;
                }
            }
            if(next_pattern != current_pattern) {
                current_pattern = next_pattern;
                last_pattern_switch = current_time;
                const char* pattern_names[] = {"Rainbow Wave", "Plasma Field", "Matrix Rain", "Fire Simulation", "Spiral Galaxy"};
                ESP_LOGI(TAG, "Switched to pattern %d (%s)", current_pattern, pattern_names[current_pattern]);
            }
        }
    }
    
    // Render current pattern with optimized buffer management
    renderCurrentPattern();
}

/** Helper function to parse key=value pairs from OWO file */
bool parseKeyValue(const char* line, char* key, char* value) {
    const char* equals = strchr(line, '=');
    if(!equals) return false;
    
    // Extract key
    char* key_start = (char*)line;
    while(*key_start == ' ' || *key_start == '\t') key_start++;
    char* key_end = (char*)equals - 1;
    while(key_end > key_start && (*key_end == ' ' || *key_end == '\t')) key_end--;
    
    int key_len = key_end - key_start + 1;
    strncpy(key, key_start, key_len);
    key[key_len] = '\0';
    
    // Extract value
    char* value_start = (char*)equals + 1;
    while(*value_start == ' ' || *value_start == '\t') value_start++;
    
    // Remove quotes if present
    if(*value_start == '"') {
        value_start++;
        char* quote_end = strrchr(value_start, '"');
        if(quote_end) *quote_end = '\0';
    }
    
    strcpy(value, value_start);
    
    // Remove trailing whitespace and newlines
    int len = strlen(value) - 1;
    while(len >= 0 && (value[len] == ' ' || value[len] == '\t' || value[len] == '\n' || value[len] == '\r')) {
        value[len--] = '\0';
    }
    
    return true;
}

/** Initialize default configuration - EXACTLY like HUB75RGBTest.cpp working setup */
void initializeDefaultConfig() {
    ESP_LOGW(TAG, "*** USING DEFAULT CONFIG - SD CARD FAILED ***");
    ESP_LOGW(TAG, "*** DEFAULT CONFIG: 128x32 dual display, brightness:255, all patterns enabled ***");
    
    // Use WORKING configuration from HUB75RGBTest.cpp
    config.width = 128;         // Full dual panel width
    config.height = 32;         // Panel height
    config.panels = 2;          // Dual panel mode
    config.brightness = 255;    // Full brightness
    config.gamma_correction = true;   // Enable gamma like HUB75RGBTest.cpp
    config.gamma_value = 2.2f;        // Same gamma value
    
    config.speed_multiplier = 2.0f;
    config.auto_pattern_switch = true;   // Enable pattern switching
    config.pattern_switch_interval = 8.0f;
    
    // Enable ALL patterns for default
    config.rainbow_wave_enabled = true;
    config.plasma_field_enabled = true;
    config.matrix_rain_enabled = true;
    config.fire_simulation_enabled = true;
    config.spiral_galaxy_enabled = true;
    
    // Pattern parameters
    config.wave_frequency = 0.08f;
    config.saturation = 100.0f;
    config.lightness = 60.0f;
    config.plasma_speed = 2.5f;
    config.plasma_scale = 0.15f;
    config.drop_probability = 0.02f;
    config.rain_hue = 120.0f;
    config.cooling_factor = 0.55f;
    config.spiral_arms = 3;
    config.rotation_speed = 1.2f;
    config.center_x = 64;   // Center of 128-wide display
    config.center_y = 16;   // Center of 32-high display
    
    // Hardware pins - EXACT same as HUB75RGBTest.cpp
    config.r0_pin = 7;   config.g0_pin = 15;  config.b0_pin = 16;
    config.r1_pin = 17;  config.g1_pin = 18;  config.b1_pin = 8;
    config.a_pin = 41;   config.b_pin = 40;   config.c_pin = 39;
    config.d_pin = 38;   config.e_pin = 42;   config.lat_pin = 36;
    config.oe_pin = 35;  config.oe_pin2 = 6;  config.clock_pin = 37;
    
    config.frame_time_ms = 10;  // 60 FPS for ultra-smooth animations (like HUB75RGBTest.cpp)
}

/** Load configuration from SD card OWO file (simplified parser) */
bool loadConfigFromSDCard() {
    ESP_LOGI(TAG, "Attempting to load config from SD card...");
    
    const char* filename = "/sdcard/config_test.owo";
    ESP_LOGI(TAG, "Looking for file: %s", filename);
    
    FILE* file = fopen(filename, "r");
    if(!file) {
        ESP_LOGW(TAG, "File config_test.owo not found on SD card");
        ESP_LOGI(TAG, "Make sure config_test.owo is in the root directory of your SD card");
        return false;
    }
    
    char line[256];
    char current_section[64] = "";
    char key[64], value[128];
    
    while(fgets(line, sizeof(line), file)) {
        // Remove trailing newline
        line[strcspn(line, "\r\n")] = 0;
        
        // Skip empty lines and comments
        if(strlen(line) == 0 || line[0] == '#') continue;
        
        // Parse section headers
        if(line[0] == '[' && line[strlen(line)-1] == ']') {
            strncpy(current_section, line + 1, strlen(line) - 2);
            current_section[strlen(line) - 2] = '\0';
            continue;
        }
        
        // Parse key=value pairs
        if(!parseKeyValue(line, key, value)) continue;
        
        // Parse only essential configuration values
        if(strcmp(current_section, "display") == 0) {
            if(strcmp(key, "width") == 0) config.width = atoi(value);
            else if(strcmp(key, "height") == 0) config.height = atoi(value);
            else if(strcmp(key, "brightness") == 0) config.brightness = atoi(value);
            else if(strcmp(key, "gamma_correction") == 0) config.gamma_correction = (strcmp(value, "true") == 0);
        }
        else if(strcmp(current_section, "animation.global") == 0) {
            if(strcmp(key, "auto_pattern_switch") == 0) config.auto_pattern_switch = (strcmp(value, "true") == 0);
            else if(strcmp(key, "pattern_switch_interval") == 0) config.pattern_switch_interval = atof(value);
        }
        else if(strcmp(current_section, "pattern.rainbow_wave") == 0) {
            if(strcmp(key, "enabled") == 0) config.rainbow_wave_enabled = (strcmp(value, "true") == 0);
        }
        else if(strcmp(current_section, "pattern.plasma_field") == 0) {
            if(strcmp(key, "enabled") == 0) config.plasma_field_enabled = (strcmp(value, "true") == 0);
        }
        else if(strcmp(current_section, "pattern.matrix_rain") == 0) {
            if(strcmp(key, "enabled") == 0) config.matrix_rain_enabled = (strcmp(value, "true") == 0);
        }
        else if(strcmp(current_section, "pattern.fire_simulation") == 0) {
            if(strcmp(key, "enabled") == 0) config.fire_simulation_enabled = (strcmp(value, "true") == 0);
        }
        else if(strcmp(current_section, "pattern.spiral_galaxy") == 0) {
            if(strcmp(key, "enabled") == 0) config.spiral_galaxy_enabled = (strcmp(value, "true") == 0);
        }
    }
    
    fclose(file);
    ESP_LOGI(TAG, "✅ SD CARD SUCCESS: Loaded config from %s", filename);
    ESP_LOGI(TAG, "✅ Config: %dx%d display, brightness:%d, all patterns from OWO file", 
             config.width, config.height, config.brightness);
    return true;
}

extern "C" void app_main() {
    // Startup delay and logging
    vTaskDelay(pdMS_TO_TICKS(2000));
    printf("\n\n\n*** ESP32 BOOTED - ANIMATED HUB75 STARTING ***\n\n\n");
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "=== ARCOS Animated HUB75 Matrix ===");
    ESP_LOGI(TAG, "Loading patterns from config_test.owo");
    ESP_LOGI(TAG, "Fast trigonometry initialized for high-performance animations");
    
    // Initialize SD card
    ESP_LOGI(TAG, "Initializing SD card...");
    ESP_LOGI(TAG, "SD Card SPI Configuration:");
    ESP_LOGI(TAG, "  MOSI: GPIO 21");
    ESP_LOGI(TAG, "  MISO: GPIO 48");
    ESP_LOGI(TAG, "  SCK:  GPIO 47");
    ESP_LOGI(TAG, "  CS:   GPIO 14");
    
    ESP_LOGI(TAG, "Attempting to mount SD card...");
    
    bool sd_success = false;
    auto sd_result = sd_card.initialize();
    if(sd_result == SdCardResult::Success) {
        sd_success = true;
        ESP_LOGI(TAG, "SD card mounted successfully!");
        
        // Try to load configuration from SD card
        if(!loadConfigFromSDCard()) {
            ESP_LOGW(TAG, "❌ SD CARD FAILED: Using default configuration");
            initializeDefaultConfig();
        }
    } else {
        ESP_LOGW(TAG, "❌ SD CARD FAILED: Using default configuration");
        initializeDefaultConfig();
    }
    
    // Configure HUB75 display - EXACTLY like HUB75RGBTest.cpp
    HUB75Config hub75_config = HUB75Config::getDefault();
    hub75_config.enable_gamma_correction = config.gamma_correction;
    hub75_config.gamma_value = config.gamma_value;
    
    // Dual display configuration based on config width
    if(config.width == 128) {
        // Full dual panel configuration - EXACTLY like HUB75RGBTest.cpp
        hub75_config.dual_display_mode = true;        // Enable dual display spillover
        hub75_config.effective_width = 128;           // 64x2 = 128 pixels wide
        
        // Panel inversion: Flip panel 0 vertically (EXACT same as HUB75RGBTest.cpp)
        hub75_config.panel_inversions[0].flip_vertical = true;   // Panel 0: flip upside down
        hub75_config.panel_inversions[1].flip_vertical = false;  // Panel 1: normal orientation
        
        ESP_LOGI(TAG, "Using dual panel configuration (128x32) - EXACTLY like HUB75RGBTest.cpp");
    } else {
        // Single panel mode
        hub75_config.dual_display_mode = false;
        hub75_config.effective_width = 64;
        
        ESP_LOGI(TAG, "Using single panel configuration (64x32)");
    }
    
    // Pin configuration - EXACT same as HUB75RGBTest.cpp
    hub75_config.pins.r0_pin = config.r0_pin;
    hub75_config.pins.g0_pin = config.g0_pin;
    hub75_config.pins.b0_pin = config.b0_pin;
    hub75_config.pins.r1_pin = config.r1_pin;
    hub75_config.pins.g1_pin = config.g1_pin;
    hub75_config.pins.b1_pin = config.b1_pin;
    hub75_config.pins.a_pin = config.a_pin;
    hub75_config.pins.b_pin = config.b_pin;
    hub75_config.pins.c_pin = config.c_pin;
    hub75_config.pins.d_pin = config.d_pin;
    hub75_config.pins.e_pin = config.e_pin;
    hub75_config.pins.lat_pin = config.lat_pin;
    hub75_config.pins.oe_pin = config.oe_pin;
    hub75_config.pins.oe_pin2 = config.oe_pin2;  // CRITICAL: Secondary OE pin for dual display
    hub75_config.pins.clock_pin = config.clock_pin;
    
    // Initialize display (EXACT same sequence as HUB75RGBTest.cpp)
    int buffer_size = HUB75Driver::calculateBufferSize(hub75_config);
    ESP_LOGI(TAG, "Calculated buffer size: %d samples (%d KB)", 
             buffer_size, (buffer_size * 2) / 1024);
    
    if(!i2sProtocol.init(hub75_config, buffer_size, &hardware, &bufferManager)) {
        ESP_LOGE(TAG, "Failed to initialize I2S protocol");
        return;
    }
    
    if(!display.init(hub75_config, &i2sProtocol)) {
        ESP_LOGE(TAG, "Failed to initialize display");
        return;
    }
    
    if(!display.start()) {
        ESP_LOGE(TAG, "Failed to start display");
        return;
    }
    
    ESP_LOGI(TAG, "Display initialized: %dx%d pixels", display.getWidth(), display.getHeight());
    ESP_LOGI(TAG, "Available patterns:");
    ESP_LOGI(TAG, "  0: Rainbow Wave (%s)", config.rainbow_wave_enabled ? "enabled" : "disabled");
    ESP_LOGI(TAG, "  1: Plasma Field (%s)", config.plasma_field_enabled ? "enabled" : "disabled");
    ESP_LOGI(TAG, "  2: Matrix Rain (%s)", config.matrix_rain_enabled ? "enabled" : "disabled");
    ESP_LOGI(TAG, "  3: Fire Simulation (%s)", config.fire_simulation_enabled ? "enabled" : "disabled");
    ESP_LOGI(TAG, "  4: Spiral Galaxy (%s)", config.spiral_galaxy_enabled ? "enabled" : "disabled");
    ESP_LOGI(TAG, "Starting animation loop...");
    
    last_pattern_switch = esp_timer_get_time() / 1000000;
    
    // Main animation loop - EXACTLY like HUB75RGBTest.cpp
    while(true) {
        updateAnimation();
        vTaskDelay(pdMS_TO_TICKS(config.frame_time_ms));
    }
}