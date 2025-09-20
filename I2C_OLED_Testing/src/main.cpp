#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"

static const char* TAG = "OLED_I2C";

/** I2C Configuration - Optimized for high speed */
#define I2C_MASTER_SCL_IO           22    // GPIO22 for SCL (standard)
#define I2C_MASTER_SDA_IO           21    // GPIO21 for SDA (standard)
#define I2C_MASTER_NUM              I2C_NUM_0     // I2C port number
#define I2C_MASTER_FREQ_HZ          1000000 // 1MHz - Maximum speed for most OLEDs
#define I2C_MASTER_TX_BUF_DISABLE   0     // I2C master doesn't need buffer
#define I2C_MASTER_RX_BUF_DISABLE   0     // I2C master doesn't need buffer
#define I2C_MASTER_TIMEOUT_MS       100   // Shorter timeout for faster operations

/** OLED Configuration - 1.5" 128x128 White OLED with SH1107G-02 COG controller */
#define OLED_I2C_ADDRESS            0x3C  // Common OLED I2C address (try 0x3D if this doesn't work)
#define OLED_WIDTH                  128
#define OLED_HEIGHT                 128
#define OLED_PAGES                  (OLED_HEIGHT / 8)
#define OLED_CONTROLLER_SH1107G     1     // Use SH1107G controller

/** OLED Commands */
#define OLED_CONTROL_BYTE_CMD_SINGLE    0x80
#define OLED_CONTROL_BYTE_CMD_STREAM    0x00
#define OLED_CONTROL_BYTE_DATA_STREAM   0x40

/** SSD1306/SH1106 Commands */
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

/** Display buffer - 128x128 pixels = 16KB (128 * 128 / 8) */
static uint8_t oled_buffer[OLED_WIDTH * OLED_PAGES];
/** Previous frame buffer for change detection */
static uint8_t oled_buffer_prev[OLED_WIDTH * OLED_PAGES];
/** Page dirty flags - track which pages need updating */
static bool page_dirty[OLED_PAGES];

/** Simple 5x7 font for ASCII characters 32-126 */
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
};

/**
 * Initialize I2C master
 */
static esp_err_t i2c_master_init(void){
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
  conf.clk_flags = 0;
  
  esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
  if (err != ESP_OK) {
    return err;
  }
  
  return i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

/**
 * Scan I2C bus for devices
 */
static void i2c_scanner(void){
  ESP_LOGI("I2C_SCANNER", "Scanning I2C bus...");
  
  for(uint8_t address = 1; address < 127; address++){
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if(ret == ESP_OK){
      ESP_LOGI("I2C_SCANNER", "Device found at address 0x%02X", address);
    }
  }
  
  ESP_LOGI("I2C_SCANNER", "I2C scan complete");
}

/**
 * Write a byte to OLED via I2C
 */
static esp_err_t oled_write_byte(uint8_t data, uint8_t cmd){
  i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
  i2c_master_start(cmd_handle);
  i2c_master_write_byte(cmd_handle, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd_handle, cmd, true);
  i2c_master_write_byte(cmd_handle, data, true);
  i2c_master_stop(cmd_handle);
  
  esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd_handle, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd_handle);
  
  return ret;
}

/**
 * Write command to OLED
 */
static esp_err_t oled_write_command(uint8_t cmd){
  return oled_write_byte(cmd, OLED_CONTROL_BYTE_CMD_SINGLE);
}

/**
 * Write data to OLED
 */
static esp_err_t oled_write_data(uint8_t data){
  return oled_write_byte(data, OLED_CONTROL_BYTE_DATA_STREAM);
}

/**
 * Initialize OLED display - SH1107G-02 COG controller for 1.5" 128x128 White OLED
 */
