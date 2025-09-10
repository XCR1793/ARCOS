/*****************************************************************
 * File:      hal_system_timer.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Provides a template HAL abstraction for system timer
 *    operations such as millisecond/microsecond counters
 *    and delay routines. Zero runtime overhead when used
 *    with compile-time platform bindings.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_CORE_HAL_SYSTEM_TIMER_HPP_
#define ARCOS_ABSTRACTION_CORE_HAL_SYSTEM_TIMER_HPP_

#include <stdint.h>

namespace arcos::abstraction{
  namespace HAL_SYSTEM_TIMER{

    /**
     * @brief Generic system timer interface for HAL.
     * 
     * PlatformImplementation must provide:
     *   static void Delay(uint32_t ms);
     *   static void DelayMicroseconds(uint32_t us);
     *   static uint32_t Millis();
     *   static uint32_t Micros();
     */
    template <typename PlatformImplementation>
    struct HalSystemTimer{

      /**
       * @brief Delays execution for a given number of milliseconds.
       * @param ms Number of milliseconds to wait
       */
      static inline void Delay(uint32_t ms){
        PlatformImplementation::Delay(ms);
      }

      /**
       * @brief Delays execution for a given number of microseconds.
       * @param us Number of microseconds to wait
       */
      static inline void DelayMicroseconds(uint32_t us){
        PlatformImplementation::DelayMicroseconds(us);
      }

      /**
       * @brief Returns the number of milliseconds since system start.
       * @return Milliseconds elapsed
       */
      static inline uint32_t Millis(){
        return PlatformImplementation::Millis();
      }

      /**
       * @brief Returns the number of microseconds since system start.
       * @return Microseconds elapsed
       */
      static inline uint32_t Micros(){
        return PlatformImplementation::Micros();
      }
    };

  } // namespace systimer
} // namespace arcos::abstraction

#endif // ARCOS_ABSTRACTION_CORE_HAL_SYSTEM_TIMER_HPP_