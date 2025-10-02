#pragma once

#include <stdint.h>
#include <cstddef>          // For size_t
#include "platform_hal.hpp" // Platform abstraction layer

/** Forward declarations of abstract interfaces */
class IParallelHardware;
class IDmaBufferManager;

/** Compile-time gamma correction table (gamma = 2.2) */
constexpr uint8_t GAMMA_TABLE_22[32] = {
  0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 6,
  7, 8, 10, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 31, 31
};

/** Alternative gamma values for different visual preferences */
constexpr uint8_t GAMMA_TABLE_18[32] = {
  0, 0, 0, 0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 10, 11,
  13, 14, 16, 18, 20, 22, 24, 26, 28, 29, 31, 31, 31, 31, 31, 31
};

constexpr uint8_t GAMMA_TABLE_26[32] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4,
  5, 6, 7, 9, 10, 12, 14, 16, 18, 20, 23, 25, 27, 29, 31, 31
};

/** RGB pixel structure for public API */
struct RGB {
  uint8_t r, g, b;
  RGB() : r(0), g(0), b(0) {}
  RGB(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
};

/** Framebuffer structure for bulk operations */
struct FrameBuffer {
  enum Format {
    RGB888 = 0    /**< 24-bit RGB format */
  };
  
  RGB* pixels;
  int width;
  int height;
  size_t size_bytes;
  Format format;
  
  FrameBuffer() : pixels(nullptr), width(0), height(0), size_bytes(0), format(RGB888) {}
};

/** HUB75 display driver configuration */
struct HUB75Config {
  /** Display dimensions */
  int matrix_width = 64;
  int matrix_height = 32;
  
  /** Panel expansion modes */
  enum class ExpansionMode {
    SINGLE,           // Single panel (64x32)
    PARALLEL_OE,      // Multiple panels via separate OE pins (parallel addressing)
    SERIES_CHAIN      // Multiple panels daisy-chained (series data flow)
  };
  
  ExpansionMode expansion_mode = ExpansionMode::SINGLE;
  int panel_count = 1;           // Number of panels (1-4 typical)
  
  /** Panel inversion settings (per-panel flip control) */
  struct PanelInversion {
    bool flip_horizontal = false;  // Flip panel horizontally (mirror left-right)
    bool flip_vertical = false;    // Flip panel vertically (mirror top-bottom)
  };
  PanelInversion panel_inversions[4];  // Up to 4 panels supported
  
  /** Legacy dual display settings (deprecated, use expansion_mode) */
  bool dual_display_mode = false;   // Enable dual display spillover
  int effective_width = 64;         // Effective width (128 for dual mode)
  
  /** Colour and rendering settings */
  int colour_depth = 5;              // Bit depth per colour channel (1-8)
  bool enable_gamma_correction = true;
  float gamma_value = 2.2f;         // Gamma correction value
  bool enable_anti_aliasing = false; // Enable 2x2 supersampling
  
  /** Buffer configuration */
  bool enable_double_buffering = true;
  int colour_buffer_count = 5;       // Number of colour planes for BCM
  
  /** Hardware settings */
  int clock_freq_hz = 10000000;     // 10MHz default
  
  /** GPIO pin mappings for HUB75 protocol */
  struct PinMapping {
    PinNumber r0_pin = 7;   // Upper half red
    PinNumber g0_pin = 15;  // Upper half green  
    PinNumber b0_pin = 16;  // Upper half blue
    PinNumber r1_pin = 17;  // Lower half red
    PinNumber g1_pin = 18;  // Lower half green
    PinNumber b1_pin = 8;   // Lower half blue
    PinNumber lat_pin = 36; // Latch signal
    PinNumber oe_pin = 35;  // Output enable (primary)
    PinNumber oe_pin2 = PIN_NC; // Output enable (secondary, PIN_NC = disabled)
    PinNumber a_pin = 41;   // Row address A
    PinNumber b_pin = 40;   // Row address B
    PinNumber c_pin = 39;   // Row address C
    PinNumber d_pin = 38;   // Row address D
    PinNumber e_pin = 42;   // Row address E (for 64-row panels)
    PinNumber clock_pin = 37; // Clock signal
  } pins;
  
  /** Advanced timing settings */
  struct TimingConfig {
    int latch_blanking = 1;    // Blanking time during latch
    int output_blanking = 1;   // Output enable blanking time
    bool continuous_mode = true;
  } timing;
  
