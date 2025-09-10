/*****************************************************************
 * File:      hal_gpio_pwm.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Provides a template for PWM HAL with runtime initialization
 *    and compile-time configuration.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_CORE_HAL_GPIO_PWM_HPP_
#define ARCOS_ABSTRACTION_CORE_HAL_GPIO_PWM_HPP_

#include <stdint.h>

namespace arcos::abstraction{
  namespace pwm {
    /**
     * @brief PWM configuration struct
     */
    struct PwmConfig {
      uint32_t frequency; // Hz
      uint8_t dutyCycle;  // 0-100%
    };
  } // namespace pwm

  /**
   * @brief HAL PWM abstraction template
   */
  template<typename PlatformImplementation,
           uintptr_t ChannelNumber,
           typename ChannelType,
           uint32_t Frequency = 1000,
           uint8_t DutyCycle = 50>
  struct HalPwm {
    private:
      inline static bool initialized_ = false;
    
    public:
      /**
       * @brief Initialise the PWM channel
       * @return true if successful, false otherwise
       */
      static inline bool Initialise(){
        initialized_ = PlatformImplementation::template Initialise<ChannelNumber, Frequency, DutyCycle>();
        return initialized_;
      }
    
      /**
       * @brief Check if the PWM channel is initialized
       */
      static inline bool IsInitialized(){return initialized_;}
    
      /**
       * @brief Set PWM frequency
       */
      static inline bool SetFrequency(uint32_t freq){
        if(!initialized_){return false;}
        return PlatformImplementation::template SetFrequency<ChannelNumber>(freq);
      }
    
      /**
       * @brief Set PWM duty cycle
       */
      static inline bool SetDutyCycle(uint8_t duty){
        if(!initialized_){return false;}
        return PlatformImplementation::template SetDutyCycle<ChannelNumber>(duty);
      }
    
      /**
       * @brief Start PWM output
       */
      static inline bool Start(){
        if(!initialized_){return false;}
        return PlatformImplementation::template Start<ChannelNumber>();
      }
    
      /**
       * @brief Stop PWM output
       */
      static inline bool Stop(){
        if(!initialized_){return false;}
        return PlatformImplementation::template Stop<ChannelNumber>();
      }
  };
} // namespace arcos::abstraction

#endif // ARCOS_ABSTRACTION_CORE_HAL_GPIO_PWM_HPP_