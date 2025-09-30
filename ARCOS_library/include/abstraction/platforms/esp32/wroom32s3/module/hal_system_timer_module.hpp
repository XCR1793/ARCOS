/*****************************************************************
 * File:      hal_system_timer_module.hpp
 * Category:  abstraction / platform
 * Author:    XCR1793 (Feather Forge) + assistant
 *
 * Purpose:
 *   ESP32-S3 (ESP-IDF) implementation of the HalSystemTimer
 *   platform binding used by arcos::abstraction::HAL_SYSTEM_TIMER.
 *
 * Notes:
 *   - Uses esp_timer_get_time() for microsecond timestamps.
 *   - Uses vTaskDelay() when FreeRTOS scheduler is running (cooperative
 *     with RTOS). If scheduler not started, delays fall back to a
 *     busy-wait using esp_timer_get_time().
 *   - All functions are static and inlined for zero runtime overhead
 *     when used as a template parameter.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_SYSTEM_TIMER_MODULE_HPP_
#define ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_SYSTEM_TIMER_MODULE_HPP_

#include <stdint.h>
#include "esp_timer.h"             // esp_timer_get_time()
#include "freertos/FreeRTOS.h"     // vTaskDelay, pdMS_TO_TICKS, taskSCHEDULER_NOT_STARTED
#include "freertos/task.h"

namespace arcos::abstraction{
  namespace HAL_SYSTEM_TIMER{

    struct ESP32S3 {
      /**
       * @brief Delay for ms milliseconds.
       *        Uses vTaskDelay() if the scheduler is running; otherwise
       *        falls back to busy-waiting using esp_timer_get_time().
       */
      static inline void Delay(uint32_t ms){
        if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED){
          if(ms == 0){
            taskYIELD();
          } else {
            vTaskDelay(pdMS_TO_TICKS(ms));
          }
        } else {
          const uint64_t start_us = static_cast<uint64_t>(esp_timer_get_time());
          const uint64_t wait_us  = static_cast<uint64_t>(ms) * 1000ULL;
          while((static_cast<uint64_t>(esp_timer_get_time()) - start_us) < wait_us){
            taskYIELD(); // cooperative multitasking in busy-wait
          }
        }
      }

      /**
       * @brief Delay for us microseconds.
       *        Busy-waits using esp_timer_get_time() for portability and
       *        determinism.
       */
      static inline void DelayMicroseconds(uint32_t us){
        const uint64_t start_us = static_cast<uint64_t>(esp_timer_get_time());
        while((static_cast<uint64_t>(esp_timer_get_time()) - start_us) < static_cast<uint64_t>(us)){
          taskYIELD();
        }
      }

      /**
       * @brief Returns milliseconds since boot (wraps at 2^32 ms).
       * @return Milliseconds elapsed as uint32_t
       */
      static inline uint32_t Millis(){
        const uint64_t us = static_cast<uint64_t>(esp_timer_get_time());
        return static_cast<uint32_t>(us / 1000ULL);
      }

      /**
       * @brief Returns microseconds since boot (wraps at 2^32 us).
       * @return Microseconds elapsed as uint32_t
       */
      static inline uint32_t Micros(){
        const uint64_t us = static_cast<uint64_t>(esp_timer_get_time());
        return static_cast<uint32_t>(us & 0xFFFFFFFFULL);
      }
    };

  } // namespace HAL_SYSTEM_TIMER
} // namespace arcos::abstraction

#endif // ARCOS_ABSTRACTION_PLATFORMS_ESP32_WROOM32S3_MODULE_HAL_SYSTEM_TIMER_MODULE_HPP_