  /** Get default configuration for 64x32 matrix */
  static HUB75Config getDefault() {
    HUB75Config config;
    // Matrix settings use struct defaults, just override what's needed
    config.enable_gamma_correction = false;
    config.enable_double_buffering = true;
    
    return config;
  }
};

/** HUB75 LED matrix driver */
class HUB75Driver {
public:
  HUB75Driver();
  ~HUB75Driver();
  
  /** Initialize the driver with configuration (uses default LCD_CAM backend) */
  bool init(const HUB75Config& config = HUB75Config{});
  
  /** Initialize the driver with custom hardware and buffer backends (dependency injection) */
  bool init(const HUB75Config& config, IParallelHardware* hardware, IDmaBufferManager* buffer_manager);
  
  /** Start continuous display transmission */
  bool start();
  
  /** Stop display transmission */
  void stop();
  
  /** Check if driver is initialized */
  bool isInitialized() const { return initialized; }
  
  /** Check if transmission is running */
  bool isRunning() const { return running; }
  
  /** Set a single pixel colour (x, y coordinates) */
  void setPixel(int x, int y, const RGB& colour);
  
  /** Get pixel colour at coordinates */
  RGB getPixel(int x, int y) const;
  
  /** Clear all pixels to black */
  void clear();
  
  /** Fill entire display with colour */
  void fill(const RGB& colour);
  
  /** Update the display with current frame buffer */
  void show();
  
  /** Advanced framebuffer operations */
  FrameBuffer getFrameBuffer() const;
  bool setFrameBuffer(const FrameBuffer& buffer);
  bool uploadFrameBuffer(const RGB* pixels, int width, int height);
  void copyFrameBuffer(RGB* destination) const;
  
  /** Configuration access */
  const HUB75Config& getConfig() const { return config; }
  bool updateConfig(const HUB75Config& newConfig);
  
  /** Get display dimensions */
  int getWidth() const { return config.dual_display_mode ? config.effective_width : config.matrix_width; }
  int getHeight() const { return config.matrix_height; }
  
  /** BCM brightness control (0-255, scales display duration, not pixel values) */
  void setBrightness(uint8_t brightness);
  uint8_t getBrightness() const { return bcm_brightness; }
  
  /** Gamma correction controls */
  void setGammaCorrection(bool enabled, float gamma = 2.2f);
  bool isGammaCorrectionEnabled() const { return config.enable_gamma_correction; }

private:
  /** Internal RGB pixel structure */
  struct RGBPixel {
    uint8_t r, g, b;
  };
  
  /** Hardware interfaces (abstraction layer) */
  IParallelHardware* hwInterface;      // Abstract hardware interface (LCD_CAM, I2S, etc.)
  IDmaBufferManager* bufferManager;    // Abstract buffer manager
  bool owns_hardware;                  // Whether we own the hardware interface
  bool owns_buffer_manager;            // Whether we own the buffer manager
  
  /** Default implementations (opaque pointers - concrete types only in .cpp) */
  void* default_hw_impl;               // Opaque pointer to default hardware implementation
  void* default_buffer_impl;           // Opaque pointer to default buffer implementation
  
  /** Buffer pointers */
  uint16_t* frontBuffer;
  uint16_t* backBuffer;
  
  /** Dual OE pin support */
  PinNumber oe_pin2;
  
  /** Platform HAL reference */
  IPlatformHAL* platform;
  
  /** Configuration and state */
  HUB75Config config;
  bool initialized;
  bool running;
  
  /** Frame buffer */
  RGBPixel* framebuffer;
  
  /** Buffer management */
  int buffer_size;
  int base_buffer_size;
  
  /** BCM brightness control (0-255, affects display duration) */
  uint8_t bcm_brightness;
  
  /** Gamma correction tables - optimised for 5-bit colour depth */
  uint8_t gamma_table[32];
  
  /** Internal methods */
  bool swapBuffers();
  void convertFramebufferToHUB75();
  uint8_t convert8to5(uint8_t value);
  uint8_t getBitFromValue(uint8_t value5bit, int bit_plane);
  bool isValidCoordinate(int x, int y) const;
  
  /** Internal helper methods */
  void initializeLUT();
  void updateGammaTable(float gamma);
  bool validateConfig(const HUB75Config& cfg) const;
  void applyConfig(const HUB75Config& cfg);
  bool isValidBufferSize(int width, int height) const;
  void synchronizeOEPins();
};