#include "types.hpp"
#include <stddef.h>
#include "gdma_lcd_parallel16.hpp"

static const uint16_t lumConvTab[] = {
0,   28,   57,   85,  114,  142,  171,  199,  228,  256,  285,  313,  342,  370,  399,  427,
456,  484,  513,  541,  570,  598,  627,  658,  689,  721,  755,  789,  825,  861,  899,  937,
977, 1018, 1060, 1103, 1147, 1192, 1239, 1287, 1336, 1386, 1437, 1490, 1544, 1599, 1656, 1714,
1773, 1834, 1896, 1959, 2024, 2090, 2157, 2226, 2297, 2369, 2442, 2517, 2593, 2671, 2751, 2832,
2914, 2999, 3085, 3172, 3261, 3352, 3444, 3538, 3634, 3732, 3831, 3932, 4035, 4139, 4245, 4354,
4464, 4575, 4689, 4804, 4922, 5041, 5162, 5285, 5410, 5537, 5666, 5797, 5930, 6065, 6202, 6341,
6482, 6626, 6771, 6918, 7068, 7220, 7373, 7529, 7687, 7848, 8010, 8175, 8342, 8512, 8683, 8857,
9033, 9212, 9393, 9576, 9762, 9949, 10140, 10333, 10528, 10725, 10926, 11128, 11333, 11541, 11751, 11963,
12179, 12396, 12617, 12840, 13065, 13293, 13524, 13757, 13993, 14232, 14474, 14718, 14965, 15215, 15467, 15722,
15980, 16241, 16505, 16771, 17041, 17313, 17588, 17866, 18147, 18431, 18717, 19007, 19300, 19596, 19894, 20196,
20501, 20809, 21119, 21433, 21750, 22071, 22394, 22720, 23050, 23383, 23719, 24058, 24400, 24746, 25095, 25447,
25802, 26161, 26523, 26888, 27257, 27629, 28004, 28383, 28765, 29151, 29540, 29932, 30328, 30728, 31131, 31537,
31947, 32360, 32777, 33198, 33622, 34050, 34481, 34916, 35355, 35797, 36243, 36693, 37146, 37603, 38064, 38529,
38997, 39469, 39945, 40425, 40908, 41396, 41887, 42382, 42881, 43384, 43891, 44401, 44916, 45435, 45957, 46484,
47015, 47549, 48088, 48631, 49178, 49728, 50283, 50843, 51406, 51973, 52545, 53120, 53700, 54284, 54873, 55465,
56062, 56663, 57269, 57878, 58492, 59111, 59733, 60360, 60992, 61627, 62268, 62912, 63561, 64215, 64873, 65535,
};

