/*****************************************************************
 * File:      AnimatedHUB75Driver.cpp
 * Category:  tests/abstraction
 * Author:    ARCOS Team
 * 
 * Purpose:
 *    Advanced HUB75 LED matrix driver with configurable animations
 *    loaded from config_test.owo file via SD card integration.
 *    Features multiple pattern types with mathematical equations.
 *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "esp_random.h"

// ARCOS abstraction framework
#include "abstraction/hal.hpp"
#include "abstraction/drivers/components/HUB75/driver_hub75.hpp"
#include "abstraction/drivers/components/HUB75/driver_hub75_i2s.hpp"
#include "abstraction/drivers/components/SD_CARD/driver_sd_card.hpp"

using namespace arcos::abstraction;
using namespace arcos::abstraction::drivers;

static const char* TAG = "ANIMATED_HUB75";

/** Platform implementations */
static HAL_PARALLEL_DEFAULT hardware;
static ParallelBuffer bufferManager;
static HUB75_I2S_Protocol i2sProtocol;
static HUB75Driver display;
static arcos::abstraction::DRIVER_SD_CARD sd_card;

/** Configuration structure from OWO file */
struct AnimationConfig {
    // Display settings
    int width, height, panels;
    uint8_t brightness;
    bool gamma_correction;
    float gamma_value;
    int refresh_rate;
    
    // Animation globals
    float speed_multiplier;
    float time_scale;
    float pattern_cycle_time;
    bool brightness_fade;
    bool auto_pattern_switch;
    float pattern_switch_interval;
    
    // Pattern enables
    bool rainbow_wave_enabled;
    bool plasma_field_enabled;
    bool matrix_rain_enabled;
    bool fire_simulation_enabled;
    bool spiral_galaxy_enabled;
    
    // Rainbow wave parameters
    float wave_frequency;
    float wave_amplitude;
    float hue_shift_speed;
    float saturation;
    float lightness;
    
    // Plasma field parameters
    float plasma_speed;
    float plasma_scale;
    float color_shift;
    
    // Matrix rain parameters
    float drop_probability;
    float fade_speed;
    int trail_length;
    float rain_hue;
    float rain_saturation;
    
    // Fire simulation parameters
    float cooling_factor;
    float spark_probability;
    float wind_effect;
    int flame_height;
    int base_temperature;
    
    // Spiral galaxy parameters
    int spiral_arms;
    float rotation_speed;
    float arm_width;
    int center_x, center_y;
    
    // Effects
    bool breathing_enabled;
    float breath_speed;
    int min_brightness, max_brightness;
    
    // Timing
    int fps_target;
    int frame_time_ms;
    float animation_step;
    
    // Hardware pins
    int r0_pin, g0_pin, b0_pin, r1_pin, g1_pin, b1_pin;
    int a_pin, b_pin, c_pin, d_pin, e_pin;
    int lat_pin, oe_pin, oe_pin2, clock_pin;
};

static AnimationConfig config;
static bool config_loaded = false;
static int current_pattern = 0;
static float animation_time = 0.0f;
static uint64_t last_pattern_switch = 0;

