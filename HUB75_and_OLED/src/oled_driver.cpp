#include "oled_driver.h"
#include "i2c_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <math.h>

static const char* TAG = "OLED_DRIVER";

/** OLED Commands for SH1107G */
#define OLED_CMD_SET_CONTRAST           0x81
#define OLED_CMD_SET_ENTIRE_ON          0xA4
#define OLED_CMD_SET_NORM_INV           0xA6
#define OLED_CMD_SET_DISP               0xAE
#define OLED_CMD_SET_MEM_ADDR           0x20
#define OLED_CMD_SET_COL_RANGE          0x21
#define OLED_CMD_SET_PAGE_RANGE         0x22
#define OLED_CMD_SET_COL_ADDR           0x00
#define OLED_CMD_SET_PAGE_ADDR          0xB0
#define OLED_CMD_SET_DISP_START_LINE    0x40
#define OLED_CMD_SET_SEG_REMAP          0xA1
#define OLED_CMD_SET_MUX_RATIO          0xA8
#define OLED_CMD_SET_COM_OUT_DIR        0xC8
#define OLED_CMD_SET_DISP_OFFSET        0xD3
#define OLED_CMD_SET_COM_PIN_CFG        0xDA
#define OLED_CMD_SET_DISP_CLK_DIV       0xD5
#define OLED_CMD_SET_PRECHARGE          0xD9
#define OLED_CMD_SET_VCOM_DESEL         0xDB
#define OLED_CMD_SET_CHARGE_PUMP        0x8D

/** Display buffers */
static uint8_t oled_buffer[OLED_BUFFER_SIZE];
static uint8_t oled_buffer_prev[OLED_BUFFER_SIZE];
static bool page_dirty[OLED_PAGES];

/** 5x7 Font for ASCII characters 32-126 */
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 (space)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // 42 *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 77 M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 87 W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 Z
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 backslash
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 f
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 103 g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // 123 {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // 125 }
    {0x10, 0x08, 0x08, 0x10, 0x08}, // 126 ~
};

// ============== PRIVATE HELPER FUNCTIONS ==============

static esp_err_t oled_write_command(uint8_t cmd) {
    return i2c_write_command(OLED_I2C_ADDRESS, OLED_CONTROL_BYTE_CMD_SINGLE, cmd);
}

static void oled_detect_changes(void) {
    for (int page = 0; page < OLED_PAGES; page++) {
        page_dirty[page] = false;
        int page_start = page * OLED_WIDTH;
        
        for (int i = 0; i < OLED_WIDTH; i++) {
            if (oled_buffer[page_start + i] != oled_buffer_prev[page_start + i]) {
                page_dirty[page] = true;
                break;
            }
        }
    }
    
    // Copy current buffer to previous for next comparison
    memcpy(oled_buffer_prev, oled_buffer, sizeof(oled_buffer));
}

// ============== PUBLIC API IMPLEMENTATION ==============