class MatrixPanel_I2S_DMA : public Adafruit_GFX {
public:
  // Constructor: pass HUB75_I2S_CFG and also call Adafruit_GFX constructor
  MatrixPanel_I2S_DMA(const HUB75_I2S_CFG &opts)
    : Adafruit_GFX(opts.mx_width * opts.chain_length, opts.mx_height)  // initializes _width, _height, rotation
  {
    setCfg(opts);  // apply HUB75 config to m_cfg and recalc PIXELS_PER_ROW, etc.
  }
// ================================================= Global Variables ================================================= //
HUB75_I2S_CFG m_cfg;
frameStruct frame_buffer[2];
frameStruct *fb; // What framebuffer we are writing pixel changes to? (pointer to either frame_buffer[0] or frame_buffer[1] basically ) used within updateMatrixDMABuffer(...)
volatile int back_buffer_id = 0;      // If using double buffer, which one is NOT active (ie. being displayed) to write too?
int brightness = 128;        // If you get ghosting... reduce brightness level. ((60/64)*255) seems to be the limit before ghosting on a 64 pixel wide physical panel for some panels.
int lsbMsbTransitionBit = 0; // For colour depth calculations
int calculated_refresh_rate = 0;
/* ESP32-HUB75-MatrixPanel-I2S-DMA functioning constants
 * we should not those once object instance initialized it's DMA structs
 * they weree const, but this lead to bugs, when the default constructor was called.
 * So now they could be changed, but shouldn't. Maybe put a cpp lock around it, so it can't be changed after initialisation
 */
uint16_t PIXELS_PER_ROW = m_cfg.mx_width * m_cfg.chain_length;      // number of pixels in a single row of all chained matrix modules (WIDTH of a combined matrix chain)
uint8_t ROWS_PER_FRAME = m_cfg.mx_height / MATRIX_ROWS_IN_PARALLEL; // RPF - rows per frame, either 16 or 32 depending on matrix module
uint8_t MASK_OFFSET = 16 - m_cfg.getPixelColorDepthBits();

Bus_Parallel16 dma_bus;

// ################################################# Global Variables ################################################# //
// ================================================= Function List ================================================= //
// void setupDMA(const HUB75_I2S_CFG &_cfg);
// inline void resetbuffers();
// void clearFrameBuffer(bool _buff_id);
// void setBrightnessOE(uint8_t brt, const int _buff_id);
// void begin(int r1, int g1, int b1, int r2, int g2, int b2, int a, int b, int c, int d, int e, int lat, int oe, int clk);
// ################################################# Function List ################################################# //


// ================================================= START begin() ================================================= //
void begin(){
  /* As DMA buffers are dynamically allocated, we must allocated in begin()
   * Ref: https://github.com/espressif/arduino-esp32/issues/831
   */
  setupDMA(m_cfg);

  // Flush the DMA buffers prior to configuring DMA - Avoid visual artefacts on boot.
  resetbuffers(); // Must fill the DMA buffer with the initial output bit sequence or the panel will display garbage

	// Start output output
	dma_bus.init();
	
	dma_bus.dma_transfer_start();
}
// ################################################# END begin() ################################################# //


// ================================================= START setupDMA(const HUB75_I2S_CFG &) ================================================= //
void setupDMA(const HUB75_I2S_CFG &_cfg)
{
  
  /***
   * Step 0: Allocate basic DMA framebuffer memory for the data we send out in parallel to the HUB75 panel.
   *         Colour depth is the only consideration.
   * 
   */

  size_t allocated_fb_memory = 0;

  int fbs_required = (m_cfg.double_buff) ? 2 : 1;

  for (int fb = 0; fb < (fbs_required); fb++)
  {
    frame_buffer[fb].rowBits.reserve(ROWS_PER_FRAME);

    for (int malloc_num = 0; malloc_num < ROWS_PER_FRAME; malloc_num++)
    {
      auto ptr = std::make_shared<rowBitStruct>(PIXELS_PER_ROW, m_cfg.getPixelColorDepthBits());

      allocated_fb_memory += ptr->getColorDepthSize(false); // byte required to display all colour depths for the two parallel rows
      frame_buffer[fb].rowBits.emplace_back(ptr); // save new rowBitStruct pointer into rows vector
      ++frame_buffer[fb].rows;
    }
  }

  /***
   * Step 1: Check what the minimum refresh rate is, and calculate the lsbMsbTransitionBit
   *         which is the bit at which we can reduce the colour depth to achieve the minimum refresh rate.
   * 
   *         This also determines the number of DMA descriptors required per row.
   */  
   
//#define FORCE_COLOR_DEPTH 1   
  
#if !defined(FORCE_COLOR_DEPTH)
  while (1)
  {
    int psPerClock = 1000000000000UL / m_cfg.i2sspeed;
    int nsPerLatch = ((PIXELS_PER_ROW + CLKS_DURING_LATCH) * psPerClock) / 1000; // time per row

    // add time to shift out LSBs + LSB-MSB transition bit - this ignores fractions...
    int nsPerRow = m_cfg.getPixelColorDepthBits() * nsPerLatch;

    // Now add the time for the remaining bit depths
    for (int i = lsbMsbTransitionBit + 1; i < m_cfg.getPixelColorDepthBits(); i++) {
      //nsPerRow += (1 << (i - lsbMsbTransitionBit - 1)) * (m_cfg.getPixelColorDepthBits() - i) * nsPerLatch;
	  nsPerRow += (1 << (i - lsbMsbTransitionBit - 1)) *  nsPerLatch;
	}

    int nsPerFrame = nsPerRow * ROWS_PER_FRAME;
    int actualRefreshRate = 1000000000UL / (nsPerFrame);
    calculated_refresh_rate = actualRefreshRate;

    if (actualRefreshRate > m_cfg.min_refresh_rate)
      break;

    if (lsbMsbTransitionBit < m_cfg.getPixelColorDepthBits() - 1)
      lsbMsbTransitionBit++;
    else
      break;
  }


#endif

  /***
   * Step 2:  Calculate the DMA descriptors required, which is used for memory allocation of the DMA linked list memory structure.
   *          This determines the number of passes required to shift out the colour bits in the framebuffer.
   *          We need to also take into consderation where a chain of panels (pixels) is so long, it requires more than one DMA payload,
   *          give this library's DMA output memory allocation approach is by the row.
   */
	
  int    dma_descs_per_row_1cdepth	 	= (frame_buffer[0].rowBits[0]->getColorDepthSize(true) + DMA_MAX - 1 ) / DMA_MAX;
  size_t last_dma_desc_bytes_1cdepth    = (frame_buffer[0].rowBits[0]->getColorDepthSize(true) % DMA_MAX);
  
  int    dma_descs_per_row_all_cdepths	  = (frame_buffer[0].rowBits[0]->getColorDepthSize(false) + DMA_MAX - 1 ) / DMA_MAX;
  size_t last_dma_desc_bytes_all_cdepths  = (frame_buffer[0].rowBits[0]->getColorDepthSize(false) % DMA_MAX);

  // Calculate per-row number
  int dma_descriptors_per_row = dma_descs_per_row_all_cdepths;

  // Add descriptors for MSB bits after transition
  for (int i = lsbMsbTransitionBit + 1; i < m_cfg.getPixelColorDepthBits(); i++) {
    dma_descriptors_per_row += (1 << (i - lsbMsbTransitionBit - 1)) * dma_descs_per_row_1cdepth;
  }
  
  //dma_descriptors_per_row = 1;

  // Allocate DMA descriptors 
  int dma_descriptions_required = dma_descriptors_per_row * ROWS_PER_FRAME;

  /***
   * Step 3:  Allocate the DMA descriptor memory via. the relevant platform DMA implementation class.
   */

  if (m_cfg.double_buff) {
    dma_bus.enable_double_dma_desc();
  }

  dma_bus.allocate_dma_desc_memory(dma_descriptions_required);



  /***
   * Step 4:  Link up the DMA descriptors per the colour depth and rows.
   */

  //fbs_required = 1; // (m_cfg.double_buff) ? 2 : 1;
  for (int fb = 0; fb < (fbs_required); fb++)
  {  
	
	int _dmadescriptor_count = 0; // for tracking
	
    for (int row = 0; row < ROWS_PER_FRAME; row++)
    {
	  //ESP_LOGV("I2S-DMA", ">>> Linking DMA descriptors for output row %d", row);    	
		
	  // Link and send all colour data, all passes of everything in one hit. 1 bit colour at least...
	  for (int dma_desc_all = 0; dma_desc_all < dma_descs_per_row_all_cdepths; dma_desc_all++) 
	  {
			size_t payload_bytes = (dma_desc_all == (dma_descs_per_row_all_cdepths-1)) ? last_dma_desc_bytes_all_cdepths:DMA_MAX;
			
			// Log the current descriptor number and the payload size being used.
			//ESP_LOGV("I2S-DMA", "Processing dma_desc_all: %d, payload_bytes: %zu, memory location: %p", dma_desc_all, payload_bytes, (frame_buffer[fb].rowBits[row]->getDataPtr(0)+(dma_desc_all*(DMA_MAX/sizeof(ESP32_I2S_DMA_STORAGE_TYPE)))));
				
		    dma_bus.create_dma_desc_link(frame_buffer[fb].rowBits[row]->getDataPtr(0)+(dma_desc_all*(DMA_MAX/sizeof(ESP32_I2S_DMA_STORAGE_TYPE))), payload_bytes, (fb==1));
			_dmadescriptor_count++;
			
			// Log the updated descriptor count after each operation.
			//ESP_LOGV("I2S-DMA", "Updated _dmadescriptor_count: %d", _dmadescriptor_count);			
	  }
	
      // Step 2: Handle additional descriptors for bits beyond the lsbMsbTransitionBit 
      for (int i = lsbMsbTransitionBit + 1; i < m_cfg.getPixelColorDepthBits(); i++) 
	  {
		  // binary time division setup: we need 2 of bit (LSBMSB_TRANSITION_BIT + 1) four of (LSBMSB_TRANSITION_BIT + 2), etc
		  // because we sweep through to MSB each time, it divides the number of times we have to sweep in half (saving linked list RAM)
		  // we need 2^(i - LSBMSB_TRANSITION_BIT - 1) == 1 << (i - LSBMSB_TRANSITION_BIT - 1) passes from i to MSB

		  for (int k = 0; k < (1 << (i - lsbMsbTransitionBit - 1)); k++)
		  {		  
			  // Link and send all colour data, all passes of everything in one hit.
			  for (int dma_desc_1cdepth = 0; dma_desc_1cdepth < dma_descs_per_row_1cdepth; dma_desc_1cdepth++) 
			  {		  
				size_t payload_bytes = (dma_desc_1cdepth == (dma_descs_per_row_1cdepth-1)) ? last_dma_desc_bytes_1cdepth:DMA_MAX;
				
				// Log the current bit and the corresponding payload size.
				//ESP_LOGV("I2S-DMA", "Processing dma_desc_1cdepth: %d, payload_bytes: %zu, memory location: %p", dma_desc_1cdepth, payload_bytes, (frame_buffer[fb].rowBits[row]->getDataPtr(i)+(dma_desc_1cdepth*(DMA_MAX/sizeof(ESP32_I2S_DMA_STORAGE_TYPE)))));
		
				dma_bus.create_dma_desc_link(frame_buffer[fb].rowBits[row]->getDataPtr(i)+(dma_desc_1cdepth*(DMA_MAX/sizeof(ESP32_I2S_DMA_STORAGE_TYPE))), payload_bytes, (fb==1));
				_dmadescriptor_count++;
				
				// Log the updated descriptor count after each operation.
			//	ESP_LOGV("I2S-DMA", "Updated _dmadescriptor_count: %d", _dmadescriptor_count);		
			  }
		  } // end K
		
      } // end all other colour depth bits
	  

    } // end all rows
		
  } // end framebuffer loop

  /***
   * Step 5:  Set default framebuffer to fb[0]
   */  

  fb = &frame_buffer[0];
  

  //
  //    Setup DMA and Output to GPIO
  //
  auto bus_cfg = dma_bus.config(); // バス設定用の構造体を取得します。

  bus_cfg.bus_freq    = m_cfg.i2sspeed;
  bus_cfg.pin_wr      = m_cfg.gpio.clk;
  bus_cfg.invert_pclk = m_cfg.clkphase;

  bus_cfg.pin_d0 = m_cfg.gpio.r1;
  bus_cfg.pin_d1 = m_cfg.gpio.g1;
  bus_cfg.pin_d2 = m_cfg.gpio.b1;
  bus_cfg.pin_d3 = m_cfg.gpio.r2;
  bus_cfg.pin_d4 = m_cfg.gpio.g2;
  bus_cfg.pin_d5 = m_cfg.gpio.b2;
  bus_cfg.pin_d6 = m_cfg.gpio.lat;
  bus_cfg.pin_d7 = m_cfg.gpio.oe;
  bus_cfg.pin_d8 = m_cfg.gpio.a;
  bus_cfg.pin_d9 = m_cfg.gpio.b;
  bus_cfg.pin_d10 = m_cfg.gpio.c;
  bus_cfg.pin_d11 = m_cfg.gpio.d;
  bus_cfg.pin_d12 = m_cfg.gpio.e;
  bus_cfg.pin_d13 = -1;
  bus_cfg.pin_d14 = -1;
  bus_cfg.pin_d15 = -1;

  dma_bus.config(bus_cfg);

} // end setupDMA

// ################################################# END setupDMA(const HUB75_I2S_CFG &) ################################################# //
// ================================================= START resetBuffers() ================================================= //
inline void resetbuffers(){
  clearFrameBuffer(0);        
  setBrightnessOE(brightness, 0); 

  if (m_cfg.double_buff) {
	
    clearFrameBuffer(1);        
    setBrightnessOE(brightness, 1);

  }
}
// ################################################# END resetBuffers() ################################################# //
// ================================================= START clearFrameBuffer(bool _buff_id) ================================================= //

void clearFrameBuffer(bool _buff_id){


  frameStruct *fb = &frame_buffer[_buff_id];

  // we start with iterating all rows in dma_buff structure
  int row_idx = fb->rowBits.size();
  do
  {
    --row_idx;

    ESP32_I2S_DMA_STORAGE_TYPE *row = fb->rowBits[row_idx]->getDataPtr(0); // set pointer to the HEAD of a buffer holding data for the entire matrix row
    ESP32_I2S_DMA_STORAGE_TYPE abcde = (ESP32_I2S_DMA_STORAGE_TYPE)row_idx;

    // get last pixel index in a row of all colourdepths
    int x_pixel = fb->rowBits[row_idx]->width * fb->rowBits[row_idx]->colour_depth;

	abcde <<= BITS_ADDR_OFFSET; // shift row y-coord to match ABCDE bits in vector from 8 to 12
	do
	{
		--x_pixel;
		if (m_cfg.line_decoder == HUB75_I2S_CFG::SM5266P)
		{
			// modifications here for row shift register type SM5266P
			// https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA/issues/164
			row[x_pixel] = abcde & (0x18 << BITS_ADDR_OFFSET); // mask out the bottom 3 bits which are the clk di bk inputs
		}
		else if (m_cfg.line_decoder  == HUB75_I2S_CFG::SM5368) 
		{
			row[ESP32_TX_FIFO_POSITION_ADJUST(x_pixel)] = 0x0000;
		}
		else
		{
			row[ESP32_TX_FIFO_POSITION_ADJUST(x_pixel)] = abcde;
		}

	} while (x_pixel != fb->rowBits[row_idx]->width); // spare the first "width's" worth of pixels as they are the LSB pixels/colordepth

	// The colour_index[0] (LSB) x_pixels must be "marked" with a previous's row address, because it is used to display
	// previous row while we pump in MSBs's for the next row.
	if (row_idx == 0) { 
		abcde = ROWS_PER_FRAME-1; // wrap around
	} else {
		abcde = row_idx-1;	
	}

    abcde <<= BITS_ADDR_OFFSET; // shift row y-coord to match ABCDE bits in vector from 8 to 12		
	do
    {
      --x_pixel;

      if (m_cfg.line_decoder == HUB75_I2S_CFG::SM5266P)
      {
        // modifications here for row shift register type SM5266P
        // https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA/issues/164
        row[x_pixel] = abcde & (0x18 << BITS_ADDR_OFFSET); // mask out the bottom 3 bits which are the clk di bk inputs
      }
      else if (m_cfg.line_decoder  == HUB75_I2S_CFG::SM5368) 
      {
        row[ESP32_TX_FIFO_POSITION_ADJUST(x_pixel)] = 0x0000;
      }
      else
      {
        row[ESP32_TX_FIFO_POSITION_ADJUST(x_pixel)] = abcde;
      }

    } while (x_pixel);

    // modifications here for row shift register type SM5266P
    // https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA/issues/164
    if (m_cfg.line_decoder == HUB75_I2S_CFG::SM5266P)
    {
      uint16_t serialCount;
      uint16_t latch;
      x_pixel = fb->rowBits[row_idx]->width - 16; // come back 8*2 pixels to allow for 8 writes
      serialCount = 8;
      do
      {
        serialCount--;
        latch = row[x_pixel] | (((((ESP32_I2S_DMA_STORAGE_TYPE)row_idx) % 8) == serialCount) << 1) << BITS_ADDR_OFFSET; // data on 'B'
        row[x_pixel++] = latch | (0x05 << BITS_ADDR_OFFSET);                                                            // clock high on 'A'and BK high for update
        row[x_pixel++] = latch | (0x04 << BITS_ADDR_OFFSET);                                                            // clock low on 'A'and BK high for update
      } while (serialCount);
    } // end SM5266P

    // row selection for SM5368 shift regs with ABC-only addressing. A is row clk, B is BK and C is row data
    if (m_cfg.line_decoder == HUB75_I2S_CFG::SM5368) 
    {
      x_pixel = fb->rowBits[row_idx]->width - 1;                                                                        // last pixel in first block)
      uint16_t c = (row_idx == 0) ? BIT_C : 0x0000;                                                                     // set row data (C) when row==0, then push through shift regs for all other rows
      row[ESP32_TX_FIFO_POSITION_ADJUST(x_pixel - 1)] |= c | BIT_B;                                                            // set row data
      row[ESP32_TX_FIFO_POSITION_ADJUST(x_pixel + 0)] |= c | BIT_A | BIT_B;                                             // set row clk and bk, carry row data
    } // end DP3246_SM5368

    // let's set LAT/OE control bits for specific pixels in each colour_index subrows
    // Need to consider the original ESP32's (WROOM) DMA TX FIFO reordering of bytes...
    uint8_t colouridx = fb->rowBits[row_idx]->colour_depth;
    do
    {
      --colouridx;

      // switch pointer to a row for a specific colour index
      row = fb->rowBits[row_idx]->getDataPtr(colouridx);

      // DP3246 needs the latch high for 3 clock cycles, so start 2 cycles earlier
      if (m_cfg.driver == HUB75_I2S_CFG::DP3246) 
      {
        row[ESP32_TX_FIFO_POSITION_ADJUST(fb->rowBits[row_idx]->width - 3)] |= BIT_LAT;   // DP3246 needs 3 clock cycle latch 
        row[ESP32_TX_FIFO_POSITION_ADJUST(fb->rowBits[row_idx]->width - 2)] |= BIT_LAT;   // DP3246 needs 3 clock cycle latch 
      } // DP3246_SM5368
      
      row[ESP32_TX_FIFO_POSITION_ADJUST(fb->rowBits[row_idx]->width - 1)] |= BIT_LAT; // -1 pixel to compensate array index starting at 0

      // ESP32_TX_FIFO_POSITION_ADJUST(dma_buff.rowBits[row_idx]->width - 1)

      // need to disable OE before/after latch to hide row transition
      // Should be one clock or more before latch, otherwise can get ghosting
      uint8_t _blank = m_cfg.latch_blanking;
      do
      {
        --_blank;

        row[ESP32_TX_FIFO_POSITION_ADJUST(0 + _blank)] |= BIT_OE;                               // disable output
        row[ESP32_TX_FIFO_POSITION_ADJUST(fb->rowBits[row_idx]->width - 1)] |= BIT_OE;          // disable output
        row[ESP32_TX_FIFO_POSITION_ADJUST(fb->rowBits[row_idx]->width - _blank - 1)] |= BIT_OE; // (LAT pulse is (width-2) -1 pixel to compensate array index starting at 0

      } while (_blank);

    } while (colouridx);


  } while (row_idx);
}

// ################################################# END clearFrameBuffer(bool _buff_id) ################################################# //
// ================================================= START setBrightnessOE(uint8_t brt, const int _buff_id) ================================================= //

void setBrightnessOE(uint8_t brt, const int _buff_id){

  frameStruct *fb = &frame_buffer[_buff_id];

  uint8_t _blank = m_cfg.latch_blanking; // don't want to inadvertantly blast over this
  uint8_t _depth = fb->rowBits[0]->colour_depth;
  uint16_t _width = fb->rowBits[0]->width;

  // start with iterating all rows in dma_buff structure
  int row_idx = fb->rowBits.size();
  do
  {
    --row_idx;

    // let's set OE control bits for specific pixels in each color_index subrows
    uint8_t colouridx = _depth;
    do
    {
      --colouridx;

      char bitplane = (2 * _depth - colouridx) % _depth;
      char bitshift = (_depth - lsbMsbTransitionBit - 1) >> 1;

      char rightshift = std::max(bitplane - bitshift - 2, 0);
      // calculate the OE disable period by brightness, and also blanking
      int brightness_in_x_pixels = ((_width - _blank) * brt) >> (7 + rightshift);
      brightness_in_x_pixels = (brightness_in_x_pixels >> 1) | (brightness_in_x_pixels & 1);

      // switch pointer to a row for a specific color index
      ESP32_I2S_DMA_STORAGE_TYPE *row = fb->rowBits[row_idx]->getDataPtr(colouridx);

      // define range of Output Enable on the center of the row
      int x_coord_max = (_width + brightness_in_x_pixels + 1) >> 1;
      int x_coord_min = (_width - brightness_in_x_pixels + 0) >> 1;
      int x_coord = _width;
      do
      {
        --x_coord;

        // (the check is already including "blanking" )
        if (x_coord >= x_coord_min && x_coord < x_coord_max)
        {
          row[ESP32_TX_FIFO_POSITION_ADJUST(x_coord)] &= BITMASK_OE_CLEAR;
        }
        else
        {
          row[ESP32_TX_FIFO_POSITION_ADJUST(x_coord)] |= BIT_OE; // Disable output after this point.
        }

      } while (x_coord);

    } while (colouridx);

  } while (row_idx);
}

// ################################################# END setBrightnessOE(uint8_t brt, const int _buff_id) ################################################# //
// ================================================= START begin(int r1, int g1, int b1, int r2, int g2, int b2, int a, int b, int c, int d, int e, int lat, int oe, int clk) ================================================= //
void begin(int r1, int g1, int b1, int r2, int g2, int b2, int a, int b, int c, int d, int e, int lat, int oe, int clk){

  // RGB
  m_cfg.gpio.r1 = r1;
  m_cfg.gpio.g1 = g1;
  m_cfg.gpio.b1 = b1;
  m_cfg.gpio.r2 = r2;
  m_cfg.gpio.g2 = g2;
  m_cfg.gpio.b2 = b2;

  // Line Select
  m_cfg.gpio.a = a;
  m_cfg.gpio.b = b;
  m_cfg.gpio.c = c;
  m_cfg.gpio.d = d;
  m_cfg.gpio.e = e;

  // Clock & Control
  m_cfg.gpio.lat = lat;
  m_cfg.gpio.oe = oe;
  m_cfg.gpio.clk = clk;

}
// ################################################# END begin(int r1, int g1, int b1, int r2, int g2, int b2, int a, int b, int c, int d, int e, int lat, int oe, int clk) ################################################# //