/** Color structures */
struct CRGB {
    uint8_t r, g, b;
    CRGB() : r(0), g(0), b(0) {}
    CRGB(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
    
    CRGB operator+(const CRGB& other) const {
        return CRGB(
            (uint8_t)fminf(255, r + other.r),
            (uint8_t)fminf(255, g + other.g),
            (uint8_t)fminf(255, b + other.b)
        );
    }
    
    CRGB operator*(float scale) const {
        return CRGB(
            (uint8_t)(r * scale),
            (uint8_t)(g * scale),
            (uint8_t)(b * scale)
        );
    }
};

/** Matrix rain drops */
struct RainDrop {
    int x, y;
    float speed;
    int trail_pos;
    bool active;
    char character;
};

static RainDrop rain_drops[128];  // Max drops for full width
static uint8_t fire_heat[128 * 32];  // Heat map for fire simulation

/** Math utility functions */
float fastSin(float x) {
    return sinf(x);
}

float fastCos(float x) {
    return cosf(x);
}

float noise2D(float x, float y) {
    // Simple pseudo-random noise function
    int n = (int)(x + y * 57.0f);
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

/** HSL to RGB conversion */
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

/** Heat to color conversion for fire effect */
CRGB heatToColor(uint8_t temperature) {
    uint8_t t192 = (uint8_t)((temperature / 255.0f) * 191);
    
    uint8_t heatramp = t192 & 0x3F;  // 0..63
    heatramp <<= 2;  // Scale to 0..252
    
    if(t192 & 0x80) {
        // Hottest - white
        return CRGB(255, 255, heatramp);
    } else if(t192 & 0x40) {
        // Hot - yellow to white
        return CRGB(255, heatramp, 0);
    } else {
        // Cool - red to yellow
        return CRGB(heatramp, 0, 0);
    }
}

/** Initialize SD card */
bool initializeSDCard() {
    ESP_LOGI(TAG, "Initializing SD card...");
    
    arcos::abstraction::SdCardConfig sd_config;
    sd_config.spi_host = 1;
    sd_config.pin_mosi = 21;
    sd_config.pin_miso = 48;
    sd_config.pin_sck = 47;
    sd_config.pin_cs = 14;
    sd_config.max_frequency_hz = 20000000;
    sd_config.max_open_files = 5;
    sd_config.format_if_failed = false;
    sd_config.mount_point = "/sdcard";
    
    if(!arcos::abstraction::DRIVER_SD_CARD::validateConfig(sd_config)) {
        ESP_LOGE(TAG, "SD card configuration validation failed!");
        return false;
    }
    
    arcos::abstraction::SdCardResult result = sd_card.initialize(sd_config);
    if(result != arcos::abstraction::SdCardResult::Success) {
        ESP_LOGI(TAG, "SD card initialization failed: %s", 
                 arcos::abstraction::DRIVER_SD_CARD::getErrorString(result));
        return false;
    }
    
    ESP_LOGI(TAG, "SD card initialized successfully!");
    return true;
}

/** Parse configuration line */
bool parseConfigLine(const char* line, char* section, char* key, char* value) {
    // Skip empty lines and comments
    if(!line || line[0] == '\0' || line[0] == '#' || line[0] == ';') {
        return false;
    }
    
    // Check for section header
    if(line[0] == '[') {
        char* end = strchr(line, ']');
        if(end) {
            int len = end - line - 1;
            strncpy(section, line + 1, len);
            section[len] = '\0';
            return false;  // Section header, not a key-value pair
        }
    }
    
    // Parse key-value pair
    char* equals = strchr((char*)line, '=');
    if(!equals) return false;
    
    // Extract key
    char* key_start = (char*)line;
    while(*key_start == ' ' || *key_start == '\t') key_start++;
    char* key_end = equals - 1;
    while(key_end > key_start && (*key_end == ' ' || *key_end == '\t')) key_end--;
    
    int key_len = key_end - key_start + 1;
    strncpy(key, key_start, key_len);
    key[key_len] = '\0';
    
    // Extract value
    char* value_start = equals + 1;
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

/** Load configuration from OWO file */
bool loadConfigFromSD() {
    ESP_LOGI(TAG, "Attempting to load config from SD card...");
    
    FILE* file = fopen("/sdcard/config_test.owo", "r");
    if(!file) {
        ESP_LOGW(TAG, "Could not open config_test.owo from SD card");
        return false;
    }
    
    // Initialize config with defaults
    config.width = 128;
    config.height = 32;
    config.panels = 2;
    config.brightness = 200;
    config.gamma_correction = true;
    config.gamma_value = 2.2f;
    config.refresh_rate = 60;
    
    config.speed_multiplier = 1.0f;
    config.time_scale = 1.0f;
    config.pattern_cycle_time = 30.0f;
    config.brightness_fade = true;
    config.auto_pattern_switch = true;
    config.pattern_switch_interval = 15.0f;
    
    config.rainbow_wave_enabled = true;
    config.plasma_field_enabled = true;
    config.matrix_rain_enabled = true;
    config.fire_simulation_enabled = true;
    config.spiral_galaxy_enabled = true;
    
    // Rainbow wave defaults
    config.wave_frequency = 0.05f;
    config.wave_amplitude = 64.0f;
    config.hue_shift_speed = 2.0f;
    config.saturation = 100.0f;
    config.lightness = 50.0f;
    
    // Plasma field defaults
    config.plasma_speed = 1.5f;
    config.plasma_scale = 0.1f;
    config.color_shift = 3.0f;
    
    // Matrix rain defaults
    config.drop_probability = 0.02f;
    config.fade_speed = 0.95f;
    config.trail_length = 8;
    config.rain_hue = 120.0f;
    config.rain_saturation = 100.0f;
    
    // Fire simulation defaults
    config.cooling_factor = 0.55f;
    config.spark_probability = 0.1f;
    config.wind_effect = 0.3f;
    config.flame_height = 16;
    config.base_temperature = 80;
    
    // Spiral galaxy defaults
    config.spiral_arms = 3;
    config.rotation_speed = 1.2f;
    config.arm_width = 8.0f;
    config.center_x = 64;
    config.center_y = 16;
    
    // Hardware pin defaults
    config.r0_pin = 7; config.g0_pin = 15; config.b0_pin = 16;
    config.r1_pin = 17; config.g1_pin = 18; config.b1_pin = 8;
    config.a_pin = 41; config.b_pin = 40; config.c_pin = 39;
    config.d_pin = 38; config.e_pin = 42; config.lat_pin = 36;
    config.oe_pin = 35; config.oe_pin2 = 6; config.clock_pin = 37;
    
    char line[256];
    char section[64] = "";
    char key[64], value[128];
    
    while(fgets(line, sizeof(line), file)) {
        if(parseConfigLine(line, section, key, value)) {
            // Parse based on section
            if(strcmp(section, "display") == 0) {
                if(strcmp(key, "width") == 0) config.width = atoi(value);
                else if(strcmp(key, "height") == 0) config.height = atoi(value);
                else if(strcmp(key, "brightness") == 0) config.brightness = atoi(value);
                else if(strcmp(key, "gamma_correction") == 0) config.gamma_correction = (strcmp(value, "true") == 0);
                else if(strcmp(key, "gamma_value") == 0) config.gamma_value = atof(value);
            }
            else if(strcmp(section, "animation.global") == 0) {
                if(strcmp(key, "speed_multiplier") == 0) config.speed_multiplier = atof(value);
                else if(strcmp(key, "time_scale") == 0) config.time_scale = atof(value);
                else if(strcmp(key, "pattern_switch_interval") == 0) config.pattern_switch_interval = atof(value);
            }
            else if(strcmp(section, "pattern.rainbow_wave") == 0) {
                if(strcmp(key, "enabled") == 0) config.rainbow_wave_enabled = (strcmp(value, "true") == 0);
                else if(strcmp(key, "wave_frequency") == 0) config.wave_frequency = atof(value);
                else if(strcmp(key, "wave_amplitude") == 0) config.wave_amplitude = atof(value);
                else if(strcmp(key, "hue_shift_speed") == 0) config.hue_shift_speed = atof(value);
                else if(strcmp(key, "saturation") == 0) config.saturation = atof(value);
                else if(strcmp(key, "lightness") == 0) config.lightness = atof(value);
            }
            else if(strcmp(section, "pattern.plasma_field") == 0) {
                if(strcmp(key, "enabled") == 0) config.plasma_field_enabled = (strcmp(value, "true") == 0);
                else if(strcmp(key, "plasma_speed") == 0) config.plasma_speed = atof(value);
                else if(strcmp(key, "plasma_scale") == 0) config.plasma_scale = atof(value);
                else if(strcmp(key, "color_shift") == 0) config.color_shift = atof(value);
            }
            else if(strcmp(section, "pattern.matrix_rain") == 0) {
                if(strcmp(key, "enabled") == 0) config.matrix_rain_enabled = (strcmp(value, "true") == 0);
                else if(strcmp(key, "drop_probability") == 0) config.drop_probability = atof(value);
                else if(strcmp(key, "fade_speed") == 0) config.fade_speed = atof(value);
                else if(strcmp(key, "trail_length") == 0) config.trail_length = atoi(value);
                else if(strcmp(key, "color_hue") == 0) config.rain_hue = atof(value);
            }
            else if(strcmp(section, "pattern.fire_simulation") == 0) {
                if(strcmp(key, "enabled") == 0) config.fire_simulation_enabled = (strcmp(value, "true") == 0);
                else if(strcmp(key, "cooling_factor") == 0) config.cooling_factor = atof(value);
                else if(strcmp(key, "spark_probability") == 0) config.spark_probability = atof(value);
                else if(strcmp(key, "flame_height") == 0) config.flame_height = atoi(value);
            }
            else if(strcmp(section, "pattern.spiral_galaxy") == 0) {
                if(strcmp(key, "enabled") == 0) config.spiral_galaxy_enabled = (strcmp(value, "true") == 0);
                else if(strcmp(key, "spiral_arms") == 0) config.spiral_arms = atoi(value);
                else if(strcmp(key, "rotation_speed") == 0) config.rotation_speed = atof(value);
                else if(strcmp(key, "center_x") == 0) config.center_x = atoi(value);
                else if(strcmp(key, "center_y") == 0) config.center_y = atoi(value);
            }
            else if(strcmp(section, "hardware.pins") == 0) {
                if(strcmp(key, "r0_pin") == 0) config.r0_pin = atoi(value);
                else if(strcmp(key, "g0_pin") == 0) config.g0_pin = atoi(value);
                else if(strcmp(key, "b0_pin") == 0) config.b0_pin = atoi(value);
                // Add other pins as needed...
            }
        }
    }
    
    fclose(file);
    ESP_LOGI(TAG, "Configuration loaded successfully from SD card!");
    return true;
}

/** Rainbow wave pattern */
void renderRainbowWave() {
    for(int y = 0; y < config.height; y++) {
        for(int x = 0; x < config.width; x++) {
            float wave = fastSin(x * config.wave_frequency + animation_time * config.hue_shift_speed);
            float hue = (wave * config.wave_amplitude + animation_time * config.color_shift) + (x * 360.0f / config.width);
            
            CRGB color = hslToRgb(hue, config.saturation, config.lightness);
            display.setPixel(x, y, RGB(color.r, color.g, color.b));
        }
    }
}

/** Plasma field pattern */
void renderPlasmaField() {
    for(int y = 0; y < config.height; y++) {
        for(int x = 0; x < config.width; x++) {
            float v1 = fastSin((x + animation_time * config.plasma_speed) * config.plasma_scale);
            float v2 = fastSin((y + animation_time * config.plasma_speed * 0.7f) * config.plasma_scale);
            float v3 = fastSin(sqrtf(x*x + y*y) * config.plasma_scale + animation_time * config.plasma_speed);
            
            float plasma = (v1 + v2 + v3) / 3.0f;
            float hue = (plasma + 1.0f) * 180.0f + animation_time * config.color_shift;
            
            CRGB color = hslToRgb(hue, 100.0f, 50.0f);
            display.setPixel(x, y, RGB(color.r, color.g, color.b));
        }
    }
}

/** Matrix rain pattern */
void renderMatrixRain() {
    // Clear screen with fade
    for(int y = 0; y < config.height; y++) {
        for(int x = 0; x < config.width; x++) {
            display.setPixel(x, y, RGB(0, 0, 0));
        }
    }
    
    // Update rain drops
    for(int i = 0; i < config.width; i++) {
        RainDrop& drop = rain_drops[i];
        
        // Spawn new drop
        if(!drop.active && (esp_random() / (float)UINT32_MAX) < config.drop_probability) {
            drop.active = true;
            drop.x = i;
            drop.y = -config.trail_length;
            drop.speed = 0.5f + (esp_random() / (float)UINT32_MAX) * 1.0f;
            drop.trail_pos = 0;
        }
        
        // Update active drop
        if(drop.active) {
            drop.y += drop.speed;
            
            // Draw trail
            for(int t = 0; t < config.trail_length; t++) {
                int trail_y = drop.y - t;
                if(trail_y >= 0 && trail_y < config.height) {
                    float intensity = (config.trail_length - t) / (float)config.trail_length;
                    CRGB color = hslToRgb(config.rain_hue, config.rain_saturation, 50.0f * intensity);
                    display.setPixel(drop.x, trail_y, RGB(color.r, color.g, color.b));
                }
            }
            
            // Remove if off screen
            if(drop.y >= config.height + config.trail_length) {
                drop.active = false;
            }
        }
    }
}

/** Fire simulation pattern */
void renderFireSimulation() {
    // Cool down heat map
    for(int x = 0; x < config.width; x++) {
        for(int y = 0; y < config.height; y++) {
            int index = x + y * config.width;
            float cooling = config.cooling_factor * ((esp_random() / (float)UINT32_MAX) * 0.4f + 0.8f);
            fire_heat[index] = (uint8_t)fmaxf(0, fire_heat[index] - cooling * 255);
        }
    }
    
    // Add sparks at bottom
    for(int x = 0; x < config.width; x++) {
        if((esp_random() / (float)UINT32_MAX) < config.spark_probability) {
            int index = x + (config.height - 1) * config.width;
            fire_heat[index] = 255;
        }
    }
    
    // Heat propagation upward
    for(int x = 0; x < config.width; x++) {
        for(int y = config.height - 2; y >= 0; y--) {
            int index = x + y * config.width;
            int below = x + (y + 1) * config.width;
            
            fire_heat[index] = (fire_heat[below] + fire_heat[index]) / 2;
        }
    }
    
    // Render heat map to colors
    for(int x = 0; x < config.width; x++) {
        for(int y = 0; y < config.height; y++) {
            int index = x + y * config.width;
            CRGB color = heatToColor(fire_heat[index]);
            display.setPixel(x, y, RGB(color.r, color.g, color.b));
        }
    }
}

/** Spiral galaxy pattern */
void renderSpiralGalaxy() {
    for(int y = 0; y < config.height; y++) {
        for(int x = 0; x < config.width; x++) {
            float dx = x - config.center_x;
            float dy = y - config.center_y;
            float r = sqrtf(dx*dx + dy*dy);
            float theta = atan2f(dy, dx) + animation_time * config.rotation_speed * 0.1f;
            
            float spiral = fastSin(theta * config.spiral_arms + r * 0.1f - animation_time * config.rotation_speed);
            float intensity = fmaxf(0, spiral) * expf(-r * 0.05f);
            
            float hue = theta * 57.2958f + animation_time * 20.0f;  // Convert to degrees
            CRGB color = hslToRgb(hue, 80.0f, intensity * 80.0f);
            
            display.setPixel(x, y, RGB(color.r, color.g, color.b));
        }
    }
}

/** Main animation update function */
void updateAnimation() {
    animation_time += 0.05f * config.speed_multiplier;
    
    // Auto-switch patterns if enabled
    if(config.auto_pattern_switch) {
        uint64_t current_time = esp_timer_get_time() / 1000000;
        if(current_time - last_pattern_switch > config.pattern_switch_interval) {
            current_pattern = (current_pattern + 1) % 5;
            last_pattern_switch = current_time;
            ESP_LOGI(TAG, "Switched to pattern %d", current_pattern);
        }
    }
    
    // Clear display
    display.clearScreen();
    
    // Render current pattern
    switch(current_pattern) {
        case 0:
            if(config.rainbow_wave_enabled) renderRainbowWave();
            break;
        case 1:
            if(config.plasma_field_enabled) renderPlasmaField();
            break;
        case 2:
            if(config.matrix_rain_enabled) renderMatrixRain();
            break;
        case 3:
            if(config.fire_simulation_enabled) renderFireSimulation();
            break;
        case 4:
            if(config.spiral_galaxy_enabled) renderSpiralGalaxy();
            break;
    }
    
    // Apply breathing effect if enabled
    if(config.breathing_enabled) {
        float breath = fastSin(animation_time * config.breath_speed) * 0.5f + 0.5f;
        uint8_t brightness = config.min_brightness + (config.max_brightness - config.min_brightness) * breath;
        display.setBrightness(brightness);
    } else {
        display.setBrightness(config.brightness);
    }
    
    display.show();
}

extern "C" void app_main() {
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "=== ARCOS Animated HUB75 Matrix ===");
    ESP_LOGI(TAG, "Loading patterns from config_test.owo");
    
    // Disable watchdog
    esp_task_wdt_deinit();
    
    // Initialize SD card and load config
    bool sd_available = initializeSDCard();
    if(sd_available && loadConfigFromSD()) {
        config_loaded = true;
        ESP_LOGI(TAG, "Configuration loaded from SD card successfully!");
    } else {
        ESP_LOGW(TAG, "Using default configuration");
        config_loaded = false;
    }
    
    // Initialize rain drops and fire heat map
    memset(rain_drops, 0, sizeof(rain_drops));
    memset(fire_heat, 0, sizeof(fire_heat));
    
    // Configure HUB75 display
    HUB75Config hub75_config = HUB75Config::getDefault();
    hub75_config.enable_gamma_correction = config.gamma_correction;
    hub75_config.gamma_value = config.gamma_value;
    hub75_config.dual_display_mode = true;
    hub75_config.effective_width = config.width;
    
    // Panel inversions
    hub75_config.panel_inversions[0].flip_vertical = true;
    hub75_config.panel_inversions[1].flip_vertical = false;
    
    // Pin configuration from OWO file
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
    hub75_config.pins.oe_pin2 = config.oe_pin2;
    hub75_config.pins.clock_pin = config.clock_pin;
    
    // Initialize display
    int buffer_size = HUB75Driver::calculateBufferSize(hub75_config);
    ESP_LOGI(TAG, "Buffer size: %d samples", buffer_size);
    
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
    
    ESP_LOGI(TAG, "Display initialized: %dx%d", config.width, config.height);
    ESP_LOGI(TAG, "Available patterns:");
    ESP_LOGI(TAG, "  0: Rainbow Wave (%s)", config.rainbow_wave_enabled ? "enabled" : "disabled");
    ESP_LOGI(TAG, "  1: Plasma Field (%s)", config.plasma_field_enabled ? "enabled" : "disabled");
    ESP_LOGI(TAG, "  2: Matrix Rain (%s)", config.matrix_rain_enabled ? "enabled" : "disabled");
    ESP_LOGI(TAG, "  3: Fire Simulation (%s)", config.fire_simulation_enabled ? "enabled" : "disabled");
    ESP_LOGI(TAG, "  4: Spiral Galaxy (%s)", config.spiral_galaxy_enabled ? "enabled" : "disabled");
    ESP_LOGI(TAG, "Starting animation loop...");
    
    last_pattern_switch = esp_timer_get_time() / 1000000;
    
    // Main animation loop
    while(true) {
        updateAnimation();
        vTaskDelay(pdMS_TO_TICKS(config.frame_time_ms));
    }
}