esp_err_t oled_init(void) {
    ESP_LOGI(TAG, "Initializing 1.5\" 128x128 White OLED (SH1107G-02 COG)...");
    
    // Clear buffers
    memset(oled_buffer, 0, sizeof(oled_buffer));
    memset(oled_buffer_prev, 0, sizeof(oled_buffer_prev));
    memset(page_dirty, true, sizeof(page_dirty));
    
    // Turn off display
    ESP_ERROR_CHECK(oled_write_command(OLED_CMD_SET_DISP | 0x00));
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Set display start line
    ESP_ERROR_CHECK(oled_write_command(OLED_CMD_SET_DISP_START_LINE | 0x00));
    
    // Set lower and higher column address
    ESP_ERROR_CHECK(oled_write_command(0x00)); // Lower column start address
    ESP_ERROR_CHECK(oled_write_command(0x10)); // Higher column start address
    
    // Set memory addressing mode (page addressing for SH1107G)
    ESP_ERROR_CHECK(oled_write_command(0x20));
    ESP_ERROR_CHECK(oled_write_command(0x02)); // Page addressing mode
    
    // Set contrast for white OLED
    ESP_ERROR_CHECK(oled_write_command(OLED_CMD_SET_CONTRAST));
    ESP_ERROR_CHECK(oled_write_command(0x80)); // Medium-high contrast
    
    // Set segment re-map (flip horizontally)
    ESP_ERROR_CHECK(oled_write_command(OLED_CMD_SET_SEG_REMAP | 0x01));
    
    // Set multiplex ratio (128-1)
    ESP_ERROR_CHECK(oled_write_command(OLED_CMD_SET_MUX_RATIO));
    ESP_ERROR_CHECK(oled_write_command(0x7F)); // 128-1 for 128x128
    
    // Set COM output scan direction (flip vertically)
    ESP_ERROR_CHECK(oled_write_command(OLED_CMD_SET_COM_OUT_DIR | 0x08));
    
    // Set display offset
    ESP_ERROR_CHECK(oled_write_command(OLED_CMD_SET_DISP_OFFSET));
    ESP_ERROR_CHECK(oled_write_command(0x00));
    
    // Set display clock divide ratio/oscillator frequency
    ESP_ERROR_CHECK(oled_write_command(OLED_CMD_SET_DISP_CLK_DIV));
    ESP_ERROR_CHECK(oled_write_command(0x51));
    
    // Set pre-charge period
    ESP_ERROR_CHECK(oled_write_command(OLED_CMD_SET_PRECHARGE));
    ESP_ERROR_CHECK(oled_write_command(0x22));
    
    // Set COM pins hardware configuration
    ESP_ERROR_CHECK(oled_write_command(OLED_CMD_SET_COM_PIN_CFG));
    ESP_ERROR_CHECK(oled_write_command(0x12));
    
    // Set VCOM deselect level
    ESP_ERROR_CHECK(oled_write_command(OLED_CMD_SET_VCOM_DESEL));
    ESP_ERROR_CHECK(oled_write_command(0x35));
    
    // Set DC-DC enable (charge pump for SH1107G)
    ESP_ERROR_CHECK(oled_write_command(0xAD));
    ESP_ERROR_CHECK(oled_write_command(0x8A));
    
    // Set normal display (not inverted)
    ESP_ERROR_CHECK(oled_write_command(OLED_CMD_SET_NORM_INV | 0x00));
    
    // Disable entire display on
    ESP_ERROR_CHECK(oled_write_command(OLED_CMD_SET_ENTIRE_ON | 0x00));
    
    // Clear display page by page
    for (int page = 0; page < 16; page++) {
        ESP_ERROR_CHECK(oled_write_command(0xB0 + page));
        ESP_ERROR_CHECK(oled_write_command(0x00));
        ESP_ERROR_CHECK(oled_write_command(0x10));
        
        uint8_t zeros[128] = {0};
        ESP_ERROR_CHECK(i2c_write_data_stream(OLED_I2C_ADDRESS, OLED_CONTROL_BYTE_DATA_STREAM, zeros, 128));
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Turn on display
    ESP_ERROR_CHECK(oled_write_command(OLED_CMD_SET_DISP | 0x01));
    
    ESP_LOGI(TAG, "OLED display initialized successfully");
    return ESP_OK;
}

esp_err_t oled_deinit(void) {
    // Turn off display
    esp_err_t err = oled_write_command(OLED_CMD_SET_DISP | 0x00);
    ESP_LOGI(TAG, "OLED display deinitialized");
    return err;
}

uint8_t* oled_get_buffer(void) {
    return oled_buffer;
}

size_t oled_get_buffer_size(void) {
    return OLED_BUFFER_SIZE;
}

void oled_clear_buffer(void) {
    memset(oled_buffer, 0x00, sizeof(oled_buffer));
}

void oled_fill_buffer(uint8_t pattern) {
    memset(oled_buffer, pattern, sizeof(oled_buffer));
}

esp_err_t oled_buffer_write(size_t offset, const uint8_t* data, size_t length) {
    if (data == NULL || offset + length > OLED_BUFFER_SIZE) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&oled_buffer[offset], data, length);
    return ESP_OK;
}

esp_err_t oled_buffer_read(size_t offset, uint8_t* data, size_t length) {
    if (data == NULL || offset + length > OLED_BUFFER_SIZE) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(data, &oled_buffer[offset], length);
    return ESP_OK;
}

void oled_set_pixel(int x, int y, bool on) {
    if (x >= 0 && x < OLED_WIDTH && y >= 0 && y < OLED_HEIGHT) {
        int page = y / 8;
        int bit = y % 8;
        int index = page * OLED_WIDTH + x;
        
        if (on) {
            oled_buffer[index] |= (1 << bit);
        } else {
            oled_buffer[index] &= ~(1 << bit);
        }
    }
}

bool oled_get_pixel(int x, int y) {
    if (x >= 0 && x < OLED_WIDTH && y >= 0 && y < OLED_HEIGHT) {
        int page = y / 8;
        int bit = y % 8;
        int index = page * OLED_WIDTH + x;
        return (oled_buffer[index] & (1 << bit)) != 0;
    }
    return false;
}

