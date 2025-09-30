/*****************************************************************
 * File:      driver_icm20948.hpp
 * Category:  abstraction/drivers
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    ICM20948 9-axis IMU driver with proper magnetometer initialization
 *    and unified sensor data access following ARCOS conventions
 *****************************************************************/

#pragma once

#include "abstraction/hal.hpp"

namespace arcos {
namespace abstraction {

struct ICM20948Data {
    // Accelerometer (g)
    float accel_x, accel_y, accel_z;
    
    // Gyroscope (degrees/second)
    float gyro_x, gyro_y, gyro_z;
    
    // Magnetometer (μT)
    float mag_x, mag_y, mag_z;
};

class DRIVER_ICM20948 {
private:
    static const char* TAG;
    static constexpr uint8_t DEFAULT_ADDRESS = 0x68;
    
    // ICM20948 Registers (Bank 0)
    static constexpr uint8_t REG_WHO_AM_I = 0x00;
    static constexpr uint8_t REG_USER_CTRL = 0x03;      // User control register
    static constexpr uint8_t REG_PWR_MGMT_1 = 0x06;
    static constexpr uint8_t REG_PWR_MGMT_2 = 0x07;
    static constexpr uint8_t REG_ACCEL_XOUT_H = 0x2D;
    static constexpr uint8_t REG_GYRO_XOUT_H = 0x33;
    static constexpr uint8_t REG_EXT_SLV_SENS_DATA_00 = 0x3B;
    static constexpr uint8_t REG_BANK_SEL = 0x7F;
    
    // Bank 2 registers  
    static constexpr uint8_t REG_ACCEL_CONFIG = 0x14;   // Bank 2
    static constexpr uint8_t REG_GYRO_CONFIG_1 = 0x01;  // Bank 2
    
    // Bank 3 registers (I2C Master)
    static constexpr uint8_t REG_I2C_MST_CTRL = 0x01;
    static constexpr uint8_t REG_I2C_SLV0_ADDR = 0x03;
    static constexpr uint8_t REG_I2C_SLV0_REG = 0x04;
    static constexpr uint8_t REG_I2C_SLV0_CTRL = 0x05;
    static constexpr uint8_t REG_I2C_SLV0_DO = 0x06;
    
    // Magnetometer (AK09916) constants
    static constexpr uint8_t MAG_I2C_ADDR = 0x0C;
    static constexpr uint8_t MAG_REG_WIA2 = 0x01;
    static constexpr uint8_t MAG_REG_CNTL2 = 0x31;
    static constexpr uint8_t MAG_REG_CNTL3 = 0x32;
    static constexpr uint8_t MAG_REG_HXL = 0x11;
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

    bool SelectBank(uint8_t bank);
    bool InitializeAccelGyro();
    bool InitializeMagnetometer();

public:
    /**
     * Constructor with default address and bus
     */
    DRIVER_ICM20948(uint8_t address = DEFAULT_ADDRESS, uint8_t bus_id = 0);

    /**
     * Initialize the sensor with default configuration
     * Includes accelerometer, gyroscope, and magnetometer
     */
    bool Initialize();

    /**
     * Check if sensor is initialized and responding
     */
    bool IsInitialized() const { return initialized_; }

    /**
     * Check if magnetometer is working
     */
    bool IsMagnetometerInitialized() const { return mag_initialized_; }

    /**
     * Read all sensor data in one call
     */
    bool ReadData(ICM20948Data& data);

    /**
     * Read individual sensor groups
     */
    bool ReadAccelerometer(float& x, float& y, float& z);
    bool ReadGyroscope(float& x, float& y, float& z);
    bool ReadMagnetometer(float& x, float& y, float& z);

    /**
     * Check sensor connection
     */
    bool IsConnected();
};

} // namespace abstraction
} // namespace arcos