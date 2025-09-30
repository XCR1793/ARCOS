/*****************************************************************
 * File:      driver_hub75.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Signal hub75 protocal driver for led matrix displays.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_DRIVER_HUB75_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_DRIVER_HUB75_HPP_

#include <cstdint>

namespace arcos::abstraction{
  namespace hub75{
    struct Config{
      public:
        int DisplayHeight = 32;
        int DisplayWidth = 64;
        uint8_t Buffers = 5;
      
      private:
        int internalDisplayArrayLength_ = DisplayHeight * DisplayWidth;

    };

    enum struct Status{
      Failed = 0,
      Suceeded = 1,
      Warnings = 2
    };
  }

  class DisplayHUB75{
    public:
      // DisplayHUB75(const hub75::Config& cfg) : config_(cfg){}
      DisplayHUB75();

      hub75::Status Initialisation();

      hub75::Status UpdateDisplayConfig(const hub75::Config& cfg);

      // template <typename BufferBitResolution>
      // hub75::Status UpdateFrameBuffer(const BufferBitResolution*);

      void TestScanDisplay( int Height = 32, int Width = 64, int scan = 16,
                            int A = 0, int B = 0, int C = 0, int D = 0, int E = 0,
                            int R0 = 0, int G0 = 0, int B0 = 0,
                            int R1 = 0, int G1 = 0, int B1 = 0,
                            int Lat = 0, int OE = 0, int CLK = 0);

    private:
      hub75::Config config_;

  };
}

#endif // ARCOS_ABSTRACTION_DRIVERS_DRIVER_HUB75_HPP_