void oled_draw_line(int x0, int y0, int x1, int y1, bool on) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        oled_set_pixel(x0, y0, on);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void oled_draw_rect(int x, int y, int width, int height, bool filled, bool on) {
    if (filled) {
        for (int i = 0; i < height; i++) {
            oled_draw_line(x, y + i, x + width - 1, y + i, on);
        }
    } else {
        oled_draw_line(x, y, x + width - 1, y, on);                    // Top
        oled_draw_line(x, y + height - 1, x + width - 1, y + height - 1, on); // Bottom
        oled_draw_line(x, y, x, y + height - 1, on);                   // Left
        oled_draw_line(x + width - 1, y, x + width - 1, y + height - 1, on);  // Right
    }
}

void oled_draw_circle(int center_x, int center_y, int radius, bool filled, bool on) {
    if (filled) {
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x*x + y*y <= radius*radius) {
                    oled_set_pixel(center_x + x, center_y + y, on);
                }
            }
        }
    } else {
        // Bresenham's circle algorithm
        int x = 0;
        int y = radius;
        int d = 3 - 2 * radius;
        
        while (y >= x) {
            // Draw 8 symmetric points
            oled_set_pixel(center_x + x, center_y + y, on);
            oled_set_pixel(center_x - x, center_y + y, on);
            oled_set_pixel(center_x + x, center_y - y, on);
            oled_set_pixel(center_x - x, center_y - y, on);
            oled_set_pixel(center_x + y, center_y + x, on);
            oled_set_pixel(center_x - y, center_y + x, on);
            oled_set_pixel(center_x + y, center_y - x, on);
            oled_set_pixel(center_x - y, center_y - x, on);
            
            x++;
            if (d > 0) {
                y--;
                d = d + 4 * (x - y) + 10;
            } else {
                d = d + 4 * x + 6;
            }
        }
    }
}

void oled_draw_char(int x, int y, char c, bool on) {
    if (c < 32 || c > 126) c = 32; // Replace invalid chars with space
    
    const uint8_t* char_data = font5x7[c - 32];
    
    for (int col = 0; col < 5; col++) {
        uint8_t column = char_data[col];
        for (int row = 0; row < 7; row++) {
            if (column & (1 << row)) {
                oled_set_pixel(x + col, y + row, on);
            }
        }
    }
}

void oled_draw_string(int x, int y, const char* str, bool on) {
    int pos_x = x;
    while (*str) {
        oled_draw_char(pos_x, y, *str, on);
        pos_x += 6; // 5 pixels wide + 1 pixel spacing
        str++;
    }
}

void oled_get_text_size(const char* str, int* width, int* height) {
    if (width) {
        *width = strlen(str) * 6; // 5 pixels + 1 spacing per char
        if (*width > 0) *width -= 1; // Remove last spacing
    }
    if (height) {
        *height = 7; // Font is 7 pixels high
    }
}

esp_err_t oled_update_display(void) {
    for (int page = 0; page < 16; page++) {
        // Set page and column addresses
        ESP_ERROR_CHECK(oled_write_command(0xB0 + page));
        ESP_ERROR_CHECK(oled_write_command(0x00));
        ESP_ERROR_CHECK(oled_write_command(0x10));
        
        // Send entire page (128 bytes) in one transaction
        int page_start = page * 128;
        ESP_ERROR_CHECK(i2c_write_data_stream(OLED_I2C_ADDRESS, OLED_CONTROL_BYTE_DATA_STREAM, 
                                            &oled_buffer[page_start], 128));
    }
    
    return ESP_OK;
}

esp_err_t oled_update_smart(void) {
    oled_detect_changes();
    
    int pages_sent = 0;
    for (int page = 0; page < OLED_PAGES; page++) {
        if (!page_dirty[page]) continue; // Skip unchanged pages
        
        // Set page and column addresses
        ESP_ERROR_CHECK(oled_write_command(0xB0 + page));
        ESP_ERROR_CHECK(oled_write_command(0x00));
        ESP_ERROR_CHECK(oled_write_command(0x10));
        
        // Send entire page (128 bytes) in one transaction
        int page_start = page * 128;
        ESP_ERROR_CHECK(i2c_write_data_stream(OLED_I2C_ADDRESS, OLED_CONTROL_BYTE_DATA_STREAM,
                                            &oled_buffer[page_start], 128));
        pages_sent++;
    }
    
    // Log efficiency
    if (pages_sent > 0) {
        ESP_LOGD(TAG, "Smart update: %d/%d pages sent (%.1f%% efficiency)", 
                 pages_sent, OLED_PAGES, (OLED_PAGES - pages_sent) * 100.0f / OLED_PAGES);
    }
    
    return ESP_OK;
}

