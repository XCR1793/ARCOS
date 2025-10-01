#include "i2c_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "I2C_DRIVER";

esp_err_t i2c_driver_init(void) {
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = 0;
    
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure I2C parameters: %s", esp_err_to_name(err));
        return err;
    }
    
    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "I2C driver initialized successfully (SCL: GPIO%d, SDA: GPIO%d, Freq: %d Hz)", 
             I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO, I2C_MASTER_FREQ_HZ);
    
    return ESP_OK;
}

esp_err_t i2c_driver_deinit(void) {
    esp_err_t err = i2c_driver_delete(I2C_MASTER_NUM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to delete I2C driver: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "I2C driver deinitialized successfully");
    return ESP_OK;
}

void i2c_scan_devices(void) {
    ESP_LOGI(TAG, "Scanning I2C bus for devices...");
    
    int devices_found = 0;
    for (uint8_t address = 1; address < 127; address++) {
        if (i2c_device_exists(address)) {
            ESP_LOGI(TAG, "Device found at address 0x%02X", address);
            devices_found++;
        }
    }
    
    if (devices_found == 0) {
        ESP_LOGW(TAG, "No I2C devices found");
    } else {
        ESP_LOGI(TAG, "I2C scan complete - found %d device(s)", devices_found);
    }
}

esp_err_t i2c_write_byte(uint8_t device_addr, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

esp_err_t i2c_write_bytes(uint8_t device_addr, const uint8_t* data, size_t length) {
    if (data == NULL || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, length, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

esp_err_t i2c_write_command(uint8_t device_addr, uint8_t control_byte, uint8_t command) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, control_byte, true);
    i2c_master_write_byte(cmd, command, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

esp_err_t i2c_write_data_stream(uint8_t device_addr, uint8_t control_byte, const uint8_t* data, size_t length) {
    if (data == NULL || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, control_byte, true);
    i2c_master_write(cmd, data, length, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

esp_err_t i2c_read_bytes(uint8_t device_addr, uint8_t* data, size_t length) {
    if (data == NULL || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_READ, true);
    
    if (length > 1) {
        i2c_master_read(cmd, data, length - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + length - 1, I2C_MASTER_NACK);
    
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

bool i2c_device_exists(uint8_t device_addr) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return (ret == ESP_OK);
}