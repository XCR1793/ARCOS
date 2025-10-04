/*****************************************************************
 * File:      driver_bme280_impl.hpp
 * Category:  abstraction/drivers/components/BME280
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    BME280 environmental sensor driver inline implementation.
 *    Automatically included by driver_bme280.hpp.
 *    Uses only cross-platform ARCOS abstractions.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_BME280_IMPL_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_BME280_IMPL_HPP_

namespace arcos{
namespace abstraction{

inline DRIVER_BME280::DRIVER_BME280(uint8_t address, uint8_t bus_id)
  : address_(address),
    bus_id_(bus_id),
    initialized_(false){
}

inline bool DRIVER_BME280::initialize(){
  // Use default configuration
  BME280Config default_config;
  return initialize(default_config);
}

inline bool DRIVER_BME280::initialize(const BME280Config& config){
  // Check chip ID
  uint8_t chip_id = 0;
  if(ESP32S3_I2C::ReadRegister(bus_id_, address_, REG_CHIP_ID, &chip_id) != HalResult::Success){
    return false;
  }
  
  if(chip_id != CHIP_ID){
    return false;
  }
  
  // Soft reset
  uint8_t reset_cmd = 0xB6;
  ESP32S3_I2C::WriteRegister(bus_id_, address_, 0xE0, reset_cmd);
  HAL_TIMER_DEFAULT::Delay(10);
  
  // Read calibration data
  if(!readCalibrationData()){
    return false;
  }
  
  // Configure sensor with provided settings
  // Humidity oversampling (must be written first)
  uint8_t ctrl_hum = config.hum_oversampling & 0x07;
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_CTRL_HUM, ctrl_hum);
  
  // Temperature and pressure oversampling + mode
  uint8_t ctrl_meas = ((config.temp_oversampling & 0x07) << 5) | 
                      ((config.press_oversampling & 0x07) << 2) | 
                      (config.mode & 0x03);
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_CTRL_MEAS, ctrl_meas);
  
  HAL_TIMER_DEFAULT::Delay(100);
  
  initialized_ = true;
  return true;
}

inline bool DRIVER_BME280::readCalibrationData(){
  uint8_t calib_data[26];
  
  // Read temperature and pressure calibration (0x88-0xA1)
  if(ESP32S3_I2C::ReadRegisterBuffer(bus_id_, address_, REG_CALIB_00, calib_data, 26) != HalResult::Success){
    return false;
  }
  
  calib_.dig_T1 = (calib_data[1] << 8) | calib_data[0];
  calib_.dig_T2 = (calib_data[3] << 8) | calib_data[2];
  calib_.dig_T3 = (calib_data[5] << 8) | calib_data[4];
  calib_.dig_P1 = (calib_data[7] << 8) | calib_data[6];
  calib_.dig_P2 = (calib_data[9] << 8) | calib_data[8];
  calib_.dig_P3 = (calib_data[11] << 8) | calib_data[10];
  calib_.dig_P4 = (calib_data[13] << 8) | calib_data[12];
  calib_.dig_P5 = (calib_data[15] << 8) | calib_data[14];
  calib_.dig_P6 = (calib_data[17] << 8) | calib_data[16];
  calib_.dig_P7 = (calib_data[19] << 8) | calib_data[18];
  calib_.dig_P8 = (calib_data[21] << 8) | calib_data[20];
  calib_.dig_P9 = (calib_data[23] << 8) | calib_data[22];
  calib_.dig_H1 = calib_data[25];
  
  // Read humidity calibration (0xE1-0xE7)
  uint8_t hum_calib[7];
  if(ESP32S3_I2C::ReadRegisterBuffer(bus_id_, address_, REG_CALIB_26, hum_calib, 7) != HalResult::Success){
    return false;
  }
  
  calib_.dig_H2 = (hum_calib[1] << 8) | hum_calib[0];
  calib_.dig_H3 = hum_calib[2];
  calib_.dig_H4 = (hum_calib[3] << 4) | (hum_calib[4] & 0x0F);
  calib_.dig_H5 = (hum_calib[5] << 4) | (hum_calib[4] >> 4);
  calib_.dig_H6 = hum_calib[6];
  
  return true;
}

inline bool DRIVER_BME280::readData(BME280Data& data){
  if(!initialized_){
    return false;
  }
  
  // Read all sensor data (0xF7-0xFE)
  uint8_t raw_data[8];
  if(ESP32S3_I2C::ReadRegisterBuffer(bus_id_, address_, REG_PRESS_MSB, raw_data, 8) != HalResult::Success){
    return false;
  }
  
  // Parse raw values
  int32_t raw_press = ((int32_t)raw_data[0] << 12) | ((int32_t)raw_data[1] << 4) | ((int32_t)raw_data[2] >> 4);
  int32_t raw_temp = ((int32_t)raw_data[3] << 12) | ((int32_t)raw_data[4] << 4) | ((int32_t)raw_data[5] >> 4);
  int32_t raw_hum = ((int32_t)raw_data[6] << 8) | (int32_t)raw_data[7];
  
  // Calculate calibrated values
  data = calculateCalibratedValues(raw_temp, raw_press, raw_hum);
  return true;
}

inline BME280Data DRIVER_BME280::calculateCalibratedValues(int32_t raw_temp, int32_t raw_press, int32_t raw_hum){
  BME280Data data;
  
  // Temperature compensation (from BME280 datasheet)
  int32_t var1, var2, t_fine;
  var1 = ((((raw_temp >> 3) - ((int32_t)calib_.dig_T1 << 1))) * ((int32_t)calib_.dig_T2)) >> 11;
  var2 = (((((raw_temp >> 4) - ((int32_t)calib_.dig_T1)) * ((raw_temp >> 4) - ((int32_t)calib_.dig_T1))) >> 12) * ((int32_t)calib_.dig_T3)) >> 14;
  t_fine = var1 + var2;
  data.temperature = ((t_fine * 5 + 128) >> 8) / 100.0f;
  
  // Pressure compensation
  int64_t var1_p, var2_p, p;
  var1_p = ((int64_t)t_fine) - 128000;
  var2_p = var1_p * var1_p * (int64_t)calib_.dig_P6;
  var2_p = var2_p + ((var1_p * (int64_t)calib_.dig_P5) << 17);
  var2_p = var2_p + (((int64_t)calib_.dig_P4) << 35);
  var1_p = ((var1_p * var1_p * (int64_t)calib_.dig_P3) >> 8) + ((var1_p * (int64_t)calib_.dig_P2) << 12);
  var1_p = (((((int64_t)1) << 47) + var1_p)) * ((int64_t)calib_.dig_P1) >> 33;
  
  if(var1_p == 0){
    data.pressure = 0;
  }else{
    p = 1048576 - raw_press;
    p = (((p << 31) - var2_p) * 3125) / var1_p;
    var1_p = (((int64_t)calib_.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2_p = (((int64_t)calib_.dig_P8) * p) >> 19;
    p = ((p + var1_p + var2_p) >> 8) + (((int64_t)calib_.dig_P7) << 4);
    data.pressure = (float)p / 256.0f;
  }
  
  // Humidity compensation
  int32_t v_x1_u32r;
  v_x1_u32r = (t_fine - ((int32_t)76800));
  v_x1_u32r = (((((raw_hum << 14) - (((int32_t)calib_.dig_H4) << 20) - (((int32_t)calib_.dig_H5) * v_x1_u32r)) +
                 ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)calib_.dig_H6)) >> 10) *
                 (((v_x1_u32r * ((int32_t)calib_.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) *
                 ((int32_t)calib_.dig_H2) + 8192) >> 14));
  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)calib_.dig_H1)) >> 4));
  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
  data.humidity = (float)(v_x1_u32r >> 12) / 1024.0f;
  
  return data;
}

inline bool DRIVER_BME280::readTemperature(float& temperature){
  BME280Data data;
  if(!readData(data)){
    return false;
  }
  temperature = data.temperature;
  return true;
}

inline bool DRIVER_BME280::readHumidity(float& humidity){
  BME280Data data;
  if(!readData(data)){
    return false;
  }
  humidity = data.humidity;
  return true;
}

inline bool DRIVER_BME280::readPressure(float& pressure){
  BME280Data data;
  if(!readData(data)){
    return false;
  }
  pressure = data.pressure;
  return true;
}

inline bool DRIVER_BME280::isConnected(){
  uint8_t chip_id = 0;
  return ESP32S3_I2C::ReadRegister(bus_id_, address_, REG_CHIP_ID, &chip_id) == HalResult::Success 
         && chip_id == CHIP_ID;
}

} // namespace abstraction
} // namespace arcos

#endif // ARCOS_ABSTRACTION_DRIVERS_BME280_IMPL_HPP_
