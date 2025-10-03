/*****************************************************************
 * File:      driver_icm20948.hpp
 * Category:  abstraction/drivers/components/ICM20948
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    ICM20948 9-axis IMU driver with proper magnetometer initialization
 *    and unified sensor data access following ARCOS conventions.
 *    Header-only implementation for optional inclusion.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_ICM20948_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_ICM20948_HPP_

#include "abstraction/hal.hpp"

namespace arcos{
namespace abstraction{

/** ICM20948 sensor data structure */
struct ICM20948Data{
  // Accelerometer (g)
  float accel_x, accel_y, accel_z;
  
  // Gyroscope (degrees/second)
  float gyro_x, gyro_y, gyro_z;
  
  // Magnetometer (μT)
  float mag_x, mag_y, mag_z;
};

/** ICM20948 9-axis IMU driver */
class DRIVER_ICM20948{
private:
  static constexpr const char* TAG = "ICM20948";
  static constexpr uint8_t DEFAULT_ADDRESS = 0x68;
  
  // ICM20948 Registers (Bank 0)
  static constexpr uint8_t REG_WHO_AM_I = 0x00;
  static constexpr uint8_t REG_USER_CTRL = 0x6A;
  static constexpr uint8_t REG_PWR_MGMT_1 = 0x06;
  static constexpr uint8_t REG_PWR_MGMT_2 = 0x07;
  static constexpr uint8_t REG_ACCEL_XOUT_H = 0x2D;
  static constexpr uint8_t REG_GYRO_XOUT_H = 0x33;
  static constexpr uint8_t REG_EXT_SLV_SENS_DATA_00 = 0x49;
  static constexpr uint8_t REG_BANK_SEL = 0x7F;
  
  // Bank 2 registers
  static constexpr uint8_t REG_ACCEL_CONFIG = 0x14;
  static constexpr uint8_t REG_GYRO_CONFIG_1 = 0x01;
  static constexpr uint8_t REG_ODR_ALIGN_EN = 0x09;
  
  // Bank 3 registers (I2C Master)
  static constexpr uint8_t REG_I2C_MST_CTRL = 0x24;
  static constexpr uint8_t REG_I2C_SLV0_ADDR = 0x25;
  static constexpr uint8_t REG_I2C_SLV0_REG = 0x26;
  static constexpr uint8_t REG_I2C_SLV0_CTRL = 0x27;
  static constexpr uint8_t REG_I2C_SLV0_DO = 0x06;
  static constexpr uint8_t REG_I2C_SLV4_ADDR = 0x13;
  static constexpr uint8_t REG_I2C_SLV4_REG = 0x14;
  static constexpr uint8_t REG_I2C_SLV4_CTRL = 0x15;
  static constexpr uint8_t REG_I2C_SLV4_DO = 0x16;
  static constexpr uint8_t REG_I2C_SLV4_DI = 0x17;
  
  // Magnetometer (AK09916) constants
  static constexpr uint8_t MAG_I2C_ADDR = 0x0C;
  static constexpr uint8_t MAG_REG_WIA2 = 0x01;
  static constexpr uint8_t MAG_REG_CNTL2 = 0x31;
  static constexpr uint8_t MAG_REG_CNTL3 = 0x32;
  static constexpr uint8_t MAG_REG_HXL = 0x11;
  static constexpr uint8_t MAG_REG_ST1 = 0x10;
  static constexpr uint8_t MAG_REG_ST2 = 0x18;
  static constexpr uint8_t MAG_CHIP_ID = 0x09;
  
  static constexpr uint8_t CHIP_ID = 0xEA;

  uint8_t address_;
  uint8_t bus_id_;
  bool initialized_;
  bool mag_initialized_;
  
  // Scale factors
  float accel_scale_;  // g per LSB
  float gyro_scale_;   // dps per LSB
  static constexpr float MAG_SCALE = 0.15f;  // μT per LSB for AK09916

  /** Select register bank
   * @param bank Bank number (0-3)
   * @return true if successful
   */
  bool selectBank(uint8_t bank);
  
  /** Initialize accelerometer and gyroscope
   * @return true if successful
   */
  bool initializeAccelGyro();
  
  /** Initialize magnetometer via I2C master
   * @return true if successful
   */
  bool initializeMagnetometer();

public:
  /** Constructor with default address and bus
   * @param address I2C address (default 0x68)
   * @param bus_id I2C bus ID (default 0)
   */
  DRIVER_ICM20948(uint8_t address = DEFAULT_ADDRESS, uint8_t bus_id = 0);

  /** Initialize the sensor with default configuration
   * Includes accelerometer, gyroscope, and magnetometer
   * @return true if initialization successful
   */
  bool initialize();

  /** Check if sensor is initialized and responding
   * @return true if initialized
   */
  bool isInitialized() const{ return initialized_; }

  /** Check if magnetometer is working
   * @return true if magnetometer initialized
   */
  bool isMagnetometerInitialized() const{ return mag_initialized_; }

  /** Read all sensor data in one call
   * @param data Reference to ICM20948Data structure to fill
   * @return true if read successful
   */
  bool readData(ICM20948Data& data);

  /** Read accelerometer data
   * @param x Reference to store X-axis acceleration (g)
   * @param y Reference to store Y-axis acceleration (g)
   * @param z Reference to store Z-axis acceleration (g)
   * @return true if read successful
   */
  bool readAccelerometer(float& x, float& y, float& z);
  
  /** Read gyroscope data
   * @param x Reference to store X-axis rotation (dps)
   * @param y Reference to store Y-axis rotation (dps)
   * @param z Reference to store Z-axis rotation (dps)
   * @return true if read successful
   */
  bool readGyroscope(float& x, float& y, float& z);
  
  /** Read magnetometer data
   * @param x Reference to store X-axis magnetic field (μT)
   * @param y Reference to store Y-axis magnetic field (μT)
   * @param z Reference to store Z-axis magnetic field (μT)
   * @return true if read successful
   */
  bool readMagnetometer(float& x, float& y, float& z);

  /** Check sensor connection
   * @return true if sensor responds with correct chip ID
   */
  bool isConnected();
};

} // namespace abstraction
} // namespace arcos

// Include inline implementation
#include "driver_icm20948_impl.hpp"

#endif // ARCOS_ABSTRACTION_DRIVERS_ICM20948_HPP_