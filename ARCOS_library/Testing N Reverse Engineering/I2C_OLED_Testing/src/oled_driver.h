#ifndef OLED_DRIVER_H
#define OLED_DRIVER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** OLED Configuration - 1.5" 128x128 White OLED with SH1107G-02 COG controller */
#define OLED_I2C_ADDRESS            0x3C  // OLED detected at address 0x3C
#define OLED_WIDTH                  128
#define OLED_HEIGHT                 128
#define OLED_PAGES                  (OLED_HEIGHT / 8)  // 16 pages for 128x128
#define OLED_BUFFER_SIZE            (OLED_WIDTH * OLED_PAGES)  // 2048 bytes

/** OLED Control Bytes */
#define OLED_CONTROL_BYTE_CMD_SINGLE    0x80
#define OLED_CONTROL_BYTE_CMD_STREAM    0x00
#define OLED_CONTROL_BYTE_DATA_STREAM   0x40

/** Rectangle area structure for selective updates */
typedef struct {
    int x;      // Left coordinate
    int y;      // Top coordinate  
    int width;  // Width in pixels
    int height; // Height in pixels
} oled_rect_t;

/** Update mode enumeration */
typedef enum {
    OLED_UPDATE_FULL,    // Update entire display
    OLED_UPDATE_SMART,   // Only update changed pages
    OLED_UPDATE_AREA,    // Update specific rectangular area
    OLED_UPDATE_PAGES    // Update specific page range
} oled_update_mode_t;

/**
 * @brief Initialize OLED display driver
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t oled_init(void);

/**
 * @brief Deinitialize OLED display driver
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t oled_deinit(void);

// ============== BUFFER MANAGEMENT ==============

/**
 * @brief Get direct access to the display buffer
 * 
 * @return uint8_t* Pointer to the display buffer (2048 bytes)
 */
uint8_t* oled_get_buffer(void);

/**
 * @brief Get buffer size
 * 
 * @return size_t Buffer size in bytes
 */
size_t oled_get_buffer_size(void);

/**
 * @brief Clear the entire display buffer
 */
void oled_clear_buffer(void);

/**
 * @brief Fill the entire display buffer with a pattern
 * 
 * @param pattern 8-bit pattern to fill with
 */
void oled_fill_buffer(uint8_t pattern);

/**
 * @brief Copy data directly to buffer at specific offset
 * 
 * @param offset Byte offset in buffer
 * @param data Source data
 * @param length Number of bytes to copy
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG if out of bounds
 */
esp_err_t oled_buffer_write(size_t offset, const uint8_t* data, size_t length);

/**
 * @brief Read data directly from buffer at specific offset
 * 
 * @param offset Byte offset in buffer
 * @param data Destination buffer
 * @param length Number of bytes to read
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG if out of bounds
 */
esp_err_t oled_buffer_read(size_t offset, uint8_t* data, size_t length);

// ============== PIXEL AND DRAWING FUNCTIONS ==============

/**
 * @brief Set a single pixel in the buffer
 * 
 * @param x X coordinate (0-127)
 * @param y Y coordinate (0-127)
 * @param on true to turn pixel on, false to turn off
 */
void oled_set_pixel(int x, int y, bool on);

/**
 * @brief Get pixel state from buffer
 * 
 * @param x X coordinate (0-127)
 * @param y Y coordinate (0-127)
 * @return true Pixel is on
 * @return false Pixel is off or coordinates invalid
 */
bool oled_get_pixel(int x, int y);

/**
 * @brief Draw a line between two points
 * 
 * @param x0 Start X coordinate
 * @param y0 Start Y coordinate
 * @param x1 End X coordinate
 * @param y1 End Y coordinate
 * @param on true to turn pixels on, false to turn off
 */
void oled_draw_line(int x0, int y0, int x1, int y1, bool on);

/**
 * @brief Draw a rectangle
 * 
 * @param x Left coordinate
 * @param y Top coordinate
 * @param width Rectangle width
 * @param height Rectangle height
 * @param filled true for filled rectangle, false for outline
 * @param on true to turn pixels on, false to turn off
 */
void oled_draw_rect(int x, int y, int width, int height, bool filled, bool on);

/**
 * @brief Draw a circle
 * 
 * @param center_x Center X coordinate
 * @param center_y Center Y coordinate
 * @param radius Circle radius
 * @param filled true for filled circle, false for outline
 * @param on true to turn pixels on, false to turn off
 */
