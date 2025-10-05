/*****************************************************************
 * File:      driver_icm20948_impl.hpp
 * Category:  abstraction/drivers/components/ICM20948
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    ICM20948 9-axis IMU driver inline implementation.
 *    Automatically included by driver_icm20948.hpp.
 *    Uses only cross-platform ARCOS abstractions.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_ICM20948_IMPL_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_ICM20948_IMPL_HPP_

namespace arcos{
namespace abstraction{

inline DRIVER_ICM20948::DRIVER_ICM20948(uint8_t address, uint8_t bus_id)
  : address_(address),
    bus_id_(bus_id),
    initialized_(false),
    mag_initialized_(false),
    accel_scale_(0.0f),
    gyro_scale_(0.0f){
}

inline bool DRIVER_ICM20948::selectBank(uint8_t bank){
  uint8_t bank_sel = (bank << 4) & 0x30;
  return ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_BANK_SEL, bank_sel) == HalResult::Success;
}

inline bool DRIVER_ICM20948::initialize(){
  // Use default configuration
  ICM20948Config default_config;
  return initialize(default_config);
}

inline bool DRIVER_ICM20948::initialize(const ICM20948Config& config){
  // Select bank 0
  if(!selectBank(0)){
    return false;
  }
  
  // Check WHO_AM_I
  uint8_t who_am_i = 0;
  if(ESP32S3_I2C::ReadRegister(bus_id_, address_, REG_WHO_AM_I, &who_am_i) != HalResult::Success){
    return false;
  }
  
  if(who_am_i != CHIP_ID){
    return false;
  }
  
  // Reset device
  selectBank(0);
  uint8_t reset = 0x80;
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_PWR_MGMT_1, reset);
  HAL_TIMER_DEFAULT::Delay(100);
  
  // Wake up device
  uint8_t pwr_mgmt = 0x01;
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_PWR_MGMT_1, pwr_mgmt);
  HAL_TIMER_DEFAULT::Delay(10);
  
  // Enable accel and gyro
  uint8_t pwr_mgmt_2 = 0x00;
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_PWR_MGMT_2, pwr_mgmt_2);
  HAL_TIMER_DEFAULT::Delay(10);
  
  // Configure accelerometer with custom range
  selectBank(2);
  uint8_t accel_config = (config.accel_range & 0x03) << 1 | 0x01;  // Range + DLPF
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_ACCEL_CONFIG, accel_config);
  
  // Set accel scale based on range
  float accel_ranges[] = {2.0f, 4.0f, 8.0f, 16.0f};
  accel_scale_ = accel_ranges[config.accel_range & 0x03] / 32768.0f;
  
  // Configure gyroscope with custom range
  uint8_t gyro_config = (config.gyro_range & 0x03) << 1 | 0x01;  // Range + DLPF
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_GYRO_CONFIG_1, gyro_config);
  
  // Set gyro scale based on range
  float gyro_ranges[] = {250.0f, 500.0f, 1000.0f, 2000.0f};
  gyro_scale_ = gyro_ranges[config.gyro_range & 0x03] / 32768.0f;
  
  selectBank(0);
  
  // Initialize magnetometer if requested (best effort)
  if(config.enable_magnetometer){
    mag_initialized_ = initializeMagnetometer();
  }else{
    mag_initialized_ = false;
  }
  
  initialized_ = true;
  return true;
}

inline bool DRIVER_ICM20948::initializeMagnetometer(){
  static constexpr uint8_t I2C_SLVX_EN = 0x80;
  static constexpr uint8_t I2C_MST_RST = 0x02;
  static constexpr uint16_t AK09916_WHO_AM_I_1 = 0x4809;
  static constexpr uint16_t AK09916_WHO_AM_I_2 = 0x0948;
  
  // Enable ODR alignment (Bank 2)
  selectBank(2);
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_ODR_ALIGN_EN, 0x01);
  
  // Try up to 10 times to initialize magnetometer
  bool init_success = false;
  for(uint8_t tries = 0; tries < 10 && !init_success; tries++){
    HAL_TIMER_DEFAULT::Delay(10);
    
    // Enable I2C master mode (Bank 0)
    selectBank(0);
    ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_USER_CTRL, 0x20);
    HAL_TIMER_DEFAULT::Delay(10);
    
    // Configure I2C master (Bank 3)
    selectBank(3);
    ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_I2C_MST_CTRL, 0x07);
    HAL_TIMER_DEFAULT::Delay(10);
    
    // Read magnetometer WHO_AM_I via SLV4
    selectBank(3);
    
    // Read WIA_1 (0x00)
    ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_I2C_SLV4_ADDR, MAG_I2C_ADDR | 0x80);
    ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_I2C_SLV4_REG, 0x00);
    ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_I2C_SLV4_CTRL, I2C_SLVX_EN);
    
    // Wait for transaction to complete
    HAL_TIMER_DEFAULT::Delay(10);
    uint8_t wia1 = 0;
    ESP32S3_I2C::ReadRegister(bus_id_, address_, REG_I2C_SLV4_DI, &wia1);
    
    // Read WIA_2 (0x01)
    ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_I2C_SLV4_REG, 0x01);
    ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_I2C_SLV4_CTRL, I2C_SLVX_EN);
    HAL_TIMER_DEFAULT::Delay(10);
    uint8_t wia2 = 0;
    ESP32S3_I2C::ReadRegister(bus_id_, address_, REG_I2C_SLV4_DI, &wia2);
    
    uint16_t who_am_i = (wia1 << 8) | wia2;
    
    if(who_am_i == AK09916_WHO_AM_I_1 || who_am_i == AK09916_WHO_AM_I_2){
      init_success = true;
    }else{
      // Reset I2C master and try again
      selectBank(0);
      uint8_t user_ctrl = 0;
      ESP32S3_I2C::ReadRegister(bus_id_, address_, REG_USER_CTRL, &user_ctrl);
      user_ctrl |= I2C_MST_RST;
      ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_USER_CTRL, user_ctrl);
      HAL_TIMER_DEFAULT::Delay(10);
    }
  }
  
  if(!init_success){
    return false;
  }
  
  // Reset magnetometer (CNTL3 = 0x01)
  selectBank(3);
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_I2C_SLV4_ADDR, MAG_I2C_ADDR);
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_I2C_SLV4_REG, MAG_REG_CNTL3);
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_I2C_SLV4_DO, 0x01);
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_I2C_SLV4_CTRL, I2C_SLVX_EN);
  HAL_TIMER_DEFAULT::Delay(100);
  
  // Set magnetometer to continuous mode 100Hz (CNTL2 = 0x08)
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_I2C_SLV4_REG, MAG_REG_CNTL2);
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_I2C_SLV4_DO, 0x08);
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_I2C_SLV4_CTRL, I2C_SLVX_EN);
  HAL_TIMER_DEFAULT::Delay(10);
  
  // Configure slave 0 to continuously read 8 bytes from magnetometer
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_I2C_SLV0_ADDR, MAG_I2C_ADDR | 0x80);
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_I2C_SLV0_REG, MAG_REG_HXL);
  ESP32S3_I2C::WriteRegister(bus_id_, address_, REG_I2C_SLV0_CTRL, I2C_SLVX_EN | 0x08);
  HAL_TIMER_DEFAULT::Delay(10);
  
  selectBank(0);
  return true;
}

inline bool DRIVER_ICM20948::readData(ICM20948Data& data){
  if(!initialized_){
    return false;
  }
  
  selectBank(0);
  
  // Read accelerometer
  uint8_t accel_data[6];
  if(ESP32S3_I2C::ReadRegisterBuffer(bus_id_, address_, REG_ACCEL_XOUT_H, accel_data, 6) != HalResult::Success){
    return false;
  }
  
  int16_t accel_x_raw = (int16_t)((accel_data[0] << 8) | accel_data[1]);
  int16_t accel_y_raw = (int16_t)((accel_data[2] << 8) | accel_data[3]);
  int16_t accel_z_raw = (int16_t)((accel_data[4] << 8) | accel_data[5]);
  
  data.accel_x = accel_x_raw * accel_scale_;
  data.accel_y = accel_y_raw * accel_scale_;
  data.accel_z = accel_z_raw * accel_scale_;
  
  // Read gyroscope
  uint8_t gyro_data[6];
  if(ESP32S3_I2C::ReadRegisterBuffer(bus_id_, address_, REG_GYRO_XOUT_H, gyro_data, 6) != HalResult::Success){
    return false;
  }
  
  int16_t gyro_x_raw = (int16_t)((gyro_data[0] << 8) | gyro_data[1]);
  int16_t gyro_y_raw = (int16_t)((gyro_data[2] << 8) | gyro_data[3]);
  int16_t gyro_z_raw = (int16_t)((gyro_data[4] << 8) | gyro_data[5]);
  
  data.gyro_x = gyro_x_raw * gyro_scale_;
  data.gyro_y = gyro_y_raw * gyro_scale_;
  data.gyro_z = gyro_z_raw * gyro_scale_;
  
  // Read magnetometer if available
  if(mag_initialized_){
    uint8_t mag_data[8];
    if(ESP32S3_I2C::ReadRegisterBuffer(bus_id_, address_, REG_EXT_SLV_SENS_DATA_00, mag_data, 8) == HalResult::Success){
      int16_t mag_x_raw = (int16_t)((mag_data[1] << 8) | mag_data[0]);
      int16_t mag_y_raw = (int16_t)((mag_data[3] << 8) | mag_data[2]);
      int16_t mag_z_raw = (int16_t)((mag_data[5] << 8) | mag_data[4]);
      
      data.mag_x = mag_x_raw * MAG_SCALE;
      data.mag_y = mag_y_raw * MAG_SCALE;
      data.mag_z = mag_z_raw * MAG_SCALE;
    }else{
      data.mag_x = data.mag_y = data.mag_z = 0.0f;
    }
  }else{
    data.mag_x = data.mag_y = data.mag_z = 0.0f;
  }
  
  return true;
}

inline bool DRIVER_ICM20948::readAccelerometer(float& x, float& y, float& z){
  ICM20948Data data;
  if(!readData(data)){
    return false;
  }
  x = data.accel_x;
  y = data.accel_y;
  z = data.accel_z;
  return true;
}

inline bool DRIVER_ICM20948::readGyroscope(float& x, float& y, float& z){
  ICM20948Data data;
  if(!readData(data)){
    return false;
  }
  x = data.gyro_x;
  y = data.gyro_y;
  z = data.gyro_z;
  return true;
}

inline bool DRIVER_ICM20948::readMagnetometer(float& x, float& y, float& z){
  ICM20948Data data;
  if(!readData(data)){
    return false;
  }
  x = data.mag_x;
  y = data.mag_y;
  z = data.mag_z;
  return true;
}

inline bool DRIVER_ICM20948::isConnected(){
  selectBank(0);
  uint8_t who_am_i = 0;
  return ESP32S3_I2C::ReadRegister(bus_id_, address_, REG_WHO_AM_I, &who_am_i) == HalResult::Success 
         && who_am_i == CHIP_ID;
}

} // namespace abstraction
} // namespace arcos

#endif // ARCOS_ABSTRACTION_DRIVERS_ICM20948_IMPL_HPP_
