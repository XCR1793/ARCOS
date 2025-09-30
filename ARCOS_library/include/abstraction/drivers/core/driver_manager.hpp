/*****************************************************************
 * File:      driver_manager.hpp
 * Category:  abstraction
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Provides centralized driver management and device registry
 *    for ARCOS abstraction layer. Manages multiple devices on
 *    shared buses and provides unified initialization.
 *****************************************************************/

#ifndef ARCOS_ABSTRACTION_DRIVERS_CORE_DRIVER_MANAGER_HPP_
#define ARCOS_ABSTRACTION_DRIVERS_CORE_DRIVER_MANAGER_HPP_

#include "driver_base.hpp"
#include "../ICM20948/icm20948_driver.hpp"
#include "../BME280/bme280_driver.hpp"

namespace arcos::abstraction::drivers{

  /** Driver registry for managing multiple devices */
  template <typename HalI2cImplementation, typename HalSpiImplementation = void>
  class DriverManager{
  public:
    /**
     * @brief Initialize driver manager
     */
    DriverManager() : i2c_bus_initialized_(false), spi_bus_initialized_(false){
    }

    /**
     * @brief Create and register ICM20948 IMU driver
     * @param device_address I2C device address (default: 0x68)
     * @param bus_frequency I2C bus frequency in Hz (default: 400kHz)
     * @return Pointer to ICM20948 driver instance
     */
    ICM20948Driver<HalI2cImplementation>* CreateICM20948(uint8_t device_address = 0x68, uint32_t bus_frequency = 400000){
      // Create full I2C configuration with platform-specific pins
      I2cDeviceConfig config = {
        device_address,     // device_address
        0,                  // bus_id (I2C_NUM_0)
        9,                  // sda_pin (from working config)
        10,                 // scl_pin (from working config)
        bus_frequency,      // clock_speed_hz
        1000,               // timeout_ms
        false               // pullup_enable (handled by HAL)
      };
      
      auto* driver = new ICM20948Driver<HalI2cImplementation>(config);
      if(driver){
        icm_drivers_.push_back(driver);
        EnsureI2cBusInitialized();
      }
      return driver;
    }

    /**
     * @brief Create and register BME280 environmental sensor driver
     * @param device_address I2C device address (default: 0x76)
     * @param bus_frequency I2C bus frequency in Hz (default: 400kHz)
     * @return Pointer to BME280 driver instance
     */
    BME280Driver<HalI2cImplementation>* CreateBME280(uint8_t device_address = 0x76, uint32_t bus_frequency = 400000){
      // Create full I2C configuration with platform-specific pins
      I2cDeviceConfig config = {
        device_address,     // device_address
        0,                  // bus_id (I2C_NUM_0)
        9,                  // sda_pin (from working config)
        10,                 // scl_pin (from working config)
        bus_frequency,      // clock_speed_hz
        1000,               // timeout_ms
        false               // pullup_enable (handled by HAL)
      };
      
      auto* driver = new BME280Driver<HalI2cImplementation>(config);
      if(driver){
        bme_drivers_.push_back(driver);
        EnsureI2cBusInitialized();
      }
      return driver;
    }

    /**
     * @brief Initialize all registered drivers
     * @return DriverResult indicating success or failure
     */
    DriverResult InitializeAllDrivers(){
      DriverResult overall_result = DriverResult::Success;

      // Initialize ICM20948 drivers
      for(auto* driver : icm_drivers_){
        DriverResult result = driver->Initialize();
        if(result != DriverResult::Success){
          overall_result = result;
        }
      }

      // Initialize BME280 drivers
      for(auto* driver : bme_drivers_){
        DriverResult result = driver->Initialize();
        if(result != DriverResult::Success){
          overall_result = result;
        }
      }

      return overall_result;
    }

    /**
     * @brief Get number of registered ICM20948 drivers
     * @return Number of ICM20948 drivers
     */
    size_t GetICM20948Count() const{
      return icm_drivers_.size;
    }

    /**
     * @brief Get number of registered BME280 drivers
     * @return Number of BME280 drivers
     */
    size_t GetBME280Count() const{
      return bme_drivers_.size;
    }

