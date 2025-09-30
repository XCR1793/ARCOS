/*****************************************************************
 * File:      driver_bme280.hpp
 * Category:  abstraction/drivers
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    BME280 environmental sensor driver with auto-initialization
 *    and calibrated readings following ARCOS conventions
 *****************************************************************/

#pragma once

#include "abstraction/hal.hpp"

namespace arcos {
namespace abstraction {

struct BME280Data {
    float temperature;  // °C
    float humidity;     // %
    float pressure;     // Pa
};

class DRIVER_BME280 {
private:
    static const char* TAG;
    static constexpr uint8_t DEFAULT_ADDRESS = 0x76;
    
    // BME280 Registers
    static constexpr uint8_t REG_CHIP_ID = 0xD0;
    static constexpr uint8_t REG_CTRL_MEAS = 0xF4;
    static constexpr uint8_t REG_CTRL_HUM = 0xF2;
    static constexpr uint8_t REG_PRESS_MSB = 0xF7;
    static constexpr uint8_t REG_CALIB_00 = 0x88;
    static constexpr uint8_t REG_CALIB_26 = 0xE1;
    
    static constexpr uint8_t CHIP_ID = 0x60;

    // Calibration coefficients
    struct CalibData {
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

    bool ReadCalibrationData();
    BME280Data CalculateCalibratedValues(int32_t raw_temp, int32_t raw_press, int32_t raw_hum);

public:
    /**
     * Constructor with default address and bus
     */
    DRIVER_BME280(uint8_t address = DEFAULT_ADDRESS, uint8_t bus_id = 0);

    /**
     * Initialize the sensor with default configuration
     * Automatically reads calibration data
     */
    bool Initialize();

    /**
     * Check if sensor is initialized and responding
     */
    bool IsInitialized() const { return initialized_; }

    /**
     * Read all sensor data in one call
     */
    bool ReadData(BME280Data& data);

    /**
     * Read individual values
     */
    bool ReadTemperature(float& temperature);
    bool ReadHumidity(float& humidity);
    bool ReadPressure(float& pressure);

    /**
     * Check sensor connection
     */
    bool IsConnected();
};

} // namespace abstraction
} // namespace arcos