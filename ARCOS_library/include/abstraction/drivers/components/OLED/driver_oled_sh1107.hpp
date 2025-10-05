/*****************************************************************
 * File:      driver_oled_sh1107.hpp
 * Category:  abstraction/drivers/components/OLED
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    SH1107 OLED display driver with auto-initialization and
 *    graphics primitives following ARCOS conventions.
 *    Header-only implementation for optional inclusion.
 *    Supports 128x128 monochrome displays.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_OLED_SH1107_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_OLED_SH1107_HPP_

#include "abstraction/hal.hpp"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Include platform-specific I2C HAL implementation
// ESP32S3_I2C is defined in the platform hal_connector.hpp (included via hal.hpp)

namespace arcos{
namespace abstraction{

/** OLED rectangle structure for selective updates */
struct OLEDRect{
  int x;      // Left coordinate
  int y;      // Top coordinate  
  int width;  // Width in pixels
  int height; // Height in pixels
  
  /** Default constructor */
  OLEDRect() : x(0), y(0), width(0), height(0){}
  
  /** Constructor with parameters */
  OLEDRect(int x_, int y_, int w_, int h_) : x(x_), y(y_), width(w_), height(h_){}
};

/** Update mode enumeration */
enum struct OLEDUpdateMode{
  Full,    // Update entire display
  Smart,   // Only update changed pages
  Area,    // Update specific rectangular area
  Pages    // Update specific page range
};

/** OLED configuration structure (optional)
 * 
 * Allows customization of display settings during initialization.
 * If not provided, sensible defaults are used automatically.
 */
struct OLEDConfig{
  uint8_t contrast;           // Display contrast (0-255)
  bool flip_horizontal;       // Flip display horizontally
  bool flip_vertical;         // Flip display vertically
  uint8_t display_offset;     // Display offset (0-127)
  
  // Default configuration: medium contrast, no flipping
  OLEDConfig()
    : contrast(0x80), flip_horizontal(true), 
      flip_vertical(true), display_offset(0){}
};

/** SH1107 OLED display driver */
class DRIVER_OLED_SH1107{
private:
  static constexpr uint8_t DEFAULT_ADDRESS = 0x3C;
  static constexpr int DISPLAY_WIDTH = 128;
  static constexpr int DISPLAY_HEIGHT = 128;
  static constexpr int PAGES = DISPLAY_HEIGHT / 8;  // 16 pages
  static constexpr int BUFFER_SIZE = DISPLAY_WIDTH * PAGES;  // 2048 bytes
  
  // SH1107 Command definitions
  static constexpr uint8_t CMD_SET_CONTRAST = 0x81;
  static constexpr uint8_t CMD_SET_ENTIRE_ON = 0xA4;
  static constexpr uint8_t CMD_SET_NORM_INV = 0xA6;
  static constexpr uint8_t CMD_SET_DISP = 0xAE;
  static constexpr uint8_t CMD_SET_MEM_ADDR = 0x20;
  static constexpr uint8_t CMD_SET_COL_ADDR = 0x00;
  static constexpr uint8_t CMD_SET_PAGE_ADDR = 0xB0;
  static constexpr uint8_t CMD_SET_DISP_START_LINE = 0x40;
  static constexpr uint8_t CMD_SET_SEG_REMAP = 0xA1;
  static constexpr uint8_t CMD_SET_MUX_RATIO = 0xA8;
  static constexpr uint8_t CMD_SET_COM_OUT_DIR = 0xC8;
  static constexpr uint8_t CMD_SET_DISP_OFFSET = 0xD3;
  static constexpr uint8_t CMD_SET_COM_PIN_CFG = 0xDA;
  static constexpr uint8_t CMD_SET_DISP_CLK_DIV = 0xD5;
  static constexpr uint8_t CMD_SET_PRECHARGE = 0xD9;
  static constexpr uint8_t CMD_SET_VCOM_DESEL = 0xDB;
  
  // Control byte definitions
  static constexpr uint8_t CONTROL_CMD_SINGLE = 0x80;
  static constexpr uint8_t CONTROL_DATA_STREAM = 0x40;
  
  /** Display buffers */
  uint8_t buffer_[BUFFER_SIZE];
  uint8_t buffer_prev_[BUFFER_SIZE];
  bool page_dirty_[PAGES];
  
  uint8_t address_;
  uint8_t bus_id_;
  bool initialized_;
  
