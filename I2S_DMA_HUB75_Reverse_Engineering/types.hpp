#pragma once

#include <stdint.h>
#include <vector>
#include <memory>
#include "esp_heap_caps.h"
#include <Adafruit_GFX.h>

#if __has_include(<esp_arduino_version.h>)
 #include <esp_arduino_version.h>
#endif

#define getRowDataPtr(row, _dpth) &(fb->rowBits[row]->data[_dpth * fb->rowBits[row]->width])

#define DMA_MAX (4096-4)
#define CLKS_DURING_LATCH 0                 // Not (yet) used.

#define BITS_RGB1_OFFSET 0 // Start point of RGB_X1 bits
#define BIT_R1 (1 << 0)
#define BIT_G1 (1 << 1)
#define BIT_B1 (1 << 2)

#define BITMASK_OE_CLEAR (0b1111111101111111)    // inverted bitmask for control bit OE in pixel vector
#define PIXEL_COLOR_MASK_BIT(color_depth_index, mask_offset) (1 << (color_depth_index + mask_offset))

// Panel Lower half RGB
#define BITS_RGB2_OFFSET 3 // Start point of RGB_X2 bits
#define BIT_R2 (1 << 3)
#define BIT_G2 (1 << 4)
#define BIT_B2 (1 << 5)

// Panel Control Signals
#define BIT_LAT (1 << 6)
#define BIT_OE (1 << 7)

// Panel GPIO Pin Addresses (A, B, C, D etc..)
#define BITS_ADDR_OFFSET 8 // Start point of address bits
#define BIT_A (1 << 8)
#define BIT_B (1 << 9)
#define BIT_C (1 << 10)
#define BIT_D (1 << 11)
#define BIT_E (1 << 12)

#define BITMASK_RGB1_CLEAR (0b1111111111111000)  // inverted bitmask for R1G1B1 bit in pixel vector
#define BITMASK_RGB2_CLEAR (0b1111111111000111)  // inverted bitmask for R2G2B2 bit in pixel vector
#define BITMASK_RGB12_CLEAR (0b1111111111000000) // inverted bitmask for R1G1B1R2G2B2 bit in pixel vector
#define BITMASK_CTRL_CLEAR (0b1110000000111111)  // inverted bitmask for control bits ABCDE,LAT,OE in pixel vector
#define BITMASK_OE_CLEAR (0b1111111101111111)    // inverted bitmask for control bit OE in pixel vector


#if defined(ESP32_THE_ORIG)
#define ESP32_TX_FIFO_POSITION_ADJUST(x_coord) (((x_coord)&1U) ? (x_coord - 1) : (x_coord + 1))
#else
#define ESP32_TX_FIFO_POSITION_ADJUST(x_coord) x_coord
#endif

#ifndef MATRIX_WIDTH
#define MATRIX_WIDTH 64 // Single panel of 64 pixel width
#endif

#ifndef MATRIX_HEIGHT
#define MATRIX_HEIGHT 32 // CHANGE THIS VALUE to 64 IF USING 64px HIGH panel(s) with E PIN
#endif

#ifndef CHAIN_LENGTH
#define CHAIN_LENGTH 1 // Number of modules chained together, i.e. 4 panels chained result in virtualmatrix 64x4=256 px long
#endif

#define MATRIX_ROWS_IN_PARALLEL 2

#define R1_PIN_DEFAULT 4
#define G1_PIN_DEFAULT 5
#define B1_PIN_DEFAULT 6
#define R2_PIN_DEFAULT 7
#define G2_PIN_DEFAULT 15
#define B2_PIN_DEFAULT 16
#define A_PIN_DEFAULT  18
#define B_PIN_DEFAULT  8
#define C_PIN_DEFAULT  3
#define D_PIN_DEFAULT  42
#define E_PIN_DEFAULT  -1 // required for 1/32 scan panels, like 64x64. Any available pin would do, i.e. IO32
#define LAT_PIN_DEFAULT 40
#define OE_PIN_DEFAULT  2
#define CLK_PIN_DEFAULT 41

#define DEFAULT_LAT_BLANKING 2

#ifdef PIXEL_COLOUR_DEPTH_BITS
#define PIXEL_COLOR_DEPTH_BITS PIXEL_COLOUR_DEPTH_BITS
#endif

// support backwarts compatibility
#ifdef PIXEL_COLOR_DEPTH_BITS
#define PIXEL_COLOR_DEPTH_BITS_DEFAULT PIXEL_COLOR_DEPTH_BITS
#else
#define PIXEL_COLOR_DEPTH_BITS_DEFAULT 8
#endif

