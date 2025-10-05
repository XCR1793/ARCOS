# ARCOS Component Drivers

This directory contains standalone sensor drivers for the ARCOS library. Each driver is self-contained and can be used independently after initializing the HAL.

## Available Drivers

### BME280 - Environmental Sensor
- **Measures:** Temperature, Humidity, Pressure
- **Interface:** I2C
- **Default Address:** 0x76 (alt: 0x77)
- **Files:** 
  - `BME280/driver_bme280.hpp` - Header with API
  - `BME280/driver_bme280_impl.hpp` - Implementation

### ICM20948 - 9-Axis IMU
- **Measures:** 3-axis accelerometer, gyroscope, magnetometer
- **Interface:** I2C
- **Default Address:** 0x68 (alt: 0x69)
- **Files:**
  - `ICM20948/driver_icm20948.hpp` - Header with API
  - `ICM20948/driver_icm20948_impl.hpp` - Implementation

## Quick Start

### 1. Include Required Headers

```cpp
#include "abstraction/hal.hpp"  // For ESP32S3_I2C
#include "abstraction/drivers/components/BME280/driver_bme280.hpp"
#include "abstraction/drivers/components/ICM20948/driver_icm20948.hpp"
```

### 2. Initialize I2C HAL

```cpp
using namespace arcos::abstraction;

// Initialize I2C bus 0 with SDA=9, SCL=10, 400kHz
ESP32S3_I2C::Initialize(0, 9, 10, 400000);
```

### 3. Create and Initialize Drivers

```cpp
// Create driver instances (address, bus_id)
DRIVER_BME280 bme280(0x76, 0);
DRIVER_ICM20948 icm20948(0x68, 0);

// Initialize with default settings
if(!bme280.initialize()) {
    // Handle error
}

if(!icm20948.initialize()) {
    // Handle error
}
```

### 4. Read Sensor Data

```cpp
// Read environmental data
BME280Data env;
if(bme280.readData(env)) {
    printf("Temp: %.1f°C, Humidity: %.1f%%, Pressure: %.0fPa\n",
           env.temperature, env.humidity, env.pressure);
}

// Read IMU data
ICM20948Data imu;
if(icm20948.readData(imu)) {
    printf("Accel: %.2f,%.2f,%.2f g\n", 
           imu.accel_x, imu.accel_y, imu.accel_z);
    printf("Gyro: %.2f,%.2f,%.2f dps\n",
           imu.gyro_x, imu.gyro_y, imu.gyro_z);
    printf("Mag: %.1f,%.1f,%.1f μT\n",
           imu.mag_x, imu.mag_y, imu.mag_z);
}
```

## Advanced Configuration

### BME280 Custom Configuration

```cpp
BME280Config config;
config.temp_oversampling = 5;   // 16x oversampling
config.press_oversampling = 5;  // 16x oversampling
config.hum_oversampling = 5;    // 16x oversampling
config.mode = 3;                // Normal mode

DRIVER_BME280 bme280(0x76, 0);
bme280.initialize(config);
```

### ICM20948 Custom Configuration

```cpp
ICM20948Config config;
config.accel_range = 3;              // ±16g
config.gyro_range = 3;               // ±2000 dps
config.enable_magnetometer = false;  // Disable mag

DRIVER_ICM20948 icm20948(0x68, 0);
icm20948.initialize(config);
```

## API Reference

### BME280 Driver

#### Constructor
```cpp
DRIVER_BME280(uint8_t address = 0x76, uint8_t bus_id = 0);
```

#### Methods
- `bool initialize()` - Initialize with defaults
- `bool initialize(const BME280Config& config)` - Initialize with custom config
- `bool readData(BME280Data& data)` - Read all sensor data
- `bool readTemperature(float& temp)` - Read temperature only
- `bool readHumidity(float& hum)` - Read humidity only
- `bool readPressure(float& press)` - Read pressure only
- `bool isInitialized() const` - Check if initialized
- `bool isConnected()` - Check if sensor responds

#### Data Structure
```cpp
struct BME280Data {
    float temperature;  // °C
    float humidity;     // %
    float pressure;     // Pa
};
```

### ICM20948 Driver

#### Constructor
```cpp
DRIVER_ICM20948(uint8_t address = 0x68, uint8_t bus_id = 0);
```

#### Methods
- `bool initialize()` - Initialize with defaults
- `bool initialize(const ICM20948Config& config)` - Initialize with custom config
- `bool readData(ICM20948Data& data)` - Read all sensor data
- `bool readAccelerometer(float& x, float& y, float& z)` - Read accel only
- `bool readGyroscope(float& x, float& y, float& z)` - Read gyro only
- `bool readMagnetometer(float& x, float& y, float& z)` - Read mag only
- `bool isInitialized() const` - Check if initialized
- `bool isMagnetometerInitialized() const` - Check if mag is working
- `bool isConnected()` - Check if sensor responds

#### Data Structure
```cpp
struct ICM20948Data {
    float accel_x, accel_y, accel_z;  // g
    float gyro_x, gyro_y, gyro_z;     // degrees/second
    float mag_x, mag_y, mag_z;        // μT
};
```

## Adding New Drivers

When creating a new driver:

1. **Use the same pattern**
   - Header file with class declaration
   - Implementation file with inline functions
   - Include `abstraction/hal.hpp`
   - Use `ESP32S3_I2C` for I2C operations

2. **Keep public API simple**
   - Constructor takes address and bus ID
   - `initialize()` with sensible defaults
   - `initialize(config)` for advanced usage
   - `readData()` returns structured data

3. **Hide complexity in private methods**
   - Calibration routines
   - Register configuration
   - Data parsing
   - Error handling

4. **Follow naming conventions**
   - `DRIVER_<SENSOR_NAME>` for class name
   - `<SENSOR_NAME>Data` for data structure
   - `<SENSOR_NAME>Config` for configuration

## Notes

- All drivers are header-only for easy integration
- Drivers use `inline` functions to avoid linking issues
- HAL must be initialized before using any driver
- Multiple sensors can share the same I2C bus
- Each sensor needs unique I2C address on the bus

## See Also

- `DRIVER_REFACTORING_GUIDE.md` - Detailed migration and architecture guide
- `HALAccelNGyro.cpp` - Complete working example
- Platform-specific HAL implementations in `abstraction/platforms/`