static esp_err_t oled_init(void){
  ESP_LOGI(TAG, "Initializing 1.5\" 128x128 White OLED (SH1107G-02 COG)...");
  
  // Turn off display
  oled_write_command(OLED_CMD_SET_DISP | 0x00);
  vTaskDelay(pdMS_TO_TICKS(100)); // Longer delay for COG displays
  
  // Set display start line
  oled_write_command(OLED_CMD_SET_DISP_START_LINE | 0x00);
  
  // Set lower column address
  oled_write_command(0x00); // Lower column start address for SH1107G
  
  // Set higher column address  
  oled_write_command(0x10); // Higher column start address for SH1107G
  
  // Set memory addressing mode (page addressing for SH1107G)
  oled_write_command(0x20); // Set memory addressing mode
  oled_write_command(0x02); // Page addressing mode
  
  // Set contrast - specific for white OLED
  oled_write_command(OLED_CMD_SET_CONTRAST);
  oled_write_command(0x80); // Medium-high contrast for SH1107G white OLED
  
  // Set segment re-map (normal orientation)
  oled_write_command(OLED_CMD_SET_SEG_REMAP | 0x00);
  
  // Set multiplex ratio (128-1)
  oled_write_command(OLED_CMD_SET_MUX_RATIO);
  oled_write_command(0x7F); // 128-1 for 128x128
  
  // Set COM output scan direction (normal)
  oled_write_command(OLED_CMD_SET_COM_OUT_DIR | 0x00);
  
  // Set display offset
  oled_write_command(OLED_CMD_SET_DISP_OFFSET);
  oled_write_command(0x00);
  
  // Set display clock divide ratio/oscillator frequency
  oled_write_command(OLED_CMD_SET_DISP_CLK_DIV);
  oled_write_command(0x51); // Different timing for SH1107G
  
  // Set pre-charge period - critical for white OLED
  oled_write_command(OLED_CMD_SET_PRECHARGE);
  oled_write_command(0x22); // Balanced pre-charge for SH1107G
  
  // Set COM pins hardware configuration
  oled_write_command(OLED_CMD_SET_COM_PIN_CFG);
  oled_write_command(0x12); // Alternative COM pin configuration
  
  // Set VCOM deselect level - important for SH1107G
  oled_write_command(OLED_CMD_SET_VCOM_DESEL);
  oled_write_command(0x35); // Specific VCOM level for SH1107G
  
  // Set DC-DC enable (charge pump)
  oled_write_command(0xAD); // DC-DC control command for SH1107G
  oled_write_command(0x8A); // Enable internal DC-DC converter
  
  // Set normal display (not inverted)
  oled_write_command(OLED_CMD_SET_NORM_INV | 0x00);
  
  // Disable entire display on
  oled_write_command(OLED_CMD_SET_ENTIRE_ON | 0x00);
  
  /** Clear display page by page (SH1107G method) */
  for(int page = 0; page < 16; page++){
    oled_write_command(0xB0 + page); // Set page address (0xB0-0xBF)
    oled_write_command(0x00); // Set lower column address
    oled_write_command(0x10); // Set higher column address
    
    for(int col = 0; col < 128; col++){
      oled_write_data(0x00);
    }
  }
  
  vTaskDelay(pdMS_TO_TICKS(100)); // Wait before turning on
  
  // Turn on display
  oled_write_command(OLED_CMD_SET_DISP | 0x01);
  
  ESP_LOGI(TAG, "OLED display initialized successfully");
  return ESP_OK;
}

/**
 * Alternative initialization for SSD1306 controller (UNUSED)
 */
/*
static esp_err_t oled_init_ssd1306(void){
  ESP_LOGI(TAG, "Trying SSD1306 initialization...");

  // Turn off display
  oled_write_command(OLED_CMD_SET_DISP | 0x00);

  // Set memory addressing mode to horizontal
  oled_write_command(OLED_CMD_SET_MEM_ADDR);
  oled_write_command(0x00); // Horizontal addressing mode

  // Set column range
  oled_write_command(OLED_CMD_SET_COL_RANGE);
  oled_write_command(0x00); // Column start address
  oled_write_command(0x7F); // Column end address (127)

  // Set page range
  oled_write_command(OLED_CMD_SET_PAGE_RANGE);
  oled_write_command(0x00); // Page start address
  oled_write_command(0x0F); // Page end address (15)

  // Set display start line
  oled_write_command(OLED_CMD_SET_DISP_START_LINE | 0x00);

  // Set contrast
  oled_write_command(OLED_CMD_SET_CONTRAST);
  oled_write_command(0xFF); // Max contrast

  // Set segment re-map
  oled_write_command(OLED_CMD_SET_SEG_REMAP | 0x01);

  // Set COM output scan direction
  oled_write_command(OLED_CMD_SET_COM_OUT_DIR | 0x08);

  // Set multiplex ratio
  oled_write_command(OLED_CMD_SET_MUX_RATIO);
  oled_write_command(0x7F); // 128-1

  // Set display offset
  oled_write_command(OLED_CMD_SET_DISP_OFFSET);
  oled_write_command(0x00);

  // Set display clock divide ratio
  oled_write_command(OLED_CMD_SET_DISP_CLK_DIV);
  oled_write_command(0x80);

  // Set pre-charge period
  oled_write_command(OLED_CMD_SET_PRECHARGE);
  oled_write_command(0xF1);

  // Set COM pins hardware configuration
  oled_write_command(OLED_CMD_SET_COM_PIN_CFG);
  oled_write_command(0x12);

  // Set VCOM deselect level
  oled_write_command(OLED_CMD_SET_VCOM_DESEL);
  oled_write_command(0x40);

  // Enable charge pump
  oled_write_command(OLED_CMD_SET_CHARGE_PUMP);
  oled_write_command(0x14);

  // Set normal display
  oled_write_command(OLED_CMD_SET_NORM_INV | 0x00);

  // Disable entire display on
  oled_write_command(OLED_CMD_SET_ENTIRE_ON | 0x00);

  // Turn on display
  oled_write_command(OLED_CMD_SET_DISP | 0x01);

  ESP_LOGI(TAG, "SSD1306 initialization complete");
  return ESP_OK;
}
*/