esp_err_t oled_update_pages(int start_page, int end_page) {
    if (start_page < 0) start_page = 0;
    if (end_page > 15) end_page = 15;
    
    for (int page = start_page; page <= end_page; page++) {
        // Set page and column addresses
        ESP_ERROR_CHECK(oled_write_command(0xB0 + page));
        ESP_ERROR_CHECK(oled_write_command(0x00));
        ESP_ERROR_CHECK(oled_write_command(0x10));
        
        // Send entire page (128 bytes) in one transaction
        int page_start = page * 128;
        ESP_ERROR_CHECK(i2c_write_data_stream(OLED_I2C_ADDRESS, OLED_CONTROL_BYTE_DATA_STREAM,
                                            &oled_buffer[page_start], 128));
    }
    
    return ESP_OK;
}

esp_err_t oled_update_area(const oled_rect_t* rect) {
    if (rect == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int start_page, end_page;
    oled_rect_to_pages(rect, &start_page, &end_page);
    
    return oled_update_pages(start_page, end_page);
}

esp_err_t oled_update_display_ex(oled_update_mode_t mode, const void* params) {
    switch (mode) {
        case OLED_UPDATE_FULL:
            return oled_update_display();
        
        case OLED_UPDATE_SMART:
            return oled_update_smart();
        
        case OLED_UPDATE_AREA:
            if (params == NULL) return ESP_ERR_INVALID_ARG;
            return oled_update_area((const oled_rect_t*)params);
        
        case OLED_UPDATE_PAGES:
            if (params == NULL) return ESP_ERR_INVALID_ARG;
            {
                const int* page_range = (const int*)params;
                return oled_update_pages(page_range[0], page_range[1]);
            }
        
        default:
            return ESP_ERR_INVALID_ARG;
    }
}

void oled_mark_all_dirty(void) {
    for (int i = 0; i < OLED_PAGES; i++) {
        page_dirty[i] = true;
    }
}

void oled_mark_pages_dirty(int start_page, int end_page) {
    if (start_page < 0) start_page = 0;
    if (end_page >= OLED_PAGES) end_page = OLED_PAGES - 1;
    
    for (int i = start_page; i <= end_page; i++) {
        page_dirty[i] = true;
    }
}

void oled_mark_area_dirty(const oled_rect_t* rect) {
    if (rect == NULL) return;
    
    int start_page, end_page;
    oled_rect_to_pages(rect, &start_page, &end_page);
    oled_mark_pages_dirty(start_page, end_page);
}

int oled_get_dirty_page_count(void) {
    int count = 0;
    for (int i = 0; i < OLED_PAGES; i++) {
        if (page_dirty[i]) count++;
    }
    return count;
}

bool oled_is_page_dirty(int page) {
    if (page < 0 || page >= OLED_PAGES) return false;
    return page_dirty[page];
}

size_t oled_pixel_to_offset(int x, int y) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) {
        return SIZE_MAX; // Invalid coordinates
    }
    
    int page = y / 8;
    return page * OLED_WIDTH + x;
}

int oled_y_to_page(int y) {
    if (y < 0 || y >= OLED_HEIGHT) return -1;
    return y / 8;
}

void oled_rect_to_pages(const oled_rect_t* rect, int* start_page, int* end_page) {
    if (rect == NULL || start_page == NULL || end_page == NULL) return;
    
    *start_page = oled_y_to_page(rect->y);
    *end_page = oled_y_to_page(rect->y + rect->height - 1);
    
    // Clamp to valid range
    if (*start_page < 0) *start_page = 0;
    if (*end_page >= OLED_PAGES) *end_page = OLED_PAGES - 1;
    if (*start_page > *end_page) *start_page = *end_page;
}

esp_err_t oled_set_upside_down(bool upside_down) {
    ESP_LOGI(TAG, "Setting OLED orientation: %s", upside_down ? "upside down" : "normal");
    
    uint8_t seg_remap_cmd = upside_down ? 0xA0 : 0xA1;  // Segment remap
    uint8_t com_scan_cmd = upside_down ? 0xC0 : 0xC8;   // COM output scan direction
    
    // Send segment remap command
    esp_err_t ret = i2c_write_command(OLED_I2C_ADDRESS, 
                                     OLED_CONTROL_BYTE_CMD_SINGLE, seg_remap_cmd);
    if (ret != ESP_OK) return ret;
    
    // Send COM scan direction command
    ret = i2c_write_command(OLED_I2C_ADDRESS, 
                           OLED_CONTROL_BYTE_CMD_SINGLE, com_scan_cmd);
    if (ret != ESP_OK) return ret;
    
    ESP_LOGI(TAG, "OLED orientation set to %s", upside_down ? "upside down" : "normal");
    return ESP_OK;
}