  /** 5x7 Font for ASCII characters 32-126 */
  static constexpr uint8_t font5x7_[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00}, // space, !
    {0x00, 0x07, 0x00, 0x07, 0x00}, {0x14, 0x7F, 0x14, 0x7F, 0x14}, // ", #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62}, // $, %
    {0x36, 0x49, 0x55, 0x22, 0x50}, {0x00, 0x05, 0x03, 0x00, 0x00}, // &, '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, {0x00, 0x41, 0x22, 0x1C, 0x00}, // (, )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, {0x08, 0x08, 0x3E, 0x08, 0x08}, // *, +
    {0x00, 0x50, 0x30, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, // comma, -
    {0x00, 0x60, 0x60, 0x00, 0x00}, {0x20, 0x10, 0x08, 0x04, 0x02}, // ., /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00}, // 0, 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31}, // 2, 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39}, // 4, 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03}, // 6, 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}, // 8, 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, {0x00, 0x56, 0x36, 0x00, 0x00}, // :, ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, {0x14, 0x14, 0x14, 0x14, 0x14}, // <, =
    {0x00, 0x41, 0x22, 0x14, 0x08}, {0x02, 0x01, 0x51, 0x09, 0x06}, // >, ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, {0x7E, 0x11, 0x11, 0x11, 0x7E}, // @, A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22}, // B, C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41}, // D, E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, {0x3E, 0x41, 0x49, 0x49, 0x7A}, // F, G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00}, // H, I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, // J, K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // L, M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E}, // N, O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, // P, Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, {0x46, 0x49, 0x49, 0x49, 0x31}, // R, S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F}, // T, U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x3F, 0x40, 0x38, 0x40, 0x3F}, // V, W
    {0x63, 0x14, 0x08, 0x14, 0x63}, {0x07, 0x08, 0x70, 0x08, 0x07}, // X, Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, {0x00, 0x7F, 0x41, 0x41, 0x00}, // Z, [
    {0x02, 0x04, 0x08, 0x10, 0x20}, {0x00, 0x41, 0x41, 0x7F, 0x00}, // backslash, ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, {0x40, 0x40, 0x40, 0x40, 0x40}, // ^, _
    {0x00, 0x01, 0x02, 0x04, 0x00}, {0x20, 0x54, 0x54, 0x54, 0x78}, // `, a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, {0x38, 0x44, 0x44, 0x44, 0x20}, // b, c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, {0x38, 0x54, 0x54, 0x54, 0x18}, // d, e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, {0x0C, 0x52, 0x52, 0x52, 0x3E}, // f, g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, {0x00, 0x44, 0x7D, 0x40, 0x00}, // h, i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, {0x7F, 0x10, 0x28, 0x44, 0x00}, // j, k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, {0x7C, 0x04, 0x18, 0x04, 0x78}, // l, m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, {0x38, 0x44, 0x44, 0x44, 0x38}, // n, o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, {0x08, 0x14, 0x14, 0x18, 0x7C}, // p, q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, {0x48, 0x54, 0x54, 0x54, 0x20}, // r, s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, {0x3C, 0x40, 0x40, 0x20, 0x7C}, // t, u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, {0x3C, 0x40, 0x30, 0x40, 0x3C}, // v, w
    {0x44, 0x28, 0x10, 0x28, 0x44}, {0x0C, 0x50, 0x50, 0x50, 0x3C}, // x, y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, {0x00, 0x08, 0x36, 0x41, 0x00}, // z, {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, {0x00, 0x41, 0x36, 0x08, 0x00}, // |, }
    {0x10, 0x08, 0x08, 0x10, 0x08}  // ~
  };
  
  /** Write a command to the OLED */
  bool writeCommand(uint8_t cmd);
  
  /** Write data stream to OLED */
  bool writeDataStream(const uint8_t* data, size_t length);
  
  /** Detect which pages have changed since last update */
  void detectChanges();

public:
  /** Constructor with default address and bus
   * @param address I2C address (default 0x3C)
   * @param bus_id I2C bus ID (default 0)
   */
  DRIVER_OLED_SH1107(uint8_t address = DEFAULT_ADDRESS, uint8_t bus_id = 0);

  /** Initialize the display with default configuration
   * Automatically configures display and clears screen
   * @return true if initialization successful
   */
  bool initialize();

  /** Initialize the display with custom configuration
   * Allows fine-grained control over display settings
   * @param config Custom configuration settings
   * @return true if initialization successful
   */
  bool initialize(const OLEDConfig& config);

  /** Deinitialize the display (turn off)
   * @return true if successful
   */
  bool deinitialize();

  /** Check if display is initialized
   * @return true if initialized
   */
  bool isInitialized() const{ return initialized_; }

  // ============== BUFFER MANAGEMENT ==============

  /** Get direct access to the display buffer
   * @return Pointer to the display buffer (2048 bytes)
   */
  uint8_t* getBuffer(){ return buffer_; }

  /** Get buffer size in bytes
   * @return Buffer size
   */
  size_t getBufferSize() const{ return BUFFER_SIZE; }

  /** Clear the entire display buffer */
  void clearBuffer();

  /** Fill the entire display buffer with a pattern
   * @param pattern 8-bit pattern to fill with
   */
  void fillBuffer(uint8_t pattern);

  /** Copy data directly to buffer at specific offset
   * @param offset Byte offset in buffer
   * @param data Source data
   * @param length Number of bytes to copy
   * @return true on success, false if out of bounds
   */
  bool bufferWrite(size_t offset, const uint8_t* data, size_t length);

  /** Read data directly from buffer at specific offset
   * @param offset Byte offset in buffer
   * @param data Destination buffer
   * @param length Number of bytes to read
   * @return true on success, false if out of bounds
   */
  bool bufferRead(size_t offset, uint8_t* data, size_t length);