  inline void setCfg(const HUB75_I2S_CFG &cfg)
  {

    m_cfg = cfg;
    PIXELS_PER_ROW = m_cfg.mx_width * m_cfg.chain_length;
    ROWS_PER_FRAME = m_cfg.mx_height / MATRIX_ROWS_IN_PARALLEL;
    MASK_OFFSET = 16 - m_cfg.getPixelColorDepthBits();
  }

inline void drawPixel(int16_t x, int16_t y, uint16_t color) // adafruit virtual void override
{
  uint8_t r, g, b;
  int16_t w = 1, h = 1;
  transform(x, y, w, h);
  updateMatrixDMABuffer(x, y, r, g, b);
}

  void transform(int16_t &x, int16_t &y, int16_t &w, int16_t &h)
  {
#ifndef NO_GFX
    int16_t t;
    switch (rotation)
    {
    case 1:
      t = _height - 1 - y - (h - 1);
      y = x;
      x = t;
      t = h;
      h = w;
      w = t;
      return;
    case 2:
      x = _width - 1 - x - (w - 1);
      y = _height - 1 - y - (h - 1);
      return;
    case 3:
      t = y;
      y = _width - 1 - x - (w - 1);
      x = t;
      t = h;
      h = w;
      w = t;
      return;
    }
#endif
  };



