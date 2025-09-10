/*****************************************************************
 * File:      hal_protocal_i2c_module.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Implements standard I2C hardware abstraction for the
 *    esp32s3 modules using ESP-IDF. Supports blocking (slow)
 *    and non-blocking (fast) single and multi-byte transfers
 *    with configurable pins.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_PROTOCAL_I2C_MODULE_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_PROTOCAL_I2C_MODULE_HPP_

#include <driver/i2c.h>
#include <driver/gpio.h>
#include <stdint.h>
#include <stdio.h>
#include <atomic>

namespace arcos::abstraction{
  struct HAL_PROTOCAL_I2C{
    static constexpr i2c_port_t DEFAULT_PORT = static_cast<i2c_port_t>(0);
    static constexpr gpio_num_t DEFAULT_SDA  = static_cast<gpio_num_t>(21);
    static constexpr gpio_num_t DEFAULT_SCL  = static_cast<gpio_num_t>(22);
    static constexpr uint32_t   DEFAULT_CLK  = 400000;

    /** Flag for non-blocking transfer completion */
    static inline std::atomic<bool> transfer_done{true};

    /**
     * @brief Initialise I2C bus with SDA/SCL pins and speed
     * @param port       I2C port (0 or 1)
     * @param sda        SDA GPIO number
     * @param scl        SCL GPIO number
     * @param clk_speed  Clock speed in Hz
     */
    static inline void Initialise(i2c_port_t port = DEFAULT_PORT,
                                  gpio_num_t sda = DEFAULT_SDA,
                                  gpio_num_t scl = DEFAULT_SCL,
                                  uint32_t clk_speed = DEFAULT_CLK){
      i2c_config_t conf{};
      conf.mode = I2C_MODE_MASTER;
      conf.sda_io_num = sda;
      conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
      conf.scl_io_num = scl;
      conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
      conf.master.clk_speed = clk_speed;
      i2c_param_config(port, &conf);
      i2c_driver_install(port, conf.mode, 0, 0, 0);
    }

    /*************************************************************
     * SLOW / BLOCKING FUNCTIONS
     *************************************************************/

    /**
     * @brief Blocking single byte write
     * @param port       I2C port
     * @param deviceAddr 7-bit device address
     * @param data       Byte to send
     * @return true if success, false if bus busy or fail
     */
    static inline bool WriteByte(i2c_port_t port, uint8_t deviceAddr, uint8_t data){
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (deviceAddr << 1) | I2C_MASTER_WRITE, true);
      i2c_master_write_byte(cmd, data, true);
      i2c_master_stop(cmd);
      esp_err_t ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(100)); // blocking 100ms
      i2c_cmd_link_delete(cmd);
      return ret == ESP_OK;
    }

    /**
     * @brief Blocking single byte read
     * @param port       I2C port
     * @param deviceAddr 7-bit device address
     * @param data       Reference to store received byte
     * @return true if success, false if bus busy or fail
     */
    static inline bool ReadByte(i2c_port_t port, uint8_t deviceAddr, uint8_t &data){
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (deviceAddr << 1) | I2C_MASTER_READ, true);
      i2c_master_read_byte(cmd, &data, I2C_MASTER_NACK);
      i2c_master_stop(cmd);
      esp_err_t ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(100)); // blocking 100ms
      i2c_cmd_link_delete(cmd);
      return ret == ESP_OK;
    }

    /**
     * @brief Blocking multi-byte write
     */
    static inline bool WriteBytes(i2c_port_t port, uint8_t deviceAddr, const uint8_t* data, uintptr_t length){
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (deviceAddr << 1) | I2C_MASTER_WRITE, true);
      i2c_master_write(cmd, const_cast<uint8_t*>(data), length, true);
      i2c_master_stop(cmd);
      esp_err_t ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(100)); // blocking
      i2c_cmd_link_delete(cmd);
      return ret == ESP_OK;
    }

    /**
     * @brief Blocking multi-byte read
     */
    static inline bool ReadBytes(i2c_port_t port, uint8_t deviceAddr, uint8_t* buffer, uintptr_t length){
      if(length == 0) return false;
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (deviceAddr << 1) | I2C_MASTER_READ, true);
      if(length > 1)
        i2c_master_read(cmd, buffer, length - 1, I2C_MASTER_ACK);
      i2c_master_read_byte(cmd, &buffer[length - 1], I2C_MASTER_NACK);
      i2c_master_stop(cmd);
      esp_err_t ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(100)); // blocking
      i2c_cmd_link_delete(cmd);
      return ret == ESP_OK;
    }

    /*************************************************************
     * FAST / NON-BLOCKING FUNCTIONS
     *************************************************************/

    /**
     * @brief Start non-blocking single byte write
     */
    static inline bool FastWriteByte(i2c_port_t port, uint8_t deviceAddr, uint8_t data){
      if(!transfer_done) return false; // previous transfer still active
      transfer_done = false;

      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (deviceAddr << 1) | I2C_MASTER_WRITE, true);
      i2c_master_write_byte(cmd, data, true);
      i2c_master_stop(cmd);

      esp_err_t ret = i2c_master_cmd_begin(port, cmd, 0); // non-blocking, returns immediately
      i2c_cmd_link_delete(cmd);
      transfer_done = true;
      return ret == ESP_OK;
    }

    /**
     * @brief Start non-blocking single byte read
     */
    static inline bool FastReadByte(i2c_port_t port, uint8_t deviceAddr, uint8_t &data){
      if(!transfer_done) return false;
      transfer_done = false;

      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (deviceAddr << 1) | I2C_MASTER_READ, true);
      i2c_master_read_byte(cmd, &data, I2C_MASTER_NACK);
      i2c_master_stop(cmd);

      esp_err_t ret = i2c_master_cmd_begin(port, cmd, 0); // non-blocking
      i2c_cmd_link_delete(cmd);
      transfer_done = true;
      return ret == ESP_OK;
    }

    /**
     * @brief Start non-blocking multi-byte write
     */
    static inline bool FastWriteBytes(i2c_port_t port, uint8_t deviceAddr, const uint8_t* data, uintptr_t length){
      if(!transfer_done) return false;
      transfer_done = false;

      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (deviceAddr << 1) | I2C_MASTER_WRITE, true);
      i2c_master_write(cmd, const_cast<uint8_t*>(data), length, true);
      i2c_master_stop(cmd);

      esp_err_t ret = i2c_master_cmd_begin(port, cmd, 0); // non-blocking
      i2c_cmd_link_delete(cmd);
      transfer_done = true;
      return ret == ESP_OK;
    }

    /**
     * @brief Start non-blocking multi-byte read
     */
    static inline bool FastReadBytes(i2c_port_t port, uint8_t deviceAddr, uint8_t* buffer, uintptr_t length){
      if(!transfer_done) return false;
      if(length == 0) return false;
      transfer_done = false;

      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (deviceAddr << 1) | I2C_MASTER_READ, true);
      if(length > 1)
        i2c_master_read(cmd, buffer, length - 1, I2C_MASTER_ACK);
      i2c_master_read_byte(cmd, &buffer[length - 1], I2C_MASTER_NACK);
      i2c_master_stop(cmd);

      esp_err_t ret = i2c_master_cmd_begin(port, cmd, 0); // non-blocking
      i2c_cmd_link_delete(cmd);
      transfer_done = true;
      return ret == ESP_OK;
    }

    /**
     * @brief Change bus speed at runtime
     * @param port       I2C port
     * @param clk_speed  New clock speed in Hz
     */
    static inline void SetSpeed(i2c_port_t port, uint32_t clk_speed){
      i2c_config_t conf{};
      conf.mode = I2C_MODE_MASTER;
      conf.master.clk_speed = clk_speed;
      i2c_param_config(port, &conf);
    }

    /**
     * @brief Reset the bus if stuck
     * @param port I2C port
     */
    static inline void ResetBus(i2c_port_t port){
      i2c_driver_delete(port);
      i2c_driver_install(port, I2C_MODE_MASTER, 0, 0, 0);
    }
  };
};

#endif // ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_PROTOCAL_I2C_MODULE_HPP_
