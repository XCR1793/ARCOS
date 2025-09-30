/*****************************************************************
 * File:      hal_i2c_esp32s3.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    ESP32-S3 implementation of the standard I2C HAL interface.
 *    Provides direct implementation without requiring bridge
 *    patterns.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_INTERFACE_I2C_ESP32S3_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_INTERFACE_I2C_ESP32S3_HPP_

#include <driver/i2c.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include "../../../../core/hal_interface_i2c.hpp"

namespace arcos::abstraction{

  /**
   * @brief ESP32-S3 I2C HAL Implementation
   * 
   * Implements the standard HAL interface for ESP32-S3 using ESP-IDF.
   * Provides all methods required by the HalI2cInterface.
   */
  struct ESP32S3_I2C_HAL{
    static constexpr const char* TAG = "ESP32S3_I2C_HAL";
    
    /**
     * @brief Initialize I2C bus with specified configuration
     */
    static HalResult Initialize(uint8_t bus_id, 
                               uint8_t sda_pin, 
                               uint8_t scl_pin, 
                               uint32_t clock_speed_hz, 
                               uint32_t timeout_ms = 1000){
      
      if(bus_id >= I2C_NUM_MAX) {
        ESP_LOGE(TAG, "Invalid I2C bus ID: %d", bus_id);
        return HalResult::InvalidParameter;
      }
      
      i2c_port_t port = static_cast<i2c_port_t>(bus_id);
      
      i2c_config_t conf = {};
      conf.mode = I2C_MODE_MASTER;
      conf.sda_io_num = static_cast<gpio_num_t>(sda_pin);
      conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
      conf.scl_io_num = static_cast<gpio_num_t>(scl_pin);
      conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
      conf.master.clk_speed = clock_speed_hz;
      conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;
      
      esp_err_t ret = i2c_param_config(port, &conf);
      if(ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(ret));
        return HalResult::HardwareError;
      }
      
      ret = i2c_driver_install(port, conf.mode, 0, 0, 0);
      if(ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return HalResult::HardwareError;
      }
      
      // Set timeout
      ret = i2c_set_timeout(port, (timeout_ms * 80000)); // Convert ms to APB ticks
      if(ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set timeout: %s", esp_err_to_name(ret));
      }
      
      ESP_LOGI(TAG, "I2C%d initialized: SDA=%d, SCL=%d, Speed=%lu Hz", 
               bus_id, sda_pin, scl_pin, clock_speed_hz);
      return HalResult::Success;
    }
    
    /**
     * @brief Deinitialize I2C bus
     */
    static HalResult Deinitialize(uint8_t bus_id){
      if(bus_id >= I2C_NUM_MAX) {
        return HalResult::InvalidParameter;
      }
      
      i2c_port_t port = static_cast<i2c_port_t>(bus_id);
      esp_err_t ret = i2c_driver_delete(port);
      
      if(ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver delete failed: %s", esp_err_to_name(ret));
        return HalResult::HardwareError;
      }
      
      return HalResult::Success;
    }
    
    /**
     * @brief Write a single register value to I2C device
     */
    static HalResult WriteRegister(uint8_t bus_id,
                                  uint8_t device_address,
                                  uint8_t register_address,
                                  uint8_t value){
      if(bus_id >= I2C_NUM_MAX) {
        return HalResult::InvalidParameter;
      }
      
      i2c_port_t port = static_cast<i2c_port_t>(bus_id);
      
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_WRITE, true);
      i2c_master_write_byte(cmd, register_address, true);
      i2c_master_write_byte(cmd, value, true);
      i2c_master_stop(cmd);
      
      esp_err_t ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(1000));
      i2c_cmd_link_delete(cmd);
      
      return EspErrorToHalResult(ret);
    }
    
    /**
     * @brief Read a single register value from I2C device
     */
    static HalResult ReadRegister(uint8_t bus_id,
                                 uint8_t device_address,
                                 uint8_t register_address,
                                 uint8_t* value){
      if(bus_id >= I2C_NUM_MAX || value == nullptr) {
        return HalResult::InvalidParameter;
      }
      
      i2c_port_t port = static_cast<i2c_port_t>(bus_id);
      
      // Write register address
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_WRITE, true);
      i2c_master_write_byte(cmd, register_address, true);
      i2c_master_stop(cmd);
      
      esp_err_t ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(1000));
      i2c_cmd_link_delete(cmd);
      
      if(ret != ESP_OK) {
        return EspErrorToHalResult(ret);
      }
      
      // Read data
      cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_READ, true);
      i2c_master_read_byte(cmd, value, I2C_MASTER_NACK);
      i2c_master_stop(cmd);
      
      ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(1000));
      i2c_cmd_link_delete(cmd);
      
      return EspErrorToHalResult(ret);
    }
    
    /**
     * @brief Write multiple bytes to consecutive registers
     */
    static HalResult WriteRegisterBuffer(uint8_t bus_id,
                                        uint8_t device_address,
                                        uint8_t register_address,
                                        const uint8_t* buffer,
                                        size_t length){
      if(bus_id >= I2C_NUM_MAX || buffer == nullptr || length == 0) {
        return HalResult::InvalidParameter;
      }
      
      i2c_port_t port = static_cast<i2c_port_t>(bus_id);
      
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_WRITE, true);
      i2c_master_write_byte(cmd, register_address, true);
      i2c_master_write(cmd, buffer, length, true);
      i2c_master_stop(cmd);
      
      esp_err_t ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(1000));
      i2c_cmd_link_delete(cmd);
      
      return EspErrorToHalResult(ret);
    }
    
    /**
     * @brief Read multiple bytes from consecutive registers
     */
    static HalResult ReadRegisterBuffer(uint8_t bus_id,
                                       uint8_t device_address,
                                       uint8_t register_address,
                                       uint8_t* buffer,
                                       size_t length){
      if(bus_id >= I2C_NUM_MAX || buffer == nullptr || length == 0) {
        return HalResult::InvalidParameter;
      }
      
      i2c_port_t port = static_cast<i2c_port_t>(bus_id);
      
      // Write register address
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_WRITE, true);
      i2c_master_write_byte(cmd, register_address, true);
      i2c_master_stop(cmd);
      
      esp_err_t ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(1000));
      i2c_cmd_link_delete(cmd);
      
      if(ret != ESP_OK) {
        return EspErrorToHalResult(ret);
      }
      
      // Read data
      cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_READ, true);
      
      for(size_t i = 0; i < length; i++) {
        i2c_master_read_byte(cmd, &buffer[i], (i == length - 1) ? I2C_MASTER_NACK : I2C_MASTER_ACK);
      }
      
      i2c_master_stop(cmd);
      
      ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(1000));
      i2c_cmd_link_delete(cmd);
      
      return EspErrorToHalResult(ret);
    }
    
    /**
     * @brief Check if I2C device is present on the bus
     */
    static HalResult ProbeDevice(uint8_t bus_id, uint8_t device_address){
      if(bus_id >= I2C_NUM_MAX) {
        return HalResult::InvalidParameter;
      }
      
      i2c_port_t port = static_cast<i2c_port_t>(bus_id);
      
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_WRITE, true);
      i2c_master_stop(cmd);
      
      esp_err_t ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(100));
      i2c_cmd_link_delete(cmd);
      
      return EspErrorToHalResult(ret);
    }
    
    /**
     * @brief Set timeout for subsequent operations
     */
    static HalResult SetTimeout(uint8_t bus_id, uint32_t timeout_ms){
      if(bus_id >= I2C_NUM_MAX) {
        return HalResult::InvalidParameter;
      }
      
      i2c_port_t port = static_cast<i2c_port_t>(bus_id);
      esp_err_t ret = i2c_set_timeout(port, (timeout_ms * 80000)); // Convert ms to APB ticks
      
      return EspErrorToHalResult(ret);
    }
    
  private:
    /**
     * @brief Convert ESP-IDF error to HAL result
     */
    static HalResult EspErrorToHalResult(esp_err_t esp_err){
      switch(esp_err){
        case ESP_OK:                return HalResult::Success;
        case ESP_ERR_TIMEOUT:       return HalResult::Timeout;
        case ESP_ERR_INVALID_ARG:   return HalResult::InvalidParameter;
        case ESP_ERR_INVALID_STATE: return HalResult::NotInitialized;
        case ESP_FAIL:              return HalResult::DeviceNotFound;
        default:                    return HalResult::HardwareError;
      }
    }
  };

  // Type alias for the unified interface
  using ESP32S3_I2C = HalI2cInterface<ESP32S3_I2C_HAL>;

} // namespace arcos::abstraction

#endif // ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_INTERFACE_I2C_ESP32S3_HPP_