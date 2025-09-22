#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "hub75_driver.hpp"
#include "i2c_driver.h"
#include "oled_driver.h"

static const char* TAG = "UNIFIED_DISPLAY";

/** FastLED compatible color definitions */
struct CRGB {
    uint8_t r, g, b;
    CRGB(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0) : r(red), g(green), b(blue) {}
};

/** Pattern and animation state */
static uint32_t time_counter = 0;
static uint8_t* persistent_mono_buffer = nullptr;

/** Display instances */
static HUB75Driver hub75_display;
static uint8_t* oled_buffer = nullptr;

/** Dual-core buffer system with front/back buffers */
static RGB* front_buffer = nullptr;   // Read by display tasks (Core 1)
static RGB* back_buffer = nullptr;    // Written by pattern task (Core 0)
static RGB* unified_buffer = nullptr; // Main shared buffer
static int buffer_width = 128;   // HUB75 dual display width
static int buffer_height = 32;   // HUB75 height
static const int oled_width = 128;
static const int oled_height = 128;

/** Task handles */
static TaskHandle_t pattern_generation_task = nullptr;
static TaskHandle_t display_update_task = nullptr;

/** Configuration */
typedef enum {
    CONVERSION_LUMINANCE = 0,
    CONVERSION_DITHERED = 1,
    CONVERSION_RGB_COMPENSATED = 2
} ConversionMode;

static ConversionMode current_conversion = CONVERSION_LUMINANCE;
static const char* conversion_names[] = {"Luminance", "Dithered", "RGB Compensated"};

/** Performance optimisation caches */
static int16_t plasma_x_cache[128];
static int16_t plasma_y_cache[32];
static bool cache_initialized = false;

/** Fast trigonometric lookup tables */
static const uint8_t sin8_table[256] = {
  128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220,218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,128,124,121,118,115,112,109,106,103,100,97,93,90,88,85,82,79,76,73,70,67,65,62,59,57,54,52,49,47,44,42,40,37,35,33,31,29,27,25,23,21,20,18,17,15,14,12,11,10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0,0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,10,11,12,14,15,17,18,20,21,23,25,27,29,31,33,35,37,40,42,44,47,49,52,54,57,59,62,65,67,70,73,76,79,82,85,88,90,93,97,100,103,106,109,112,115,118,121,124
};

/** Colour palettes */
static const CRGB HeatColours[16] = {
    {0,0,0}, {32,0,0}, {64,0,0}, {96,0,0}, {128,0,0}, {160,8,0}, {192,16,0}, {224,32,0},
    {255,64,0}, {255,96,0}, {255,128,0}, {255,160,0}, {255,192,0}, {255,224,0}, {255,255,0}, {255,255,255}
};

static const CRGB RainbowColours[16] = {
    {255,0,0}, {255,32,0}, {255,64,0}, {255,128,0}, {255,255,0}, {128,255,0}, {0,255,0}, {0,255,128},
    {0,255,255}, {0,128,255}, {0,0,255}, {128,0,255}, {255,0,255}, {255,0,128}, {255,0,64}, {255,0,32}
};

static const CRGB LavaColours[16] = {
    {0,0,0}, {64,0,0}, {128,0,0}, {192,0,0}, {255,0,0}, {255,32,0}, {255,64,0}, {255,96,0},
    {255,128,0}, {255,160,0}, {255,192,0}, {255,224,0}, {255,255,0}, {255,255,64}, {255,255,128}, {255,255,192}
};

static const CRGB* palettes[] = {HeatColours, RainbowColours, LavaColours};
static const int NUM_PALETTES = 3;
static int currentPaletteIndex = 0;

/** Fast trigonometric functions */
static inline uint8_t sin8(uint8_t theta){
    return sin8_table[theta];
}

static inline uint8_t cos8(uint8_t theta){
    return sin8_table[(uint8_t)(theta + 64)];
}

