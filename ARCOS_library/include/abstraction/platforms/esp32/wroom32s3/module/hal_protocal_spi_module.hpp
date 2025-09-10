/*****************************************************************
 * File:      hal_protocal_spi_module.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge) - fixed by assistant
 *
 * Purpose:
 *   Practical SPI hardware abstraction for ESP32-WROOM-32S3.
 *   Provides runtime initialization and device handling wrappers
 *   around ESP-IDF spi_master functions.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_PROTOCALS_SPI_MODULE_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_PROTOCALS_SPI_MODULE_HPP_

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

namespace arcos::abstraction {

/**
 * @brief SPI hardware abstraction layer for ESP32-S3.
 *
 * Usage:
 *   1. Call Initialise() once per bus (SPI2_HOST or SPI3_HOST).
 *   2. Call AddDevice() to attach a device (chip-select + config).
 *   3. Use Transfer(), Transmit(), or Receive() for synchronous
 *      blocking I/O.
 */
struct HAL_PROTOCAL_SPI {
  HAL_PROTOCAL_SPI() = delete;

  /**
   * @brief Initialize an SPI bus with pins.
   *
   * @param host      SPI host (SPI1_HOST / SPI2_HOST / SPI3_HOST).
   * @param mosi      MOSI GPIO.
   * @param miso      MISO GPIO.
   * @param sclk      SCLK GPIO.
   * @param dma_chan  DMA channel (SPI_DMA_CH_AUTO or 0/1/2...).
   *
   * @return ESP_OK on success.
   */
  static inline esp_err_t Initialise(spi_host_device_t host,
                                     gpio_num_t mosi,
                                     gpio_num_t miso,
                                     gpio_num_t sclk,
                                     int dma_chan = SPI_DMA_CH_AUTO) {
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num      = mosi;
    buscfg.miso_io_num      = miso;
    buscfg.sclk_io_num      = sclk;
    buscfg.quadwp_io_num    = -1;
    buscfg.quadhd_io_num    = -1;
    buscfg.max_transfer_sz  = 4096;

    return spi_bus_initialize(host, &buscfg, dma_chan);
  }

  /**
   * @brief Add a device to the SPI bus.
   *
   * @param host            SPI host (e.g. SPI2_HOST).
   * @param cs_gpio         Chip-select GPIO.
   * @param out_handle      Pointer to device handle.
   * @param clock_speed_hz  Clock speed (Hz).
   * @param mode            SPI mode 0..3.
   * @param spics_io_num    Override CS pin (-1 uses cs_gpio).
   *
   * @return ESP_OK on success.
   */
  static inline esp_err_t AddDevice(spi_host_device_t host,
                                    gpio_num_t cs_gpio,
                                    spi_device_handle_t *out_handle,
                                    int clock_speed_hz = 10 * 1000 * 1000,
                                    int mode = 0,
                                    int spics_io_num = -1) {
    if (!out_handle) return ESP_ERR_INVALID_ARG;

    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = clock_speed_hz;
    devcfg.mode           = mode & 0x3;
    devcfg.spics_io_num   = (spics_io_num >= 0) ? spics_io_num : cs_gpio;
    devcfg.queue_size     = 1;
    devcfg.flags          = SPI_DEVICE_HALFDUPLEX;

    return spi_bus_add_device(host, &devcfg, out_handle);
  }

  /**
   * @brief Full-duplex transfer (blocking).
   *
   * @param handle      Device handle.
   * @param tx          Transmit buffer (nullable).
   * @param rx          Receive buffer (nullable).
   * @param length      Transfer length in bytes.
   * @param timeout_ms  Timeout (kept for API parity).
   *
   * @return ESP_OK on success.
   */
  static inline esp_err_t Transfer(spi_device_handle_t handle,
                                   const uint8_t *tx,
                                   uint8_t *rx,
                                   size_t length,
                                   uint32_t timeout_ms = portMAX_DELAY) {
    if (!handle || length == 0) return ESP_ERR_INVALID_ARG;

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length    = length * 8;
    t.tx_buffer = tx;
    t.rx_buffer = rx;

    (void)timeout_ms;
    return spi_device_transmit(handle, &t);
  }

  /**
   * @brief Simple transmit (discard RX).
   */
  static inline esp_err_t Transmit(spi_device_handle_t handle,
                                   const uint8_t *tx,
                                   size_t length,
                                   uint32_t timeout_ms = portMAX_DELAY) {
    return Transfer(handle, tx, nullptr, length, timeout_ms);
  }

  /**
   * @brief Simple receive (transmit 0xFF).
   */
  static inline esp_err_t Receive(spi_device_handle_t handle,
                                  uint8_t *rx,
                                  size_t length,
                                  uint32_t timeout_ms = portMAX_DELAY) {
    if (!rx || length == 0) return ESP_ERR_INVALID_ARG;

    if (length <= 64) {
      uint8_t temp[64];
      for (size_t i = 0; i < length; ++i) temp[i] = 0xFF;
      return Transfer(handle, temp, rx, length, timeout_ms);
    } else {
      uint8_t *temp = (uint8_t *)malloc(length);
      if (!temp) return ESP_ERR_NO_MEM;
      for (size_t i = 0; i < length; ++i) temp[i] = 0xFF;
      esp_err_t res = Transfer(handle, temp, rx, length, timeout_ms);
      free(temp);
      return res;
    }
  }
};

}  // namespace arcos::abstraction

#endif  // ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_PROTOCALS_SPI_MODULE_HPP_
