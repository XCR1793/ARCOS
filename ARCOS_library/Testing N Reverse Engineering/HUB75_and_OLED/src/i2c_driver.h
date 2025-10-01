#ifndef I2/** I2C Configuration - Try common ESP32-S3 I2C pins */
#define I2C_DRIVER_H

#include "driver/i2c.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** I2C Configuration */
#define I2C_MASTER_SCL_IO           2    // GPIO2 for SCL
#define I2C_MASTER_SDA_IO           1    // GPIO1 for SDA
#define I2C_MASTER_NUM              I2C_NUM_0     // I2C port number
#define I2C_MASTER_FREQ_HZ          100000        // 100kHz - More conservative speed
#define I2C_MASTER_TX_BUF_DISABLE   0             // I2C master doesn't need buffer
#define I2C_MASTER_RX_BUF_DISABLE   0             // I2C master doesn't need buffer
#define I2C_MASTER_TIMEOUT_MS       100           // Timeout for operations

/**
 * @brief Initialize I2C master driver
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t i2c_driver_init(void);

/**
 * @brief Deinitialize I2C master driver
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t i2c_driver_deinit(void);

/**
 * @brief Scan I2C bus for devices
 * 
 * Scans addresses 0x01 to 0x7F and logs found devices
 */
void i2c_scan_devices(void);

/**
 * @brief Write a single byte to I2C device
 * 
 * @param device_addr I2C device address (7-bit)
 * @param data Byte to write
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t i2c_write_byte(uint8_t device_addr, uint8_t data);

/**
 * @brief Write multiple bytes to I2C device
 * 
 * @param device_addr I2C device address (7-bit)
 * @param data Pointer to data buffer
 * @param length Number of bytes to write
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t i2c_write_bytes(uint8_t device_addr, const uint8_t* data, size_t length);

/**
 * @brief Write command byte with control byte to I2C device
 * 
 * @param device_addr I2C device address (7-bit)
 * @param control_byte Control byte to send first
 * @param command Command byte to send
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t i2c_write_command(uint8_t device_addr, uint8_t control_byte, uint8_t command);

/**
 * @brief Write data with control byte to I2C device
 * 
 * @param device_addr I2C device address (7-bit)
 * @param control_byte Control byte to send first
 * @param data Pointer to data buffer
 * @param length Number of bytes to write
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t i2c_write_data_stream(uint8_t device_addr, uint8_t control_byte, const uint8_t* data, size_t length);

/**
 * @brief Read bytes from I2C device
 * 
 * @param device_addr I2C device address (7-bit)
 * @param data Pointer to buffer to store read data
 * @param length Number of bytes to read
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t i2c_read_bytes(uint8_t device_addr, uint8_t* data, size_t length);

/**
 * @brief Check if device exists on I2C bus
 * 
 * @param device_addr I2C device address (7-bit)
 * @return true Device responds to address
 * @return false Device does not respond
 */
bool i2c_device_exists(uint8_t device_addr);

#ifdef __cplusplus
}
#endif

#endif // I2C_DRIVER_H