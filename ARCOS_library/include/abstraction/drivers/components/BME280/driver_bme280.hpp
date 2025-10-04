/*****************************************************************
 * File:      driver_bme280.hpp
 * Category:  abstraction/drivers/components/BME280
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    BME280 environmental sensor driver with auto-initialization
 *    and calibrated readings following ARCOS conventions.
 *    Header-only implementation for optional inclusion.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_BME280_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_BME280_HPP_

#include "abstraction/hal.hpp"

// Include platform-specific I2C HAL implementation
// ESP32S3_I2C is defined in the platform hal_connector.hpp (included via hal.hpp)

namespace arcos{
namespace abstraction{

/** BME280 sensor data structure */
struct BME280Data{
  float temperature;  // °C
  float humidity;     // %
  float pressure;     // Pa
};

/** BME280 configuration structure (optional)
 * 
 * Allows customization of sensor settings during initialization.
 * If not provided, sensible defaults are used automatically.
 */
struct BME280Config{
  uint8_t temp_oversampling;  // 0=skip, 1=x1, 2=x2, 3=x4, 4=x8, 5=x16
  uint8_t press_oversampling; // 0=skip, 1=x1, 2=x2, 3=x4, 4=x8, 5=x16
  uint8_t hum_oversampling;   // 0=skip, 1=x1, 2=x2, 3=x4, 4=x8, 5=x16
  uint8_t mode;               // 0=sleep, 1/2=forced, 3=normal
  
  // Default configuration: 1x oversampling, normal mode
  BME280Config()
    : temp_oversampling(1), press_oversampling(1),
      hum_oversampling(1), mode(3){}
};

/** BME280 environmental sensor driver */
class DRIVER_BME280{
private:
  static constexpr const char* TAG = "BME280";
  static constexpr uint8_t DEFAULT_ADDRESS = 0x76;
  
  // BME280 Registers
  static constexpr uint8_t REG_CHIP_ID = 0xD0;
  static constexpr uint8_t REG_CTRL_MEAS = 0xF4;
  static constexpr uint8_t REG_CTRL_HUM = 0xF2;
  static constexpr uint8_t REG_PRESS_MSB = 0xF7;
  static constexpr uint8_t REG_CALIB_00 = 0x88;
  static constexpr uint8_t REG_CALIB_26 = 0xE1;
  static constexpr uint8_t CHIP_ID = 0x60;

  /** Calibration coefficients from BME280 */
  struct CalibData{
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
  } calib_;

  uint8_t address_;
  uint8_t bus_id_;
  bool initialized_;

  /** Read calibration data from sensor
   * @return true if successful
   */
  bool readCalibrationData();
  
  /** Calculate calibrated values from raw sensor data
   * @param raw_temp Raw temperature value
   * @param raw_press Raw pressure value
   * @param raw_hum Raw humidity value
   * @return Calibrated sensor data
   */
  BME280Data calculateCalibratedValues(int32_t raw_temp, int32_t raw_press, int32_t raw_hum);

public:
  /** Constructor with default address and bus
   * @param address I2C address (default 0x76)
   * @param bus_id I2C bus ID (default 0)
   */
  DRIVER_BME280(uint8_t address = DEFAULT_ADDRESS, uint8_t bus_id = 0);

  /** Initialize the sensor with default configuration
   * Automatically reads calibration data and configures sensor
   * @return true if initialization successful
   */
  bool initialize();

  /** Initialize the sensor with custom configuration
   * Allows fine-grained control over sensor settings
   * @param config Custom configuration settings
   * @return true if initialization successful
   */
  bool initialize(const BME280Config& config);

  /** Check if sensor is initialized and responding
   * @return true if initialized
   */
  bool isInitialized() const{ return initialized_; }

  /** Read all sensor data in one call
   * @param data Reference to BME280Data structure to fill
   * @return true if read successful
   */
  bool readData(BME280Data& data);

  /** Read temperature only
   * @param temperature Reference to store temperature (°C)
   * @return true if read successful
   */
  bool readTemperature(float& temperature);
  
  /** Read humidity only
   * @param humidity Reference to store humidity (%)
   * @return true if read successful
   */
  bool readHumidity(float& humidity);
  
  /** Read pressure only
   * @param pressure Reference to store pressure (Pa)
   * @return true if read successful
   */
  bool readPressure(float& pressure);

  /** Check sensor connection
   * @return true if sensor responds with correct chip ID
   */
  bool isConnected();
};

} // namespace abstraction
} // namespace arcos

// Include inline implementation
#include "driver_bme280_impl.hpp"

#endif // ARCOS_ABSTRACTION_DRIVERS_BME280_HPP_