  void IRAM_ATTR updateMatrixDMABuffer(uint16_t x_coord, uint16_t y_coord, uint8_t red, uint8_t green, uint8_t blue)
{


  /* 1) Check that the co-ordinates are within range, or it'll break everything big time.
   * Valid co-ordinates are from 0 to (MATRIX_XXXX-1)
   */
  if (x_coord >= PIXELS_PER_ROW || y_coord >= m_cfg.mx_height)
  {
    return;
  }

  /* LED Brightness Compensation. Because if we do a basic "red & mask" for example,
   * we'll NEVER send the dimmest possible colour, due to binary skew.
   * i.e. It's almost impossible for colour_depth_idx of 0 to be sent out to the MATRIX unless the 'value' of a colour is exactly '1'
   * https://ledshield.wordpress.com/2012/11/13/led-brightness-to-your-eye-gamma-correction-no/
   */
  uint16_t red16, green16, blue16;
#ifdef NO_CIE1931
  red16 	= red;
  green16 	= green;
  blue16 	= blue;
#else  
  red16 = lumConvTab[red];
  green16 = lumConvTab[green];
  blue16 = lumConvTab[blue];
#endif

  /* When using the drawPixel, we are obviously only changing the value of one x,y position,
   * however, the two-scan panels paint TWO lines at the same time
   * and this reflects the parallel in-DMA-memory data structure of uint16_t's that are getting
   * pumped out at high speed.
   *
   * So we need to ensure we persist the bits (8 of them) of the uint16_t for the row we aren't changing.
   *
   * The DMA buffer order has also been reversed (refer to the last code in this function)
   * so we have to check for this and check the correct position of the MATRIX_DATA_STORAGE_TYPE
   * data.
   */
  x_coord = ESP32_TX_FIFO_POSITION_ADJUST(x_coord);

  uint16_t _colourbitclear = BITMASK_RGB1_CLEAR, _colourbitoffset = 0;

  if (y_coord >= ROWS_PER_FRAME)
  { // if we are drawing to the bottom part of the panel
    _colourbitoffset = BITS_RGB2_OFFSET;
    _colourbitclear = BITMASK_RGB2_CLEAR;
    y_coord -= ROWS_PER_FRAME;
  }

  // Iterating through colour depth bits, which we assume are 8 bits per RGB subpixel (24bpp)
  uint8_t colour_depth_idx = m_cfg.getPixelColorDepthBits();
  do
  {
    --colour_depth_idx;

#ifdef NO_CIE1931
    uint16_t mask = colour_depth_idx;
#else	
    uint16_t mask = PIXEL_COLOR_MASK_BIT(colour_depth_idx, MASK_OFFSET);
#endif	
    uint16_t RGB_output_bits = 0;

    /* Per the .h file, the order of the output RGB bits is:
     * BIT_B2, BIT_G2, BIT_R2,    BIT_B1, BIT_G1, BIT_R1     */
    RGB_output_bits |= (bool)(blue16 & mask); // --B
    RGB_output_bits <<= 1;
    RGB_output_bits |= (bool)(green16 & mask); // -BG
    RGB_output_bits <<= 1;
    RGB_output_bits |= (bool)(red16 & mask); // BGR
    RGB_output_bits <<= _colourbitoffset;    // shift colour bits to the required position

    // Get the contents at this address,
    // it would represent a vector pointing to the full row of pixels for the specified colour depth bit at Y coordinate
    ESP32_I2S_DMA_STORAGE_TYPE *p = getRowDataPtr(y_coord, colour_depth_idx);

    // We need to update the correct uint16_t word in the rowBitStruct array pointing to a specific pixel at X - coordinate
    p[x_coord] &= _colourbitclear; // reset RGB bits
    p[x_coord] |= RGB_output_bits; // set new RGB bits

#if defined(SPIRAM_DMA_BUFFER)
    Cache_WriteBack_Addr((uint32_t)&p[x_coord], sizeof(ESP32_I2S_DMA_STORAGE_TYPE));
#endif

  } while (colour_depth_idx); // end of colour depth loop (8)
} // updateMatrixDMABuffer (specific co-ords change)

};