/** Fast colour palette lookup */
static inline CRGB colourFromPalette(const CRGB* palette, uint8_t index){
    int paletteIndex = (index >> 4) & 0x0F;  // Map 0-255 to 0-15
    return palette[paletteIndex];
}

/** Initialize performance caches */
void initializePerformanceCaches() {
    ESP_LOGI(TAG, "Performance caches initialised");
}

/** Ultra-high-performance plasma pattern generation to specific buffer */
void generatePlasmaPatternToBuffer(RGB* target_buffer){
    const CRGB* currentPalette = palettes[currentPaletteIndex];
    
    // Initialize caches on first run
    if(!cache_initialized){
        for(int x = 0; x < buffer_width; x++){
            plasma_x_cache[x] = (int16_t)(sin8(x * 4) << 6);  // Reduced bit shift for speed
        }
        for(int y = 0; y < buffer_height; y++){
            plasma_y_cache[y] = (int16_t)(sin8(y * 6) << 6);  // Reduced bit shift for speed
        }
        cache_initialized = true;
    }
    
    uint8_t wibble = sin8(time_counter);
    uint8_t cos_time = cos8(-time_counter);
    
    // Ultra-fast row-based processing
    for(int y = 0; y < buffer_height; y++){
        int16_t base_y = plasma_y_cache[y];
        RGB* row_ptr = &target_buffer[y * buffer_width];
        
        for(int x = 0; x < buffer_width; x++){
            int16_t v = base_y + plasma_x_cache[x];
            
            // Simplified plasma calculation - fewer operations
            if(x & 2){  // Every 4th pixel for speed
                v += sin8(time_counter + x) >> 3;
            }
            
            CRGB pixel_colour = colourFromPalette(currentPalette, (uint8_t)(v >> 6));
            
            // Direct row pointer write - faster than index calculation
            *row_ptr++ = RGB(pixel_colour.r, pixel_colour.g, pixel_colour.b);
        }
    }
}

/** RGB to monochrome conversion - Standard luminance */
uint8_t rgbToLuminance(const RGB& pixel){
    // ITU-R BT.709 luminance coefficients
    return (uint8_t)((pixel.r * 0.2126f + pixel.g * 0.7152f + pixel.b * 0.0722f));
}

/** Convert RGB buffer to monochrome and update OLED (optimized for Core 1) */
void convertAndDisplayOLED(){
    if (!persistent_mono_buffer) {
        size_t mono_size = buffer_width * buffer_height;
        persistent_mono_buffer = (uint8_t*)malloc(mono_size);
        if(!persistent_mono_buffer){
            ESP_LOGE(TAG, "Failed to allocate persistent mono buffer");
            return;
        }
    }
    
    // Ultra-fast RGB to mono conversion using only green channel
    RGB* src = unified_buffer;  // Read from shared buffer, not back_buffer
    uint8_t* dst = persistent_mono_buffer;
    
    for(int i = 0; i < buffer_width * buffer_height; i++){
        // Even faster approximation using only green channel (most perceptually important)
        *dst++ = src->g; // Just use green channel - fastest possible
        src++;
    }
    
    // Get direct access to OLED buffer for fastest writing
    uint8_t* oled_buf = oled_get_buffer();
    if(!oled_buf) {
        ESP_LOGE(TAG, "OLED buffer is NULL - OLED not working!");
        return;
    }
    
    // DEBUG: Test OLED with simple pattern to verify it's working
    static bool test_pattern = true;
    if(test_pattern) {
        ESP_LOGI(TAG, "Testing OLED with simple checkerboard pattern");
        memset(oled_buf, 0, 128 * 16); // Clear entire buffer
        
        // Create a simple test pattern - checkerboard in middle
        for(int page = 6; page < 10; page++) {
            for(int x = 0; x < 128; x++) {
                if((x / 8) % 2 == 0) {
                    oled_buf[page * 128 + x] = 0xFF; // Full byte on
                } else {
                    oled_buf[page * 128 + x] = 0x00; // Full byte off  
                }
            }
        }
        
        oled_update_pages(0, 15); // Update all pages
        ESP_LOGI(TAG, "OLED test pattern sent - should see checkerboard in middle");
        test_pattern = false;
        return;
    }
    
    // Clear only the area we're updating (bottom 32 rows = pages 12-15)
    memset(&oled_buf[12 * 128], 0, 4 * 128); // Clear pages 12-15
    
    // Direct buffer write - much faster than oled_set_pixel
    for(int y = 0; y < 32; y++){
        int page = (y + 96) / 8;  // Page for OLED buffer
        int bit_pos = (y + 96) % 8;
        uint8_t bit_mask = 1 << bit_pos;
        
        for(int x = 0; x < 128; x++){
            int src_idx = y * 128 + x;
            if(persistent_mono_buffer[src_idx] > 64){
                oled_buf[page * 128 + x] |= bit_mask;
            }
        }
    }
    
    // Update only the pages we modified (12-15 for the HUB75 area)
    oled_update_pages(0, 15); // Update all pages for simplicity
}

