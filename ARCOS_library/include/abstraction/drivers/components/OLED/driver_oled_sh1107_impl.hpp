/*****************************************************************
 * File:      driver_oled_sh1107_impl.hpp
 * Category:  abstraction/drivers/components/OLED
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    SH1107 OLED display driver inline implementation.
 *    Automatically included by driver_oled_sh1107.hpp.
 *    Uses only cross-platform ARCOS abstractions.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_OLED_SH1107_IMPL_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_OLED_SH1107_IMPL_HPP_

#include <cmath>

namespace arcos{
namespace abstraction{

// ============== PRIVATE HELPER FUNCTIONS ==============

inline bool DRIVER_OLED_SH1107::writeCommand(uint8_t cmd){
  // OLED protocol: Send control byte (0x80) followed by command byte
  uint8_t buffer[2] = {CONTROL_CMD_SINGLE, cmd};
  return ESP32S3_I2C::WriteBytes(bus_id_, address_, buffer, 2) == HalResult::Success;
}

inline bool DRIVER_OLED_SH1107::writeDataStream(const uint8_t* data, size_t length){
  // OLED protocol: Send control byte (0x40) followed by data stream
  // We need to prepend the control byte to the data
  // Allocate a temporary buffer for control byte + data
  uint8_t* buffer = new uint8_t[length + 1];
  buffer[0] = CONTROL_DATA_STREAM;
  memcpy(&buffer[1], data, length);
  
  HalResult result = ESP32S3_I2C::WriteBytes(bus_id_, address_, buffer, length + 1);
  delete[] buffer;
  
  return result == HalResult::Success;
}

inline void DRIVER_OLED_SH1107::detectChanges(){
  for(int page = 0; page < PAGES; page++){
    page_dirty_[page] = false;
    int page_start = page * DISPLAY_WIDTH;
    
    for(int i = 0; i < DISPLAY_WIDTH; i++){
      if(buffer_[page_start + i] != buffer_prev_[page_start + i]){
        page_dirty_[page] = true;
        break;
      }
    }
  }
  
  // Copy current buffer to previous for next comparison
  memcpy(buffer_prev_, buffer_, sizeof(buffer_));
}

// ============== CONSTRUCTOR ==============

inline DRIVER_OLED_SH1107::DRIVER_OLED_SH1107(uint8_t address, uint8_t bus_id)
  : address_(address),
    bus_id_(bus_id),
    initialized_(false){
  memset(buffer_, 0, sizeof(buffer_));
  memset(buffer_prev_, 0, sizeof(buffer_prev_));
  memset(page_dirty_, true, sizeof(page_dirty_));
}

// ============== INITIALIZATION ==============

inline bool DRIVER_OLED_SH1107::initialize(){
  OLEDConfig default_config;
  return initialize(default_config);
}

inline bool DRIVER_OLED_SH1107::initialize(const OLEDConfig& config){
  // Turn off display
  if(!writeCommand(CMD_SET_DISP | 0x00)){
    return false;
  }
  HAL_TIMER_DEFAULT::Delay(100);
  
  // Set display start line
  if(!writeCommand(CMD_SET_DISP_START_LINE | 0x00)) return false;
  
  // Set lower and higher column address
  if(!writeCommand(0x00)) return false;  // Lower column start address
  if(!writeCommand(0x10)) return false;  // Higher column start address
  
  // Set memory addressing mode (page addressing for SH1107)
  if(!writeCommand(0x20)) return false;
  if(!writeCommand(0x02)) return false;  // Page addressing mode
  
  // Set contrast
  if(!writeCommand(CMD_SET_CONTRAST)) return false;
  if(!writeCommand(config.contrast)) return false;
  
  // Set segment re-map
  uint8_t seg_remap = config.flip_horizontal ? (CMD_SET_SEG_REMAP | 0x01) : (CMD_SET_SEG_REMAP & 0xFE);
  if(!writeCommand(seg_remap)) return false;
  
  // Set multiplex ratio (128-1)
  if(!writeCommand(CMD_SET_MUX_RATIO)) return false;
  if(!writeCommand(0x7F)) return false;  // 128-1 for 128x128
  
  // Set COM output scan direction
  uint8_t com_dir = config.flip_vertical ? (CMD_SET_COM_OUT_DIR | 0x08) : (CMD_SET_COM_OUT_DIR & 0xF7);
  if(!writeCommand(com_dir)) return false;
  
  // Set display offset
  if(!writeCommand(CMD_SET_DISP_OFFSET)) return false;
  if(!writeCommand(config.display_offset)) return false;
  
  // Set display clock divide ratio/oscillator frequency
  if(!writeCommand(CMD_SET_DISP_CLK_DIV)) return false;
  if(!writeCommand(0x51)) return false;
  
  // Set pre-charge period
  if(!writeCommand(CMD_SET_PRECHARGE)) return false;
  if(!writeCommand(0x22)) return false;
  
  // Set COM pins hardware configuration
  if(!writeCommand(CMD_SET_COM_PIN_CFG)) return false;
  if(!writeCommand(0x12)) return false;
  
  // Set VCOM deselect level
  if(!writeCommand(CMD_SET_VCOM_DESEL)) return false;
  if(!writeCommand(0x35)) return false;
  
  // Set DC-DC enable (charge pump for SH1107)
  if(!writeCommand(0xAD)) return false;
  if(!writeCommand(0x8A)) return false;
  
  // Set normal display (not inverted)
  if(!writeCommand(CMD_SET_NORM_INV | 0x00)) return false;
  
  // Disable entire display on
  if(!writeCommand(CMD_SET_ENTIRE_ON | 0x00)) return false;
  
  // Clear display page by page
  for(int page = 0; page < 16; page++){
    if(!writeCommand(0xB0 + page)) return false;
    if(!writeCommand(0x00)) return false;
    if(!writeCommand(0x10)) return false;
    
    uint8_t zeros[128] = {0};
    if(!writeDataStream(zeros, 128)) return false;
  }
  
  HAL_TIMER_DEFAULT::Delay(100);
  
  // Turn on display
  if(!writeCommand(CMD_SET_DISP | 0x01)) return false;
  
  initialized_ = true;
  return true;
}

inline bool DRIVER_OLED_SH1107::deinitialize(){
  initialized_ = false;
  return writeCommand(CMD_SET_DISP | 0x00);
}

// ============== BUFFER MANAGEMENT ==============

inline void DRIVER_OLED_SH1107::clearBuffer(){
  memset(buffer_, 0x00, sizeof(buffer_));
}

inline void DRIVER_OLED_SH1107::fillBuffer(uint8_t pattern){
  memset(buffer_, pattern, sizeof(buffer_));
}

inline bool DRIVER_OLED_SH1107::bufferWrite(size_t offset, const uint8_t* data, size_t length){
  if(data == nullptr || offset + length > BUFFER_SIZE){
    return false;
  }
  
  memcpy(&buffer_[offset], data, length);
  return true;
}

inline bool DRIVER_OLED_SH1107::bufferRead(size_t offset, uint8_t* data, size_t length){
  if(data == nullptr || offset + length > BUFFER_SIZE){
    return false;
  }
  
  memcpy(data, &buffer_[offset], length);
  return true;
}

// ============== PIXEL AND DRAWING FUNCTIONS ==============

inline void DRIVER_OLED_SH1107::setPixel(int x, int y, bool on){
  if(x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT){
    int page = y / 8;
    int bit = y % 8;
    int index = page * DISPLAY_WIDTH + x;
    
    if(on){
      buffer_[index] |= (1 << bit);
    }else{
      buffer_[index] &= ~(1 << bit);
    }
  }
}

inline bool DRIVER_OLED_SH1107::getPixel(int x, int y){
  if(x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT){
    int page = y / 8;
    int bit = y % 8;
    int index = page * DISPLAY_WIDTH + x;
    return (buffer_[index] & (1 << bit)) != 0;
  }
  return false;
}

inline void DRIVER_OLED_SH1107::drawLine(int x0, int y0, int x1, int y1, bool on){
  // Bresenham's line algorithm
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;
  
  while(true){
    setPixel(x0, y0, on);
    
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

inline void DRIVER_OLED_SH1107::drawRect(int x, int y, int width, int height, bool filled, bool on){
  if(filled){
    for(int i = 0; i < height; i++){
      drawLine(x, y + i, x + width - 1, y + i, on);
    }
  }else{
    drawLine(x, y, x + width - 1, y, on);                          // Top
    drawLine(x, y + height - 1, x + width - 1, y + height - 1, on); // Bottom
    drawLine(x, y, x, y + height - 1, on);                         // Left
    drawLine(x + width - 1, y, x + width - 1, y + height - 1, on);  // Right
  }
}

inline void DRIVER_OLED_SH1107::drawCircle(int center_x, int center_y, int radius, bool filled, bool on){
  if(filled){
    for(int y = -radius; y <= radius; y++){
      for(int x = -radius; x <= radius; x++){
        if(x*x + y*y <= radius*radius){
          setPixel(center_x + x, center_y + y, on);
        }
      }
    }
  }else{
    // Bresenham's circle algorithm
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;
    
    while(y >= x){
      // Draw 8 symmetric points
      setPixel(center_x + x, center_y + y, on);
      setPixel(center_x - x, center_y + y, on);
      setPixel(center_x + x, center_y - y, on);
      setPixel(center_x - x, center_y - y, on);
      setPixel(center_x + y, center_y + x, on);
      setPixel(center_x - y, center_y + x, on);
      setPixel(center_x + y, center_y - x, on);
      setPixel(center_x - y, center_y - x, on);
      
      x++;
      if(d > 0){
        y--;
        d = d + 4 * (x - y) + 10;
      }else{
        d = d + 4 * x + 6;
      }
    }
  }
}

// ============== TEXT RENDERING ==============

inline void DRIVER_OLED_SH1107::drawChar(int x, int y, char c, bool on){
  if(c < 32 || c > 126) c = 32; // Replace invalid chars with space
  
  const uint8_t* char_data = font5x7_[c - 32];
  
  for(int col = 0; col < 5; col++){
    uint8_t column = char_data[col];
    for(int row = 0; row < 7; row++){
      if(column & (1 << row)){
        setPixel(x + col, y + row, on);
      }
    }
  }
}

inline void DRIVER_OLED_SH1107::drawString(int x, int y, const char* str, bool on){
  int pos_x = x;
  while(*str){
    drawChar(pos_x, y, *str, on);
    pos_x += 6; // 5 pixels wide + 1 pixel spacing
    str++;
  }
}

inline void DRIVER_OLED_SH1107::getTextSize(const char* str, int* width, int* height){
  if(width){
    *width = strlen(str) * 6; // 5 pixels + 1 spacing per char
    if(*width > 0) *width -= 1; // Remove last spacing
  }
  if(height){
    *height = 7; // Font is 7 pixels high
  }
}

// ============== DISPLAY UPDATE FUNCTIONS ==============

inline bool DRIVER_OLED_SH1107::updateDisplay(){
  if(!initialized_) return false;
  
  for(int page = 0; page < 16; page++){
    // Set page and column addresses
    if(!writeCommand(0xB0 + page)) return false;
    if(!writeCommand(0x00)) return false;
    if(!writeCommand(0x10)) return false;
    
    // Send entire page (128 bytes) in one transaction
    int page_start = page * 128;
    if(!writeDataStream(&buffer_[page_start], 128)) return false;
  }
  
  return true;
}

inline bool DRIVER_OLED_SH1107::updateSmart(){
  if(!initialized_) return false;
  
  detectChanges();
  
  int pages_sent = 0;
  for(int page = 0; page < PAGES; page++){
    if(!page_dirty_[page]) continue; // Skip unchanged pages
    
    // Set page and column addresses
    if(!writeCommand(0xB0 + page)) return false;
    if(!writeCommand(0x00)) return false;
    if(!writeCommand(0x10)) return false;
    
    // Send entire page (128 bytes) in one transaction
    int page_start = page * 128;
    if(!writeDataStream(&buffer_[page_start], 128)) return false;
    pages_sent++;
  }
  
  return true;
}

inline bool DRIVER_OLED_SH1107::updatePages(int start_page, int end_page){
  if(!initialized_) return false;
  
  if(start_page < 0) start_page = 0;
  if(end_page > 15) end_page = 15;
  
  for(int page = start_page; page <= end_page; page++){
    // Set page and column addresses
    if(!writeCommand(0xB0 + page)) return false;
    if(!writeCommand(0x00)) return false;
    if(!writeCommand(0x10)) return false;
    
    // Send entire page (128 bytes) in one transaction
    int page_start = page * 128;
    if(!writeDataStream(&buffer_[page_start], 128)) return false;
  }
  
  return true;
}

inline bool DRIVER_OLED_SH1107::updateArea(const OLEDRect& rect){
  int start_page, end_page;
  rectToPages(rect, &start_page, &end_page);
  
  return updatePages(start_page, end_page);
}

// ============== ADVANCED BUFFER OPERATIONS ==============

inline void DRIVER_OLED_SH1107::markAllDirty(){
  for(int i = 0; i < PAGES; i++){
    page_dirty_[i] = true;
  }
}

inline void DRIVER_OLED_SH1107::markPagesDirty(int start_page, int end_page){
  if(start_page < 0) start_page = 0;
  if(end_page >= PAGES) end_page = PAGES - 1;
  
  for(int i = start_page; i <= end_page; i++){
    page_dirty_[i] = true;
  }
}

inline void DRIVER_OLED_SH1107::markAreaDirty(const OLEDRect& rect){
  int start_page, end_page;
  rectToPages(rect, &start_page, &end_page);
  markPagesDirty(start_page, end_page);
}

inline bool DRIVER_OLED_SH1107::setUpsideDown(bool upside_down){
  if(!initialized_) return false;
  
  uint8_t seg_remap_cmd = upside_down ? 0xA0 : 0xA1;  // Segment remap
  uint8_t com_scan_cmd = upside_down ? 0xC0 : 0xC8;   // COM output scan direction
  
  if(!writeCommand(seg_remap_cmd)) return false;
  if(!writeCommand(com_scan_cmd)) return false;
  
  return true;
}

inline bool DRIVER_OLED_SH1107::setFlipHorizontal(bool flip){
  if(!initialized_) return false;
  
  // Segment remap: 0xA0 = normal (column 0 mapped to SEG0)
  //                0xA1 = flipped (column 127 mapped to SEG0)
  uint8_t seg_remap_cmd = flip ? 0xA1 : 0xA0;
  
  return writeCommand(seg_remap_cmd);
}

inline bool DRIVER_OLED_SH1107::setFlipVertical(bool flip){
  if(!initialized_) return false;
  
  // COM scan direction: 0xC0 = normal (scan from COM0 to COM[N-1])
  //                     0xC8 = flipped (scan from COM[N-1] to COM0)
  uint8_t com_scan_cmd = flip ? 0xC8 : 0xC0;
  
  return writeCommand(com_scan_cmd);
}

inline int DRIVER_OLED_SH1107::getDirtyPageCount() const{
  int count = 0;
  for(int i = 0; i < PAGES; i++){
    if(page_dirty_[i]) count++;
  }
  return count;
}

inline bool DRIVER_OLED_SH1107::isPageDirty(int page) const{
  if(page < 0 || page >= PAGES) return false;
  return page_dirty_[page];
}

// ============== UTILITY FUNCTIONS ==============

inline int DRIVER_OLED_SH1107::yToPage(int y) const{
  if(y < 0 || y >= DISPLAY_HEIGHT) return -1;
  return y / 8;
}

inline void DRIVER_OLED_SH1107::rectToPages(const OLEDRect& rect, int* start_page, int* end_page) const{
  if(start_page == nullptr || end_page == nullptr) return;
  
  *start_page = yToPage(rect.y);
  *end_page = yToPage(rect.y + rect.height - 1);
  
  // Clamp to valid range
  if(*start_page < 0) *start_page = 0;
  if(*end_page >= PAGES) *end_page = PAGES - 1;
  if(*start_page > *end_page) *start_page = *end_page;
}

} // namespace abstraction
} // namespace arcos

#endif // ARCOS_ABSTRACTION_DRIVERS_OLED_SH1107_IMPL_HPP_