void oled_draw_circle(int center_x, int center_y, int radius, bool filled, bool on);

// ============== TEXT RENDERING ==============

/**
 * @brief Draw a character at specified position
 * 
 * @param x Left coordinate
 * @param y Top coordinate
 * @param c Character to draw (ASCII 32-126)
 * @param on true to turn pixels on, false to turn off
 */
void oled_draw_char(int x, int y, char c, bool on);

/**
 * @brief Draw a string at specified position
 * 
 * @param x Left coordinate
 * @param y Top coordinate
 * @param str Null-terminated string to draw
 * @param on true to turn pixels on, false to turn off
 */
void oled_draw_string(int x, int y, const char* str, bool on);

/**
 * @brief Get text dimensions for planning layout
 * 
 * @param str String to measure
 * @param width Pointer to store width (can be NULL)
 * @param height Pointer to store height (can be NULL)
 */
void oled_get_text_size(const char* str, int* width, int* height);

// ============== DISPLAY UPDATE FUNCTIONS ==============

/**
 * @brief Update the entire display with buffer contents
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t oled_update_display(void);

/**
 * @brief Smart update - only send changed pages to display
 * 
 * Compares current buffer with previous state and only updates modified pages.
 * This significantly reduces I2C traffic for animations with partial changes.
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t oled_update_smart(void);

/**
 * @brief Update specific page range
 * 
 * @param start_page First page to update (0-15)
 * @param end_page Last page to update (0-15)  
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t oled_update_pages(int start_page, int end_page);

/**
 * @brief Update specific rectangular area
 * 
 * Only updates the pages that contain the specified rectangle.
 * Useful for updating small areas like text or simple graphics.
 * 
 * @param rect Rectangle area to update
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t oled_update_area(const oled_rect_t* rect);

/**
 * @brief Update display with specified mode
 * 
 * @param mode Update mode (full, smart, area, or pages)
 * @param params Parameters for area or page updates (can be NULL for full/smart)
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t oled_update_display_ex(oled_update_mode_t mode, const void* params);

// ============== ADVANCED BUFFER OPERATIONS ==============

/**
 * @brief Mark all pages as dirty (force full update on next smart update)
 */
void oled_mark_all_dirty(void);

/**
 * @brief Mark specific pages as dirty
 * 
 * @param start_page First page to mark dirty (0-15)
 * @param end_page Last page to mark dirty (0-15)
 */
void oled_mark_pages_dirty(int start_page, int end_page);

/**
 * @brief Mark area as dirty (affected pages will be updated on next smart update)
 * 
 * @param rect Rectangle area to mark as dirty
 */
void oled_mark_area_dirty(const oled_rect_t* rect);

/**
 * @brief Get the number of dirty pages
 * 
 * @return int Number of pages that need updating
 */
int oled_get_dirty_page_count(void);

/**
 * @brief Check if specific page is dirty
 * 
 * @param page Page number (0-15)
 * @return true Page needs updating
 * @return false Page is clean
 */
bool oled_is_page_dirty(int page);

// ============== UTILITY FUNCTIONS ==============

/**
 * @brief Convert pixel coordinates to buffer byte offset
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @return size_t Byte offset in buffer, or SIZE_MAX if coordinates invalid
 */
size_t oled_pixel_to_offset(int x, int y);

/**
 * @brief Convert Y coordinate to page number
 * 
 * @param y Y coordinate
 * @return int Page number (0-15), or -1 if invalid
 */
int oled_y_to_page(int y);

/**
 * @brief Get page boundaries for a rectangle
 * 
 * @param rect Rectangle to analyze
 * @param start_page Pointer to store first affected page
 * @param end_page Pointer to store last affected page
 */
void oled_rect_to_pages(const oled_rect_t* rect, int* start_page, int* end_page);

/**
 * @brief Create rectangle structure
 * 
 * @param x Left coordinate
 * @param y Top coordinate
 * @param width Rectangle width
 * @param height Rectangle height
 * @return oled_rect_t Rectangle structure
 */
static inline oled_rect_t oled_rect(int x, int y, int width, int height) {
    oled_rect_t rect = {x, y, width, height};
    return rect;
}

#ifdef __cplusplus
}
#endif

#endif // OLED_DRIVER_H