#define PIXEL_COLOR_DEPTH_BITS_MAX 12

struct HUB75_I2S_CFG
{

  /**
   * Enumeration of hardware-specific chips
   * used to drive matrix modules
   */
  enum shift_driver
  {
    SHIFTREG = 0,
    FM6124,
    FM6126A,
    ICN2038S,
    MBI5124,
    DP3246
  };

  enum line_driver
  {
    TYPE138 = 0,    // 3-to-8 decoder
    TYPE595,        // shift register decoder
    TYPE_DIRECT,    // direct row control 
    SM5266P,        // shift register decoder with DE control
    SM5368 = TYPE595
  };
  /**
   * I2S clock speed selector
   */
  enum clk_speed
  {
    HZ_8M = 8000000,
    HZ_10M = 8000000,
    HZ_15M = 16000000, // for compatability
    HZ_16M = 16000000,
    HZ_20M = 20000000 // for compatability  
  };

  //
  // Members must be in order of declaration or it breaks Arduino compiling due to strict checking.
  //

  // physical width of a single matrix panel module (in pixels, usually it is 64 ;) )
  uint16_t mx_width;

  // physical height of a single matrix panel module (in pixels, usually almost always it is either 32 or 64)
  uint16_t mx_height;

  // number of chained panels regardless of the topology, default 1 - a single matrix module
  uint16_t chain_length;

  // GPIO Mapping
  struct i2s_pins
  {
    int8_t r1, g1, b1, r2, g2, b2, a, b, c, d, e, lat, oe, clk;
  } gpio;

  // Matrix driver chip type - default is a plain shift register
  shift_driver driver;
  line_driver line_decoder;

  // use DMA double buffer (twice as much RAM required)
  bool double_buff;

  // I2S clock speed
  clk_speed i2sspeed;

  // How many clock cycles to blank OE before/after LAT signal change, default is 1 clock
  uint8_t latch_blanking;

  /**
   *  I2S clock phase
   *  0 - data lines are clocked with negative edge
   *  Clk  /¯\_/¯\_/
   *  LAT  __/¯¯¯\__
   *  EO   ¯¯¯¯¯¯\___
   *
   *  1 - data lines are clocked with positive edge (default now as of 10 June 2021)
   *  https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA/issues/130
   *  Clk  \_/¯\_/¯\
   *  LAT  __/¯¯¯\__
   *  EO   ¯¯¯¯¯¯\__
   *
   */
  bool clkphase;

  // Minimum refresh / scan rate needs to be configured on start due to LSBMSB_TRANSITION_BIT calculation in allocateDMAmemory()
  // Set this to '1' to get all colour depths displayed with correct BCM time weighting.
  uint8_t min_refresh_rate;

  // struct constructor
  HUB75_I2S_CFG(
      uint16_t _w = MATRIX_WIDTH,
      uint16_t _h = MATRIX_HEIGHT,
      uint16_t _chain = CHAIN_LENGTH,
      i2s_pins _pinmap = {
          R1_PIN_DEFAULT, G1_PIN_DEFAULT, B1_PIN_DEFAULT, R2_PIN_DEFAULT, G2_PIN_DEFAULT, B2_PIN_DEFAULT,
          A_PIN_DEFAULT, B_PIN_DEFAULT, C_PIN_DEFAULT, D_PIN_DEFAULT, E_PIN_DEFAULT,
          LAT_PIN_DEFAULT, OE_PIN_DEFAULT, CLK_PIN_DEFAULT},
      shift_driver _drv = SHIFTREG, 
      line_driver _line_drv = TYPE138,
      bool _dbuff = false, 
      clk_speed _i2sspeed = HZ_8M,
      uint8_t _latblk = DEFAULT_LAT_BLANKING, // Anything > 1 seems to cause artefacts on ICS panels
      bool _clockphase = true, 
      uint16_t _min_refresh_rate = 60, 
      uint8_t _pixel_color_depth_bits = PIXEL_COLOR_DEPTH_BITS_DEFAULT) 
      : mx_width(_w), mx_height(_h), chain_length(_chain), gpio(_pinmap), driver(_drv), double_buff(_dbuff), i2sspeed(_i2sspeed), latch_blanking(_latblk), clkphase(_clockphase), min_refresh_rate(_min_refresh_rate)
  {
    setPixelColorDepthBits(_pixel_color_depth_bits);
  }

