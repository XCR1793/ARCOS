/*****************************************************************
 * File:      hal_gpio_pwm_esp32.hpp
 * Category:  abstraction / platform
 * Author:    Assistant (ESP32-S3/ESP-IDF implementation)
 * 
 * Purpose:
 *    Provides an ESP32-S3-specific implementation of the PWM HAL
 *    using ESP-IDF’s LEDC driver. Supports compile-time channel
 *    mapping and runtime frequency/duty control.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_PLATFORM_ESP32_WROOM32S3_MODULE_HAL_GPIO_PWM_MODULE_HPP_
#define ARCOS_ABSTRACTION_PLATFORM_ESP32_WROOM32S3_MODULE_HAL_GPIO_PWM_MODULE_HPP_

#include <stdint.h>
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

namespace arcos::abstraction{
  namespace HAL_GPIO_PWM{
    /**
     * @brief ESP32-S3 PWM PlatformImplementation
     */
    struct Esp32PwmPlatform {
      private:
        inline static constexpr uint32_t DutyResolutionBits = 13;
        inline static bool channel_initialized_[LEDC_CHANNEL_MAX] = {false};
        inline static int8_t channel_gpio_map_[LEDC_CHANNEL_MAX] = {
          -1, -1, -1, -1, -1, -1, -1, -1,
#if LEDC_CHANNEL_MAX > 8
          -1, -1, -1, -1, -1, -1, -1, -1
#endif
        };
        inline static constexpr const char *TAG = "Esp32PwmPlatform";
      
        template<uintptr_t ChannelNumber>
        static inline ledc_channel_t ChannelToLedcChannel(){
          return static_cast<ledc_channel_t>(ChannelNumber % LEDC_CHANNEL_MAX);
        }
      
        template<uintptr_t ChannelNumber>
        static inline ledc_timer_t ChannelToLedcTimer(){
          return static_cast<ledc_timer_t>(ChannelNumber % LEDC_TIMER_MAX);
        }
      
        template<uintptr_t ChannelNumber>
        static inline bool ChannelValid(){
          return (ChannelNumber < static_cast<uintptr_t>(LEDC_CHANNEL_MAX));
        }
      
      public:
        /**
         * @brief Assign GPIO to channel
         */
        static inline void SetPinForChannel(uintptr_t channel, int gpio){
          if(channel >= static_cast<uintptr_t>(LEDC_CHANNEL_MAX)){
            return;
          }
          channel_gpio_map_[channel] = static_cast<int8_t>(gpio);
        }
      
        /**
         * @brief Initialise PWM channel
         */
        template<uintptr_t ChannelNumber, uint32_t Frequency, uint8_t DutyCycle>
        static inline bool Initialise(){
          if(!ChannelValid<ChannelNumber>()){
            return false;
          }
          const int ch = static_cast<int>(ChannelNumber);
          const int gpio = channel_gpio_map_[ch];
          if(gpio < 0){
            return false;
          }
        
          ledc_timer_config_t timer_conf{};
          timer_conf.speed_mode = LEDC_LOW_SPEED_MODE; // S3 only has low-speed mode
          timer_conf.timer_num = ChannelToLedcTimer<ChannelNumber>();
          timer_conf.duty_resolution = static_cast<ledc_timer_bit_t>(DutyResolutionBits);
          timer_conf.freq_hz = static_cast<int>(Frequency);
          timer_conf.clk_cfg = LEDC_AUTO_CLK;
          if(ledc_timer_config(&timer_conf) != ESP_OK){
            return false;
          }
        
          const uint32_t max_duty = ((1u << DutyResolutionBits) - 1u);
          const uint32_t duty_val = (static_cast<uint32_t>(DutyCycle) * max_duty) / 100u;
        
          ledc_channel_config_t ch_conf{};
          ch_conf.gpio_num = static_cast<gpio_num_t>(gpio);
          ch_conf.speed_mode = LEDC_LOW_SPEED_MODE;
          ch_conf.channel = ChannelToLedcChannel<ChannelNumber>();
          ch_conf.intr_type = LEDC_INTR_DISABLE;
          ch_conf.timer_sel = timer_conf.timer_num;
          ch_conf.duty = duty_val;
          ch_conf.hpoint = 0;
          if(ledc_channel_config(&ch_conf) != ESP_OK){
            return false;
          }
        
          ledc_fade_func_install(0);
          channel_initialized_[ch] = true;
          return true;
        }
      
        /**
         * @brief Set PWM frequency
         */
        template<uintptr_t ChannelNumber>
        static inline bool SetFrequency(uint32_t freq){
          if(!ChannelValid<ChannelNumber>()){
            return false;
          }
          ledc_timer_config_t timer_conf{};
          timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
          timer_conf.timer_num = ChannelToLedcTimer<ChannelNumber>();
          timer_conf.duty_resolution = static_cast<ledc_timer_bit_t>(DutyResolutionBits);
          timer_conf.freq_hz = static_cast<int>(freq);
          timer_conf.clk_cfg = LEDC_AUTO_CLK;
          return (ledc_timer_config(&timer_conf) == ESP_OK);
        }
      
        /**
         * @brief Set PWM duty cycle
         */
        template<uintptr_t ChannelNumber>
        static inline bool SetDutyCycle(uint8_t duty){
          if(!ChannelValid<ChannelNumber>()){
            return false;
          }
          const int ch = static_cast<int>(ChannelNumber);
          if(!channel_initialized_[ch]){
            return false;
          }
        
          const uint32_t max_duty = ((1u << DutyResolutionBits) - 1u);
          const uint32_t duty_val = (static_cast<uint32_t>(duty) * max_duty) / 100u;
        
          ledc_channel_t ledc_ch = ChannelToLedcChannel<ChannelNumber>();
          if(ledc_set_duty(LEDC_LOW_SPEED_MODE, ledc_ch, duty_val) != ESP_OK){
            return false;
          }
          if(ledc_update_duty(LEDC_LOW_SPEED_MODE, ledc_ch) != ESP_OK){
            return false;
          }
          return true;
        }
      
        /**
         * @brief Start PWM output
         */
        template<uintptr_t ChannelNumber>
        static inline bool Start(){
          if(!ChannelValid<ChannelNumber>()){
            return false;
          }
          const int ch = static_cast<int>(ChannelNumber);
          if(!channel_initialized_[ch]){
            return false;
          }
          ledc_channel_t ledc_ch = ChannelToLedcChannel<ChannelNumber>();
          return (ledc_update_duty(LEDC_LOW_SPEED_MODE, ledc_ch) == ESP_OK);
        }
      
        /**
         * @brief Stop PWM output
         */
        template<uintptr_t ChannelNumber>
        static inline bool Stop(){
          if(!ChannelValid<ChannelNumber>()){
            return false;
          }
          const int ch = static_cast<int>(ChannelNumber);
          if(!channel_initialized_[ch]){
            return false;
          }
          ledc_channel_t ledc_ch = ChannelToLedcChannel<ChannelNumber>();
          if(ledc_set_duty(LEDC_LOW_SPEED_MODE, ledc_ch, 0) != ESP_OK){
            return false;
          }
          return (ledc_update_duty(LEDC_LOW_SPEED_MODE, ledc_ch) == ESP_OK);
        }
    };
  } // namespace HAL_GPIO_PWM
} // namespace arcos::abstraction

#endif // ARCOS_ABSTRACTION_PLATFORM_ESP32_WROOM32S3_MODULE_HAL_GPIO_PWM_MODULE_HPP_
