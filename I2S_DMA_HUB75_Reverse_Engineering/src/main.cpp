#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "rom/lldesc.h"
#include "i2s_parallel.h"

constexpr int CLK_PIN = 37;
constexpr int BUS_PINS[8] = {7, 15, 16, 17, 18, 8, -1, -1};
constexpr int DMA_BUFFER_SIZE = 1024;

class I2SParallelDMA {
public:
    I2SParallelDMA() {
        std::memset(dma_buffer, 0, sizeof(dma_buffer));
        dma_desc.size = DMA_BUFFER_SIZE;
        dma_desc.length = DMA_BUFFER_SIZE;
        dma_desc.buf = dma_buffer;
        dma_desc.eof = 1;
        dma_desc.owner = 1;
        dma_desc.sosf = 0;
        dma_desc.qe.stqe_next = &dma_desc; // loop indefinitely
    }

    void init() {
        i2s_parallel_config_t cfg{};
        cfg.gpio_clk = CLK_PIN;
        std::memcpy(cfg.gpios_bus, BUS_PINS, sizeof(BUS_PINS));
        cfg.sample_width = I2S_PARALLEL_WIDTH_8;
        cfg.sample_rate = 1000000; // example

        esp_err_t err = i2s_parallel_driver_install(I2S_NUM_0, &cfg, false, nullptr, nullptr);
        if (err != ESP_OK) {
            printf("I2S driver install failed: %d\n", err);
        }
    }

    void startDMA() {
        esp_err_t err = i2s_parallel_send_dma(I2S_NUM_0, &dma_desc);
        if (err != ESP_OK) {
            printf("I2S DMA start failed: %d\n", err);
        }
    }

    uint8_t* buffer() { return dma_buffer; }

private:
    uint8_t dma_buffer[DMA_BUFFER_SIZE];
    lldesc_t dma_desc;
};

extern "C" void app_main() {
    I2SParallelDMA i2s;

    // Fill DMA buffer
    for (int i = 0; i < DMA_BUFFER_SIZE; ++i) {
        i2s.buffer()[i] = i;
    }

    i2s.init();
    i2s.startDMA();

    while (true) {
        for (int i = 0; i < DMA_BUFFER_SIZE; ++i) {
            i2s.buffer()[i] ^= 0xFF; // update pattern
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
