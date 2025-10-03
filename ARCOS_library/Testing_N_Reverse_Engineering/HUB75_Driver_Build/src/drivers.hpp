/*****************************************************************
 * File:      drivers.hpp
 * Category:  abstraction
 * 
 * Purpose:
 *    Main drivers header - includes all device driver interfaces
 *    and provides unified access to HUB75 display drivers
 *    
 * Usage:
 *    #include "drivers.hpp"  // All drivers available
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_HPP_

// Core driver abstractions
#include "drivers/core/driver_base.hpp"

// Device drivers
#include "drivers/communication/HUB75/driver_hub75.hpp"

// HAL abstraction (ensure HAL is included)
#include "hal.hpp"

namespace arcos::abstraction::drivers{

  /**
   * Type aliases for common driver configurations
   * These provide convenient access to driver types
   */
  
  // HUB75 display driver using default HAL parallel implementation
  using HUB75Display = HUB75Driver;

  /**
   * Quick initialization helper
   * NOTE: Driver now requires dependency injection - use with caution
   */
  inline HUB75Driver* CreateHUB75Display(){
    return new HUB75Driver();
  }

} // namespace arcos::abstraction::drivers

#endif // ARCOS_ABSTRACTION_DRIVERS_HPP_
