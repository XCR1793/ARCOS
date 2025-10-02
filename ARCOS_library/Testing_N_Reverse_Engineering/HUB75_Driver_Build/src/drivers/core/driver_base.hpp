/*****************************************************************
 * File:      driver_base.hpp
 * Category:  drivers/core
 * 
 * Purpose:
 *    Base interface for all HUB75-related device drivers
 *****************************************************************/

#ifndef HUB75_DRIVERS_CORE_DRIVER_BASE_HPP_
#define HUB75_DRIVERS_CORE_DRIVER_BASE_HPP_

#include <stdint.h>

namespace hub75::drivers{

  /** Driver operation result codes */
  enum struct DriverResult{
    Success = 0,
    ErrorInit = 1,
    ErrorCommunication = 2,
    ErrorNotReady = 3,
    ErrorUnknown = 4
  };

  /** Base interface for all drivers */
  template <typename ConcreteDriver>
  class DriverBase{
  public:
    DriverResult Initialize(){
      return static_cast<ConcreteDriver*>(this)->InitializeImpl();
    }

    bool IsReady() const{
      return static_cast<const ConcreteDriver*>(this)->IsReadyImpl();
    }

    DriverResult Reset(){
      return static_cast<ConcreteDriver*>(this)->ResetImpl();
    }

  protected:
    bool initialized_ = false;
  };

} // namespace hub75::drivers

#endif // HUB75_DRIVERS_CORE_DRIVER_BASE_HPP_
