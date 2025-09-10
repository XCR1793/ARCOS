#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdint.h>

#define I2C_PORT I2C_NUM_0
#define SDA_PIN 9
#define SCL_PIN 10
#define I2C_FREQ 400000

#define ICM_ADDR 0x68
#define AK09916_ADDR 0x0C

// Helper I2C functions
bool i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, val, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret == ESP_OK;
}

bool i2c_read_regs(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    // Set register
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    if(ret != ESP_OK) return false;

    // Read data
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    if(len > 1) {
        i2c_master_read(cmd, buf, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, &buf[len - 1], I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret == ESP_OK;
}

// Initialize ICM20948 with AK09916 magnetometer
bool icm_init_mag() {
    // Reset and wake ICM
    i2c_write_reg(ICM_ADDR, 0x7F, 0x00); // Bank 0
    i2c_write_reg(ICM_ADDR, 0x06, 0x01); // PWR_MGMT_1: clock auto
    vTaskDelay(pdMS_TO_TICKS(10));

    // Enable I2C master mode
    i2c_write_reg(ICM_ADDR, 0x6A, 0x20); // USER_CTRL: I2C_MST_EN
    i2c_write_reg(ICM_ADDR, 0x24, 0x0D); // I2C_MST_CTRL: 400kHz, multi-master

    // Configure slave 0 to read AK09916 continuously
    i2c_write_reg(ICM_ADDR, 0x25, AK09916_ADDR | 0x80); // SLV0_ADDR | read
    i2c_write_reg(ICM_ADDR, 0x26, 0x10); // SLV0_REG: HXL
    i2c_write_reg(ICM_ADDR, 0x27, 0x87); // SLV0_CTRL: enable, 6 bytes

    // Set magnetometer to continuous mode 2 (100Hz)
    i2c_write_reg(AK09916_ADDR, 0x31, 0x08);
    vTaskDelay(pdMS_TO_TICKS(10));

    return true;
}

// Read accel, gyro, magnetometer
bool icm_read(float &ax, float &ay, float &az,
              float &gx, float &gy, float &gz,
              float &mx, float &my, float &mz) {
    uint8_t buf[12];
    if(!i2c_read_regs(ICM_ADDR, 0x2D, buf, 12)) return false;

    int16_t raw_ax = (buf[0] << 8) | buf[1];
    int16_t raw_ay = (buf[2] << 8) | buf[3];
    int16_t raw_az = (buf[4] << 8) | buf[5];
    int16_t raw_gx = (buf[6] << 8) | buf[7];
    int16_t raw_gy = (buf[8] << 8) | buf[9];
    int16_t raw_gz = (buf[10] << 8) | buf[11];

    ax = raw_ax / 16384.0f * 9.80665f;
    ay = raw_ay / 16384.0f * 9.80665f;
    az = raw_az / 16384.0f * 9.80665f;
    gx = raw_gx / 131.0f;
    gy = raw_gy / 131.0f;
    gz = raw_gz / 131.0f;

    // Read magnetometer via EXT_SENS_DATA_00..05
    uint8_t mag[6];
    if(!i2c_read_regs(ICM_ADDR, 0x49, mag, 6)) {
        mx = my = mz = 0.0f;
    } else {
        int16_t raw_mx = (mag[1] << 8) | mag[0];
        int16_t raw_my = (mag[3] << 8) | mag[2];
        int16_t raw_mz = (mag[5] << 8) | mag[4];
        mx = raw_mx * 0.15f;
        my = raw_my * 0.15f;
        mz = raw_mz * 0.15f;
    }

    return true;
}

extern "C" void app_main(void) {
    i2c_config_t conf{};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = SDA_PIN;
    conf.scl_io_num = SCL_PIN;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_FREQ;
    i2c_param_config(I2C_PORT, &conf);
    i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);

    if(!icm_init_mag()) {
        printf("ICM20948 + magnetometer init failed\n");
        return;
    }

    printf("ax ay az gx gy gz mx my mz\n");
    while(true) {
        float ax, ay, az, gx, gy, gz, mx, my, mz;
        if(icm_read(ax, ay, az, gx, gy, gz, mx, my, mz)) {
            printf("%6.2f %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f %6.2f\n",
                   ax, ay, az, gx, gy, gz, mx, my, mz);
        } else {
            printf("read failed\n");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
