# Abstraction
### Functions and purpose
The abstraction folder holds all templates and virtual functions and publically exposed Hardware Abstraction Layer (HAL) function calls similar to those of the arduino library. Currently the HAL is taken care of by other libraries and the HAL system doesnt have to directly touch bare metal. The Core folder contains all the simple and base level abstraction such as gpio, whilst the Driver folder contains all protocal usage for specialised systems such as integrated sensors.

The Core hold templates for the implementation files in 
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

All core file implementations in each platform is then connected to a hal_connector.hpp file which each folder must have. This is to increase ease of scaling as the hal.hpp only has to include the hal_connector file which each platform's files connect to.

File define protections setup is as follows
```
Project Name: ARCOS
Folder Location: Abstraction
File Name: README.md
Combined = ARCOS_ABSTRACTION_README_MD_

Deeper File Example
Project Name: ARCOS
Folder Location: Abstraction/core
File Name = hal_gpio.hpp
Combined = ARCOS_ABSTRACTION_CORE_HAL_GPIO_HPP_
```

For commenting /** comment */ is used normalle except if its attached to the end of a line where // is used. The reason for this distinction is to show the difference in comments where // is used to explain singular lines that are inline whilst /** this */ is used for everything else. Essentially // is quick explaination.

***
## Core
### HAL core features implemented
- **hal_gpio_digital.hpp** : Digital reads and digital writes for both compile time and run time setting and usage. This also includes setting pins as inputs / outputs, internally pulling pins up or down and the fast lower register level implementations of each.

### HAL core features partially implemented (Needs Revision)
- **hal_gpio_pwm.hpp** : PWM outputs for any pwm capable pins, allowing the ability to change its frequency and duty cycle live in both compile time and run time depending on the needs of the project.
- **hal_protocal_i2c.hpp** : I2C protocal interface for writing and reading bytes along with initialising any i2c or i2c capable pins to start the protocal at any speed the user sets.
- **hal_system_timer.hpp** : Timing based systems like system delay(milliseconds), delay microseconds, millis and micros.

### HAL core features to add
- **hal_protocal_spi.hpp**
- **hal_protocal_uart.hpp**
- **hal_protocal_i2s.hpp**
- **hal_gpio_analog**
- **hal_protocal_single_wire.hpp**
- **hal_protocal_wifi.hpp**
- **hal_protocal_bluetooth.hpp**

## Drivers
### HAL driver features to add
- **BME280**
- **ICM20948**
- **HUB75**
- **SPI SD Card**
- **WS2812B**