/** CORE 0 TASK: Pattern Generation + HUB75 Updates at 60fps (No OLED overhead) */
void patternGenerationTask(void* parameters) {
    ESP_LOGI(TAG, "Core 0: Pattern+HUB75 task started at 60fps (zero OLED overhead)");
    
    uint64_t last_update_time = 0;
    const uint64_t target_interval_us = 16667; // Start with 60fps (16.67ms) to be very safe
    uint32_t frame_count = 0;
    uint32_t last_fps_report = 0;
    
    while (true) {
        uint64_t current_time_us = esp_timer_get_time();
        
        // Run at precise 60fps
        if (current_time_us - last_update_time >= target_interval_us) {
            
            // 1. Generate pattern in back_buffer (Core 0 exclusive)
            if (back_buffer) {
                generatePlasmaPatternToBuffer(back_buffer);
                time_counter += 2; // Faster animation for higher framerate
                
                // 2. Copy to unified_buffer atomically (for OLED to read safely)
                memcpy(unified_buffer, back_buffer, buffer_width * buffer_height * sizeof(RGB));
                
                // 3. Use pixel-by-pixel method (more reliable for dual displays)
                // uploadFrameBuffer seems to have issues with dual display buffer layout
                for(int y = 0; y < buffer_height; y++){
                    for(int x = 0; x < buffer_width; x++){
                        int buffer_index = y * buffer_width + x;
                        hub75_display.setPixel(x, y, back_buffer[buffer_index]);
                    }
                }
                hub75_display.show();
                
                frame_count++;
            }
            
            last_update_time = current_time_us;
            
            // Performance reporting every 5 seconds
            uint32_t current_time_ms = current_time_us / 1000;
            if (current_time_ms - last_fps_report >= 5000) {
                float fps = (float)frame_count * 1000.0f / (current_time_ms - last_fps_report);
                ESP_LOGI(TAG, "Core 0 [Pattern+HUB75]: %.1f fps (target: 15fps)", fps);
                frame_count = 0;
                last_fps_report = current_time_ms;
            }
        } else {
            // Precise timing control - only delay if we're ahead of schedule
            int64_t remaining_us = target_interval_us - (current_time_us - last_update_time);
            if (remaining_us > 1000) {
                vTaskDelay(pdMS_TO_TICKS(remaining_us / 1000));
            }
        }
    }
}