    /**
     * @brief Get ICM20948 driver by index
     * @param index Driver index
     * @return Pointer to ICM20948 driver or nullptr if index invalid
     */
    ICM20948Driver<HalI2cImplementation>* GetICM20948(size_t index){
      if(index < icm_drivers_.size()){
        return icm_drivers_[index];
      }
      return nullptr;
    }

    /**
     * @brief Get BME280 driver by index
     * @param index Driver index
     * @return Pointer to BME280 driver or nullptr if index invalid
     */
    BME280Driver<HalI2cImplementation>* GetBME280(size_t index){
      if(index < bme_drivers_.size()){
        return bme_drivers_[index];
      }
      return nullptr;
    }

    /**
     * @brief Check if all drivers are ready
     * @return True if all drivers are ready, false otherwise
     */
    bool AreAllDriversReady() const{
      for(const auto* driver : icm_drivers_){
        if(!driver->IsReady()) return false;
      }
      
      for(const auto* driver : bme_drivers_){
        if(!driver->IsReady()) return false;
      }
      
      return true;
    }

    /**
     * @brief Destructor - cleanup all drivers
     */
    ~DriverManager(){
      for(auto* driver : icm_drivers_){
        delete driver;
      }
      icm_drivers_.clear();

      for(auto* driver : bme_drivers_){
        delete driver;
      }
      bme_drivers_.clear();
    }

  private:
    // Simple vector-like containers (replace with std::vector if available)
    static constexpr size_t MAX_DRIVERS = 10;
    
    bool i2c_bus_initialized_;
    bool spi_bus_initialized_;

    // Simple vector-like functionality
    struct ICMVector{
      ICM20948Driver<HalI2cImplementation>* data[MAX_DRIVERS];
      size_t size = 0;
      
      void push_back(ICM20948Driver<HalI2cImplementation>* item){
        if(size < MAX_DRIVERS){
          data[size++] = item;
        }
      }
      
      ICM20948Driver<HalI2cImplementation>* operator[](size_t index){
        return (index < size) ? data[index] : nullptr;
      }
      
      // Iterator support
      ICM20948Driver<HalI2cImplementation>** begin() { return data; }
      ICM20948Driver<HalI2cImplementation>** end() { return data + size; }
      const ICM20948Driver<HalI2cImplementation>* const* begin() const { return data; }
      const ICM20948Driver<HalI2cImplementation>* const* end() const { return data + size; }
      
      void clear(){
        size = 0;
      }
    } icm_drivers_;

    struct BMEVector{
      BME280Driver<HalI2cImplementation>* data[MAX_DRIVERS];
      size_t size = 0;
      
      void push_back(BME280Driver<HalI2cImplementation>* item){
        if(size < MAX_DRIVERS){
          data[size++] = item;
        }
      }
      
      BME280Driver<HalI2cImplementation>* operator[](size_t index){
        return (index < size) ? data[index] : nullptr;
      }
      
      // Iterator support
      BME280Driver<HalI2cImplementation>** begin() { return data; }
      BME280Driver<HalI2cImplementation>** end() { return data + size; }
      const BME280Driver<HalI2cImplementation>* const* begin() const { return data; }
      const BME280Driver<HalI2cImplementation>* const* end() const { return data + size; }
      
      void clear(){
        size = 0;
      }
    } bme_drivers_;

    /**
     * @brief Ensure I2C bus is initialized (called automatically)
     */
    void EnsureI2cBusInitialized(){
      if(!i2c_bus_initialized_){
        // Initialize I2C bus with default ESP32-S3 pins
        HalI2cImplementation::Initialize(0, 9, 10, 400000, 1000); // SDA=9, SCL=10
        i2c_bus_initialized_ = true;
      }
    }

    /**
     * @brief Ensure SPI bus is initialized (called automatically)
     */
    void EnsurSpiBusInitialized(){
      if(!spi_bus_initialized_ && !std::is_void_v<HalSpiImplementation>){
        // Initialize SPI HAL if not already done
        HalSpiImplementation::InitializeBus();
        spi_bus_initialized_ = true;
      }
    }
  };

}; // arcos::abstraction::drivers

#endif // ARCOS_ABSTRACTION_DRIVERS_CORE_DRIVER_MANAGER_HPP_