  // pixel_color_depth_bits must be between 12 and 2, and mask_offset needs to be calculated accordently
  // so they have to be private with getter (and setter)
  void setPixelColorDepthBits(uint8_t _pixel_color_depth_bits)
  {
    if (_pixel_color_depth_bits > PIXEL_COLOR_DEPTH_BITS_MAX || _pixel_color_depth_bits < 2)
    {

      if (_pixel_color_depth_bits > PIXEL_COLOR_DEPTH_BITS_MAX)
      {
        pixel_color_depth_bits = PIXEL_COLOR_DEPTH_BITS_MAX;
      }
      else
      {
        pixel_color_depth_bits = 2;
      }
    }
    else
    {
      pixel_color_depth_bits = _pixel_color_depth_bits;
    }
  }

  uint8_t getPixelColorDepthBits() const
  {
    return pixel_color_depth_bits;
  }

private:
  // these were priviously handeld as defines (PIXEL_COLOR_DEPTH_BITS, MASK_OFFSET)
  // to make it changable after compilation, it is now part of the config
  uint8_t pixel_color_depth_bits;
}; // end of structure HUB75_I2S_CFG


#define ESP32_I2S_DMA_STORAGE_TYPE uint16_t // DMA output of one uint16_t at a time.


struct rowBitStruct
/**
 * @struct rowBitStruct
 * @brief Structure to hold row data for HUB75 matrix panel DMA operations
 * 
 * @var width
 * Width of the row in pixels
 * 
 * @var colour_depth
 * Number of color depths (i.e. copies of each row for each colur bitmask)
 * 
 * @var data
 * Pointer to DMA storage type array holding pixel data for the row
 * 
 * @note Memory allocation differs based on target platform and configuration:
 * - For ESP32-S3 with SPIRAM: Allocates aligned memory in SPIRAM
 * - For other configurations: Allocates DMA-capable internal memory
 */
{
  const size_t width;
  const uint8_t colour_depth;
  //const bool double_buff;
  ESP32_I2S_DMA_STORAGE_TYPE *data;

  /** @brief Returns size (in bytes) of a colour depth row data array
   * 
   * @param single_color_depth 
   *        - if true, returns size for a single color depth layer
   *        - if false, returns total size for all color depth layers for a row.
   * 
   * @returns size_t - Size in bytes required for DMA buffer allocation
   * 
   */
  size_t getColorDepthSize(bool single_color_depth)
  {
    int _cdepth = (single_color_depth) ? 1:colour_depth;
    return width * _cdepth * sizeof(ESP32_I2S_DMA_STORAGE_TYPE);
  };


  /** @brief
   * Returns pointer to the row's data vector beginning at pixel[0] for _dpth colour bit
   * 
   * NOTE: this call might be very slow in loops. Due to poor instruction caching in esp32 it might be required a reread from flash
   * every loop cycle, better use inlined #define instead in such cases
   */
  inline ESP32_I2S_DMA_STORAGE_TYPE *getDataPtr(const uint8_t _dpth = 0) { return &(data[_dpth * width]); };

  // constructor - allocates DMA-capable memory to hold the struct data
  //rowBitStruct(const size_t _width, const uint8_t _depth, const bool _dbuff) : width(_width), colour_depth(_depth), double_buff(_dbuff)
  rowBitStruct(const size_t _width, const uint8_t _depth) : width(_width), colour_depth(_depth)
  {

    // #if defined(SPIRAM_FRAMEBUFFER) && defined (CONFIG_IDF_TARGET_ESP32S3)
#if defined(SPIRAM_DMA_BUFFER)

    // data = (ESP32_I2S_DMA_STORAGE_TYPE *)heap_caps_aligned_alloc(64, size()+size()*double_buff, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    // No longer have double buffer in the same struct - have a different struct
    data = (ESP32_I2S_DMA_STORAGE_TYPE *)heap_caps_aligned_alloc(64, getColorDepthSize(false), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    // data = (ESP32_I2S_DMA_STORAGE_TYPE *)heap_caps_malloc( size()+size()*double_buff, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

    // No longer have double buffer in the same struct - have a different struct
    data = (ESP32_I2S_DMA_STORAGE_TYPE *)heap_caps_malloc(getColorDepthSize(false), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

#endif
  }
  ~rowBitStruct() { delete data; }
};

struct frameStruct
{
  uint8_t rows = 0; // number of rows held in current frame, not used actually, just to keep the idea of struct
  std::vector<std::shared_ptr<rowBitStruct>> rowBits;
};