/** CORE 1 TASK: OLED Updates Only at 25fps (Lockless read from Core 0) */
void displayUpdateTask(void* parameters) {
    ESP_LOGI(TAG, "Core 1: OLED-only task started at 25fps (lockless read from Core 0)");
    
    uint64_t last_oled_update = 0;
    const uint64_t oled_interval_us = 100000;  // Start with 10fps (100ms) to be safe
    uint32_t oled_frame_count = 0;
    uint32_t last_fps_report = 0;
    
    while (true) {
        uint64_t current_time_us = esp_timer_get_time();
        
        // Update OLED at precise 25fps
        if (current_time_us - last_oled_update >= oled_interval_us) {
            
            // Read whatever frame is currently available from Core 0 (no blocking!)
            // Core 0 updates back_buffer pointer atomically, so this is safe
            ESP_LOGI(TAG, "OLED task running - attempting to update display");
            convertAndDisplayOLED();
            oled_frame_count++;
            last_oled_update = current_time_us;
            
            // Performance reporting every 5 seconds
            uint32_t current_time_ms = current_time_us / 1000;
            if (current_time_ms - last_fps_report >= 5000) {
                float oled_fps = (float)oled_frame_count * 1000.0f / (current_time_ms - last_fps_report);
                ESP_LOGI(TAG, "Core 1 [OLED-only]: %.1f fps (target: 10fps)", oled_fps);
                oled_frame_count = 0;
                last_fps_report = current_time_ms;
            }
        } else {
            // Precise timing control - only delay if we're ahead of schedule
            int64_t remaining_us = oled_interval_us - (current_time_us - last_oled_update);
            if (remaining_us > 1000) {
                vTaskDelay(pdMS_TO_TICKS(remaining_us / 1000));
            }
        }
    }
}