/**
 * Update display with buffer contents - SSD1306 Horizontal Addressing Mode
 */
esp_err_t oled_update_display_ssd1306(void){
  // Set column range
  oled_write_command(OLED_CMD_SET_COL_RANGE);
  oled_write_command(0x00); // Column start address
  oled_write_command(0x7F); // Column end address (127)
  
  // Set page range  
  oled_write_command(OLED_CMD_SET_PAGE_RANGE);
  oled_write_command(0x00); // Page start address
  oled_write_command(0x0F); // Page end address (15)
  
  /** Send buffer data in chunks */
  const int chunk_size = 32;
  for(int i = 0; i < sizeof(oled_buffer); i += chunk_size){
    int remaining = sizeof(oled_buffer) - i;
    int current_chunk = (remaining < chunk_size) ? remaining : chunk_size;
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
    i2c_master_write(cmd, &oled_buffer[i], current_chunk, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if(ret != ESP_OK){
      ESP_LOGE(TAG, "Failed to send display data chunk: %s", esp_err_to_name(ret));
      return ret;
    }
  }
  
  return ESP_OK;
}

/**
 * Clear display buffer
 */
void oled_clear_buffer(void){
  memset(oled_buffer, 0x00, sizeof(oled_buffer));
}

/**
 * Fill display buffer
 */
void oled_fill_buffer(uint8_t pattern){
  memset(oled_buffer, pattern, sizeof(oled_buffer));
}

/**
 * Set a pixel in the buffer
 */
void oled_set_pixel(int x, int y, bool on){
  if(x >= 0 && x < OLED_WIDTH && y >= 0 && y < OLED_HEIGHT){
    int page = y / 8;
    int bit = y % 8;
    int index = page * OLED_WIDTH + x;
    
    if(on){
      oled_buffer[index] |= (1 << bit);
    }else{
      oled_buffer[index] &= ~(1 << bit);
    }
  }
}

/**
 * Draw a line between two points
 */
void oled_draw_line(int x0, int y0, int x1, int y1, bool on){
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;
  
  while(true){
    oled_set_pixel(x0, y0, on);
    
    if(x0 == x1 && y0 == y1) break;
    
    int e2 = 2 * err;
    if(e2 > -dy){
      err -= dy;
      x0 += sx;
    }
    if(e2 < dx){
      err += dx;
      y0 += sy;
    }
  }
}

/**
 * Draw a rectangle
 */
void oled_draw_rect(int x, int y, int width, int height, bool filled, bool on){
  if(filled){
    for(int i = 0; i < height; i++){
      oled_draw_line(x, y + i, x + width - 1, y + i, on);
    }
  }else{
    oled_draw_line(x, y, x + width - 1, y, on);                    // Top
    oled_draw_line(x, y + height - 1, x + width - 1, y + height - 1, on); // Bottom
    oled_draw_line(x, y, x, y + height - 1, on);                   // Left
    oled_draw_line(x + width - 1, y, x + width - 1, y + height - 1, on);  // Right
  }
}

/**
 * Draw a character at specified position
 */
void oled_draw_char(int x, int y, char c, bool on){
  if(c < 32 || c > 126) c = 32; // Replace invalid chars with space
  
  const uint8_t* char_data = font5x7[c - 32];
  
  for(int col = 0; col < 5; col++){
    uint8_t column = char_data[col];
    for(int row = 0; row < 7; row++){
      if(column & (1 << row)){
        oled_set_pixel(x + col, y + row, on);
      }
    }
  }
}

/**
 * Draw a string at specified position
 */
void oled_draw_string(int x, int y, const char* str, bool on){
  int pos_x = x;
  while(*str){
    oled_draw_char(pos_x, y, *str, on);
    pos_x += 6; // 5 pixels wide + 1 pixel spacing
    str++;
  }
}

/**
 * Fast update display - Optimized for high refresh rates
 */
esp_err_t oled_update_display(void){
  /** SH1107G optimized for maximum speed */
  for(int page = 0; page < 16; page++){
    /** Set page address (0xB0-0xBF) */
    esp_err_t ret = oled_write_command(0xB0 + page);
    if(ret != ESP_OK) return ret;
    
    /** Set column addresses */
    ret = oled_write_command(0x00); // Lower column
    if(ret != ESP_OK) return ret;
    ret = oled_write_command(0x10); // Higher column
    if(ret != ESP_OK) return ret;
    
    /** Send entire page in larger chunks for speed */
    int page_start = page * 128;
    const int chunk_size = 64; // Larger chunks for speed
    
    for(int i = 0; i < 128; i += chunk_size){
      int remaining = 128 - i;
      int current_chunk = (remaining < chunk_size) ? remaining : chunk_size;
      
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
      i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
      i2c_master_write(cmd, &oled_buffer[page_start + i], current_chunk, true);
      i2c_master_stop(cmd);
      
      ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
      i2c_cmd_link_delete(cmd);
      
      if(ret != ESP_OK){
        ESP_LOGE(TAG, "Failed to send display data chunk: %s", esp_err_to_name(ret));
        return ret;
      }
      /** No delay between chunks for maximum speed */
    }
  }
  
  return ESP_OK;
}

/**
 * Ultra-fast update - Send entire page at once (experimental)
 */
esp_err_t oled_update_display_fast(void){
  for(int page = 0; page < 16; page++){
    /** Set page and column addresses */
    esp_err_t ret = oled_write_command(0xB0 + page);
    if(ret != ESP_OK) return ret;
    ret = oled_write_command(0x00);
    if(ret != ESP_OK) return ret;
    ret = oled_write_command(0x10);
    if(ret != ESP_OK) return ret;
    
    /** Send entire page (128 bytes) in one transaction */
    int page_start = page * 128;
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
    i2c_master_write(cmd, &oled_buffer[page_start], 128, true);
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if(ret != ESP_OK){
      ESP_LOGE(TAG, "Failed to send page %d: %s", page, esp_err_to_name(ret));
      return ret;
    }
  }
  
  return ESP_OK;
}

/**
 * Mark all pages as dirty (force full update)
 */
void oled_mark_all_dirty(void){
  for(int i = 0; i < OLED_PAGES; i++){
    page_dirty[i] = true;
  }
}

/**
 * Detect changed pages by comparing with previous frame
 */
void oled_detect_changes(void){
  for(int page = 0; page < OLED_PAGES; page++){
    page_dirty[page] = false;
    int page_start = page * OLED_WIDTH;
    
    for(int i = 0; i < OLED_WIDTH; i++){
      if(oled_buffer[page_start + i] != oled_buffer_prev[page_start + i]){
        page_dirty[page] = true;
        break; // Page has changes, no need to check further
      }
    }
  }
  
  // Copy current buffer to previous for next comparison
  memcpy(oled_buffer_prev, oled_buffer, sizeof(oled_buffer));
}

/**
 * Smart update - Only send changed pages
 */
esp_err_t oled_update_smart(void){
  oled_detect_changes();
  
  int pages_sent = 0;
  for(int page = 0; page < OLED_PAGES; page++){
    if(!page_dirty[page]) continue; // Skip unchanged pages
    
    // Set page and column addresses
    esp_err_t ret = oled_write_command(0xB0 + page);
    if(ret != ESP_OK) return ret;
    ret = oled_write_command(0x00);
    if(ret != ESP_OK) return ret;
    ret = oled_write_command(0x10);
    if(ret != ESP_OK) return ret;
    
    // Send entire page (128 bytes) in one transaction
    int page_start = page * 128;
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
    i2c_master_write(cmd, &oled_buffer[page_start], 128, true);
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if(ret != ESP_OK){
      ESP_LOGE(TAG, "Failed to send page %d: %s", page, esp_err_to_name(ret));
      return ret;
    }
    pages_sent++;
  }
  
  // Log efficiency
  if(pages_sent > 0){
    ESP_LOGD(TAG, "Smart update: %d/%d pages sent (%.1f%% efficiency)", 
             pages_sent, OLED_PAGES, (OLED_PAGES - pages_sent) * 100.0f / OLED_PAGES);
  }
  
  return ESP_OK;
}

/**
 * Update only specific pages - For partial screen updates
 */
esp_err_t oled_update_pages(int start_page, int end_page){
  if(start_page < 0) start_page = 0;
  if(end_page > 15) end_page = 15;
  
  for(int page = start_page; page <= end_page; page++){
    /** Set page and column addresses */
    esp_err_t ret = oled_write_command(0xB0 + page);
    if(ret != ESP_OK) return ret;
    ret = oled_write_command(0x00);
    if(ret != ESP_OK) return ret;
    ret = oled_write_command(0x10);
    if(ret != ESP_OK) return ret;
    
    /** Send entire page (128 bytes) in one transaction */
    int page_start = page * 128;
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
    i2c_master_write(cmd, &oled_buffer[page_start], 128, true);
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if(ret != ESP_OK){
      ESP_LOGE(TAG, "Failed to send page %d: %s", page, esp_err_to_name(ret));
      return ret;
    }
  }
  
  return ESP_OK;
}

extern "C" void app_main(void){
  ESP_LOGI(TAG, "Starting OLED I2C Test");
  
  /** Initialize I2C */
  ESP_ERROR_CHECK(i2c_master_init());
  ESP_LOGI(TAG, "I2C initialized successfully");
  
  /** Scan for I2C devices */
  i2c_scanner();
  
  /** Initialize SH1107G display */
  ESP_LOGI(TAG, "Initializing SH1107G-02 COG display...");
  ESP_ERROR_CHECK(oled_init());
  
  /** Clear the buffer */
  oled_clear_buffer();
  
  /** Wait after initialization */
  vTaskDelay(pdMS_TO_TICKS(500));
  
  /** Test with simple patterns for SH1107G */
  ESP_LOGI(TAG, "Testing SH1107G with simple patterns...");
  
  /** Test 1: All black */
  memset(oled_buffer, 0x00, sizeof(oled_buffer));
  ESP_ERROR_CHECK(oled_update_display());
  ESP_LOGI(TAG, "Test 1: All black");
  vTaskDelay(pdMS_TO_TICKS(3000));
  
  /** Test 2: All white */
  memset(oled_buffer, 0xFF, sizeof(oled_buffer));
  ESP_ERROR_CHECK(oled_update_display());
  ESP_LOGI(TAG, "Test 2: All white");
  vTaskDelay(pdMS_TO_TICKS(3000));
  
  /** Test 3: Horizontal lines */
  memset(oled_buffer, 0xAA, sizeof(oled_buffer)); // 10101010 pattern
  ESP_ERROR_CHECK(oled_update_display());
  ESP_LOGI(TAG, "Test 3: Horizontal lines");
  vTaskDelay(pdMS_TO_TICKS(3000));
  
  /** Test 4: Vertical lines (corrected for page addressing) */
  for(int page = 0; page < 16; page++){
    for(int col = 0; col < 128; col++){
      int index = page * 128 + col;
      /** Create vertical stripes: alternate every 4 columns */
      oled_buffer[index] = ((col / 4) % 2) ? 0xFF : 0x00;
    }
  }
  ESP_ERROR_CHECK(oled_update_display());
  ESP_LOGI(TAG, "Test 4: Vertical stripes (corrected)");
    
  /** Smart update demo - patterns 7-11 only */
  int counter = 0;
  uint32_t pattern_start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
  
  ESP_LOGI(TAG, "Starting ultra-high-speed animation test...");
  
  uint32_t last_fps_time = 0;
  uint32_t fps_frame_count = 0;
  
  while(1){
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t elapsed_time = current_time - pattern_start_time;
        
    // Calculate pattern with variable durations
    uint32_t total_elapsed = 0;
    int pattern_type = 0;
    uint32_t pattern_durations[] = {3000, 3000, 3000, 3000, 15000}; // ASCII pattern gets 15 seconds
    
    for(int i = 0; i < 5; i++){
      if(elapsed_time < total_elapsed + pattern_durations[i]){
        pattern_type = i;
        break;
      }
      total_elapsed += pattern_durations[i];
      if(i == 4) { // Reset after all patterns
        pattern_start_time = current_time;
        elapsed_time = 0;
        total_elapsed = 0;
        pattern_type = 0;
        break;
      }
    }
    
    uint32_t pattern_time = elapsed_time - total_elapsed; // Time within current pattern
    bool pattern_just_started = (pattern_time < 50); // First 50ms of pattern
    
    /** Variable refresh rate for smart update patterns */
    if(pattern_type % 5 == 0 || pattern_type % 5 == 4){
      // Static text: No updates needed after first frame
      vTaskDelay(pdMS_TO_TICKS(100)); // Very slow since no I2C traffic
    } else {
      // Animated patterns: Maximum speed possible
      vTaskDelay(pdMS_TO_TICKS(1)); // Minimal delay for animations
    }
        
    /** Only show smart update patterns 7-11 (mapped to 0-4) */
    switch(pattern_type % 5){ // 5 smart update patterns only
      case 0: // Pattern 7: Static text
        {
          /** Static text - should only update once, then ZERO I2C traffic */
          if(pattern_just_started){
            oled_clear_buffer();
            oled_draw_string(10, 10, "STATIC TEXT", true);
            oled_draw_string(10, 20, "Zero I2C after", true);
            oled_draw_string(10, 30, "first frame!", true);
            oled_draw_rect(5, 5, 118, 50, false, true);
            ESP_LOGI(TAG, "Pattern: Static text (ZERO I2C after frame 1)");
          }
          break;
        }
      case 1: // Pattern 8: Blinking pixel
        {
          /** Single blinking pixel - minimal I2C usage */
          if(pattern_just_started){
            oled_clear_buffer();
            oled_draw_string(10, 10, "BLINKING PIXEL", true);
            oled_draw_string(10, 20, "1 byte/frame", true);
            ESP_LOGI(TAG, "Pattern: Blinking pixel (minimal I2C)");
          }
          // Blink pixel at 2 Hz
          bool pixel_on = (pattern_time / 500) % 2; // Toggle every 500ms
          oled_set_pixel(64, 64, pixel_on);
          break;
        }
      case 2: // Pattern 9: Scanning line
        {
          /** Horizontal line scan - demonstrates page-by-page updates */
          if(pattern_just_started){
            oled_clear_buffer();
            ESP_LOGI(TAG, "Pattern: Scanning line (1 page/frame)");
          }
          int scan_y = (pattern_time / 25) % 128; // Scan top to bottom in 3.2 seconds
          oled_draw_line(0, scan_y, 127, scan_y, true);
          if(scan_y > 0) oled_draw_line(0, scan_y - 1, 127, scan_y - 1, false); // Erase trail
          break;
        }
      case 3: // Pattern 10: Static grid + moving dot
        {
          /** Performance test - static image with tiny moving dot */
          if(pattern_just_started){
            // Draw complex static background once
            oled_clear_buffer();
            for(int i = 0; i < 128; i += 8){
              oled_draw_line(i, 0, i, 127, true);
              oled_draw_line(0, i, 127, i, true);
            }
            oled_draw_string(20, 30, "STATIC GRID", true);
            oled_draw_string(15, 50, "Moving dot test", true);
            ESP_LOGI(TAG, "Pattern: Static grid + moving dot");
          }
          // Move a single dot in a circle (minimal updates)
          float angle = pattern_time * 0.002f; // Complete circle in ~3 seconds
          int dot_x = 64 + 30 * cos(angle);
          int dot_y = 90 + 20 * sin(angle);
          
          // Clear old position (approximate)
          static int old_dot_x = 64, old_dot_y = 90;
          oled_set_pixel(old_dot_x, old_dot_y, false);
          oled_set_pixel(old_dot_x+1, old_dot_y, false);
          oled_set_pixel(old_dot_x, old_dot_y+1, false);
          oled_set_pixel(old_dot_x+1, old_dot_y+1, false);
          
          // Draw new position
          oled_set_pixel(dot_x, dot_y, true);
          oled_set_pixel(dot_x+1, dot_y, true);
          oled_set_pixel(dot_x, dot_y+1, true);
          oled_set_pixel(dot_x+1, dot_y+1, true);
          
          old_dot_x = dot_x;
          old_dot_y = dot_y;
          break;
        }
      case 4: // Pattern 11: ASCII character table
        {
          /** Display all printable ASCII characters in a scrolling table */
          // Clear buffer every frame to prevent overlapping
          oled_clear_buffer();
          
          if(pattern_just_started){
            ESP_LOGI(TAG, "Pattern: ASCII character table");
          }
          
          // Draw title
          oled_draw_string(25, 0, "ASCII TABLE", true);
          
          // Calculate which set of characters to show (rotate every 5 seconds)
          int char_set = (pattern_time / 5000) % 3; // 3 sets of characters in 15 seconds
          
          // Display different character ranges - isolate problematic characters
          int start_char = 32;
          char range_info[20];
          switch(char_set){
            case 0: 
              start_char = 32; 
              sprintf(range_info, "32-63: !\"#$%%&'()*+");
              break;  // Space to ? (32-63) - numbers and punctuation
            case 1: 
              start_char = 64; 
              sprintf(range_info, "64-90: @A-Z[\\]^_");
              break;  // @ to _ (64-90) - uppercase letters + symbols
            case 2: 
              start_char = 96; 
              sprintf(range_info, "96-126: `a-z{|}~");
              break;  // ` to ~ (96-126) - lowercase + symbols (CORRUPTED)
          }
          
          // Show range info
          oled_draw_string(0, 8, range_info, true);
          
          // Display limited range to fit properly and avoid overflow
          int max_chars = 32; // Show 32 characters max
          int end_char = start_char + max_chars - 1;
          if(end_char > 126) end_char = 126; // Don't go beyond printable ASCII
          
          // Display 8x4 grid of characters with proper spacing
          int char_index = 0;
          for(int row = 0; row < 4 && char_index < max_chars; row++){
            for(int col = 0; col < 8 && char_index < max_chars; col++){
              int char_code = start_char + char_index;
              if(char_code > end_char) break;
              
              // Better spacing - each character gets more room
              int x = 8 + col * 15;  // Increased spacing
              int y = 20 + row * 15; // Moved down to make room for range info
              
              // Show character and its code
              char display_str[8];
              if(char_code == 32){
                sprintf(display_str, "SP");  // Show "SP" for space
              } else if(char_code >= 32 && char_code <= 126){
                sprintf(display_str, "%c", (char)char_code);
              } else {
                sprintf(display_str, "?");  // Show ? for invalid chars
              }
              oled_draw_string(x, y, display_str, true);
              
              // Show ASCII code below character (smaller)
              char code_str[16];  // Large buffer to prevent any overflow
              snprintf(code_str, sizeof(code_str), "%d", char_code);
              oled_draw_string(x, y + 8, code_str, true);
              
              char_index++;
            }
          }
          
          // Show current character set info
          char set_info[20];
          sprintf(set_info, "Set %d/3", char_set + 1);
          oled_draw_string(85, 120, set_info, true);
          
          break;
        }
    }
        
    /** Always use smart updates for all patterns */
    if(pattern_just_started){
      // Force full update on first frame of new pattern
      oled_mark_all_dirty();
      ESP_ERROR_CHECK(oled_update_display_fast());
    } else {
      // Smart update - only changed pages
      ESP_ERROR_CHECK(oled_update_smart());
    }
    
    counter++;
    fps_frame_count++;
    
    /** Measure actual FPS every second */
    if(current_time - last_fps_time >= 1000){
      float actual_fps = fps_frame_count * 1000.0f / (current_time - last_fps_time);
      const char* pattern_names[] = {"Static Text", "Blink Pixel", "Scan Line", "Grid+Dot", "ASCII Table"};
      ESP_LOGI(TAG, "ACTUAL FPS: %.1f | Pattern: %s | Mode: Ultra-Fast Smart Update", 
               actual_fps, pattern_names[pattern_type % 5]);
      fps_frame_count = 0;
      last_fps_time = current_time;
    }
  }
}