  // ============== PIXEL AND DRAWING FUNCTIONS ==============

  /** Set a single pixel in the buffer
   * @param x X coordinate (0-127)
   * @param y Y coordinate (0-127)
   * @param on true to turn pixel on, false to turn off
   */
  void setPixel(int x, int y, bool on);

  /** Get pixel state from buffer
   * @param x X coordinate (0-127)
   * @param y Y coordinate (0-127)
   * @return true if pixel is on, false otherwise
   */
  bool getPixel(int x, int y);

  /** Draw a line between two points
   * @param x0 Start X coordinate
   * @param y0 Start Y coordinate
   * @param x1 End X coordinate
   * @param y1 End Y coordinate
   * @param on true to turn pixels on, false to turn off
   */
  void drawLine(int x0, int y0, int x1, int y1, bool on);

  /** Draw a rectangle
   * @param x Left coordinate
   * @param y Top coordinate
   * @param width Rectangle width
   * @param height Rectangle height
   * @param filled true for filled rectangle, false for outline
   * @param on true to turn pixels on, false to turn off
   */
  void drawRect(int x, int y, int width, int height, bool filled, bool on);

  /** Draw a circle
   * @param center_x Center X coordinate
   * @param center_y Center Y coordinate
   * @param radius Circle radius
   * @param filled true for filled circle, false for outline
   * @param on true to turn pixels on, false to turn off
   */
  void drawCircle(int center_x, int center_y, int radius, bool filled, bool on);

  // ============== TEXT RENDERING ==============

  /** Draw a character at specified position
   * @param x Left coordinate
   * @param y Top coordinate
   * @param c Character to draw (ASCII 32-126)
   * @param on true to turn pixels on, false to turn off
   */
  void drawChar(int x, int y, char c, bool on);

  /** Draw a string at specified position
   * @param x Left coordinate
   * @param y Top coordinate
   * @param str Null-terminated string to draw
   * @param on true to turn pixels on, false to turn off
   */
  void drawString(int x, int y, const char* str, bool on);

  /** Get text dimensions for planning layout
   * @param str String to measure
   * @param width Pointer to store width (can be nullptr)
   * @param height Pointer to store height (can be nullptr)
   */
  void getTextSize(const char* str, int* width, int* height);

  // ============== DISPLAY UPDATE FUNCTIONS ==============

  /** Update the entire display with buffer contents
   * @return true if successful
   */
  bool updateDisplay();

  /** Smart update - only send changed pages to display
   * Compares current buffer with previous state and only updates modified pages.
   * @return true if successful
   */
  bool updateSmart();

  /** Update specific page range
   * @param start_page First page to update (0-15)
   * @param end_page Last page to update (0-15)
   * @return true if successful
   */
  bool updatePages(int start_page, int end_page);

  /** Update specific rectangular area
   * Only updates the pages that contain the specified rectangle.
   * @param rect Rectangle area to update
   * @return true if successful
   */
  bool updateArea(const OLEDRect& rect);

  // ============== ADVANCED BUFFER OPERATIONS ==============

  /** Mark all pages as dirty (force full update on next smart update) */
  void markAllDirty();

  /** Mark specific pages as dirty
   * @param start_page First page to mark dirty (0-15)
   * @param end_page Last page to mark dirty (0-15)
   */
  void markPagesDirty(int start_page, int end_page);

  /** Mark area as dirty
   * @param rect Rectangle area to mark as dirty
   */
  void markAreaDirty(const OLEDRect& rect);

  /** Set display orientation (upside down rotation)
   * @param upside_down true to flip display 180 degrees, false for normal
   * @return true if successful
   */
  bool setUpsideDown(bool upside_down);

  /** Set horizontal flip (segment remap)
   * @param flip true to flip horizontally, false for normal
   * @return true if successful
   */
  bool setFlipHorizontal(bool flip);

  /** Set vertical flip (COM scan direction)
   * @param flip true to flip vertically, false for normal
   * @return true if successful
   */
  bool setFlipVertical(bool flip);

  /** Get the number of dirty pages
   * @return Number of pages that need updating
   */
  int getDirtyPageCount() const;

  /** Check if specific page is dirty
   * @param page Page number (0-15)
   * @return true if page needs updating
   */
  bool isPageDirty(int page) const;

  // ============== UTILITY FUNCTIONS ==============

  /** Convert Y coordinate to page number
   * @param y Y coordinate
   * @return Page number (0-15), or -1 if invalid
   */
  int yToPage(int y) const;

  /** Get page boundaries for a rectangle
   * @param rect Rectangle to analyze
   * @param start_page Pointer to store first affected page
   * @param end_page Pointer to store last affected page
   */
  void rectToPages(const OLEDRect& rect, int* start_page, int* end_page) const;
};

} // namespace abstraction
} // namespace arcos

// Include inline implementation
#include "driver_oled_sh1107_impl.hpp"

#endif // ARCOS_ABSTRACTION_DRIVERS_OLED_SH1107_HPP_