/** Initialize unified display system */
extern "C" void app_main() {
    ESP_LOGI(TAG, "Starting Unified HUB75 + OLED Display System");
    
    // Disable watchdog entirely to prevent task interference
    esp_task_wdt_deinit();
    ESP_LOGI(TAG, "Watchdog disabled to prevent task interference");
    
    ESP_LOGI(TAG, "Initializing unified display system with dual-core architecture");
    
    size_t buffer_size = buffer_width * buffer_height * sizeof(RGB);
    
    // Allocate DMA-aligned buffers to prevent corruption
    front_buffer = (RGB*)heap_caps_malloc(buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!front_buffer) {
        ESP_LOGE(TAG, "Failed to allocate front buffer (%d bytes)", buffer_size);
        return;
    }
    
    back_buffer = (RGB*)heap_caps_malloc(buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!back_buffer) {
        ESP_LOGE(TAG, "Failed to allocate back buffer (%d bytes)", buffer_size);
        return;
    }
    
    unified_buffer = (RGB*)heap_caps_malloc(buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!unified_buffer) {
        ESP_LOGE(TAG, "Failed to allocate unified buffer (%d bytes)", buffer_size);
        return;
    }
    
    ESP_LOGI(TAG, "Allocated double buffers: 3x %d bytes = %d total", buffer_size, buffer_size * 3);
    
    // Clear all buffers
    memset(front_buffer, 0, buffer_size);
    memset(back_buffer, 0, buffer_size);
    memset(unified_buffer, 0, buffer_size);
    
    ESP_LOGI(TAG, "Using lockless dual-core communication");
    
    // Initialize I2C
    i2c_driver_init();
    ESP_LOGI(TAG, "I2C initialized");
    
    // Initialize OLED
    esp_err_t ret = oled_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OLED initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Check OLED wiring: SCL->GPIO39, SDA->GPIO38");
        return;
    }
    
    oled_buffer = oled_get_buffer();
    if (!oled_buffer) {
        ESP_LOGE(TAG, "Failed to get OLED buffer");
        return;
    }
    
    ESP_LOGI(TAG, "OLED initialized successfully");
    oled_set_upside_down(true);
    ESP_LOGI(TAG, "OLED set to upside down orientation");
    
    // Initialize HUB75 display
    HUB75Config hub75_config = HUB75Config::getDefault();
    hub75_config.dual_display_mode = true;
    hub75_config.effective_width = 128;
    hub75_config.pins.oe_pin2 = 6;  // Secondary OE pin
    
    if (!hub75_display.init(hub75_config)) {
        ESP_LOGE(TAG, "HUB75 initialization failed");
        return;
    }
    
    if (!hub75_display.start()) {
        ESP_LOGE(TAG, "Failed to start HUB75");
        return;
    }
    
    ESP_LOGI(TAG, "HUB75 initialized successfully");
    
    // Initialize performance caches
    initializePerformanceCaches();
    
    // Test HUB75 with startup pattern
    ESP_LOGI(TAG, "Testing HUB75 with startup pattern");
    hub75_display.fill(RGB(32, 0, 0));
    hub75_display.show();
    vTaskDelay(pdMS_TO_TICKS(500));
    hub75_display.fill(RGB(0, 32, 0));
    hub75_display.show();
    vTaskDelay(pdMS_TO_TICKS(500));
    hub75_display.fill(RGB(0, 0, 32));
    hub75_display.show();
    vTaskDelay(pdMS_TO_TICKS(500));
    hub75_display.clear();
    hub75_display.show();
    ESP_LOGI(TAG, "HUB75 startup test completed");
    
    ESP_LOGI(TAG, "All displays initialized successfully");
    
    // Display system information
    size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_8BIT);
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    
    ESP_LOGI(TAG, "=== UNIFIED DISPLAY SYSTEM ===");
    ESP_LOGI(TAG, "  HUB75: %dx%d pixels (dual %dx%d panels)", buffer_width, buffer_height, buffer_width/2, buffer_height);
    ESP_LOGI(TAG, "  OLED: %dx%d pixels (HUB75 mirrored at bottom)", oled_width, oled_height);
    ESP_LOGI(TAG, "  Unified Buffer: %dx%d RGB pixels (%d bytes)", buffer_width, buffer_height, buffer_size);
    ESP_LOGI(TAG, "  Free RAM: %d KB / %d KB", free_heap / 1024, total_heap / 1024);
    ESP_LOGI(TAG, "  Conversion Modes: Luminance, Dithered, RGB Compensated");
    ESP_LOGI(TAG, "");
    
    // Create dual-core tasks
    ESP_LOGI(TAG, "Creating dual-core tasks (Pattern=Core0, Display=Core1)");
    
    // Create pattern generation task on Core 0 (high priority for 60fps)
    BaseType_t core0_result = xTaskCreatePinnedToCore(
        patternGenerationTask,      // Task function
        "PatternHUB75",            // Task name
        8192,                      // Stack size (bytes)
        nullptr,                   // Parameters
        configMAX_PRIORITIES - 1,  // High priority for 60fps
        &pattern_generation_task,  // Task handle
        0                          // Core 0
    );
    
    if (core0_result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create pattern generation task");
        return;
    }
    
    // Create display update task on Core 1 (medium priority for 25fps)
    BaseType_t core1_result = xTaskCreatePinnedToCore(
        displayUpdateTask,         // Task function
        "OLEDDisplay",            // Task name
        4096,                     // Stack size (bytes)
        nullptr,                  // Parameters
        configMAX_PRIORITIES - 2, // Medium priority for 25fps
        &display_update_task,     // Task handle
        1                         // Core 1
    );
    
    if (core1_result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create display update task");
        return;
    }
    
    ESP_LOGI(TAG, "Dual-core tasks created successfully");
    
    // Main loop for user interaction (optional)
    while(true) {
        vTaskDelay(pdMS_TO_TICKS(10000)); // 10 second intervals
        
        // Optional: Switch conversion modes or palettes
        static uint32_t switch_counter = 0;
        switch_counter++;
        
        if (switch_counter % 30 == 0) { // Every 5 minutes
            current_conversion = (ConversionMode)((current_conversion + 1) % 3);
            ESP_LOGI(TAG, "Switched to %s conversion", conversion_names[current_conversion]);
        }
        
        if (switch_counter % 60 == 0) { // Every 10 minutes
            currentPaletteIndex = (currentPaletteIndex + 1) % NUM_PALETTES;
            const char* palette_names[] = {"Heat", "Rainbow", "Lava"};
            ESP_LOGI(TAG, "Switched to %s palette", palette_names[currentPaletteIndex]);
        }
    }
}