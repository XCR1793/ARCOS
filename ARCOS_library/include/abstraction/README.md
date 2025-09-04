# Abstraction
### Functions and purpose
The abstraction folder holds all templates and virtual functions and publically exposed Hardware Abstraction Layer (HAL) function calls similar to those of the arduino library. Currently the HAL is taken care of by other libraries and the HAL system doesnt have to directly touch bare metal. The Core folder contains all the simple and base level abstraction such as gpio, whilst the Driver folder contains all protocals and specialised systems such as integrated sensors.

The Core and Drivers hold templates for the implementation files in 
```
abstraction/platforms/{platform}/{device}/
```
will use to drive the templates.

The main hal.hpp file holds the switch that will automatically detect and route the correct hal platform to the firmware during compile time. However the hal switch requires build-system to include flags.

Examples:

```
GCC:
  CXXFLAGS += -DTARGET_ESP32_Xtensa_Wroom32S3

Makefile:
  CXXFLAGS += -DTARGET_ESP32_Xtensa_Wroom32S3

PlatformIO:
  build_flags = -DTARGET_ESP32_Xtensa_Wroom32S3

CMake:
  add_compile_definitions(TARGET_ESP32_Xtensa_Wroom32S3)
```

All driver and core file implementations in each platform is then connected to a hal_connector.hpp file which each folder must have. This is to increase ease of scaling as the hal.hpp only has to include the hal_connector file which each platform's files connect to.