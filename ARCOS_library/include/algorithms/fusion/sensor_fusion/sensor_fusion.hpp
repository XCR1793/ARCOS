/*****************************************************************
 * File:      sensor_fusion.hpp
 * Category:  algorithms/fusion/sensor_fusion
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Generic sensor fusion algorithm for combining multiple sensor
 *    readings with configurable weighting and filtering strategies.
 *****************************************************************/

#ifndef ARCOS_ALGORITHMS_FUSION_SENSOR_FUSION_HPP_
#define ARCOS_ALGORITHMS_FUSION_SENSOR_FUSION_HPP_

#include <stdint.h>

namespace arcos::algorithms::fusion{

/** Sensor fusion configuration */
struct SensorFusionConfig{
  float weight_sensor_a = 0.5f;
  float weight_sensor_b = 0.5f;
  bool enable_filtering = true;
  float filter_alpha = 0.1f;  // Low-pass filter coefficient
};

/** Generic sensor fusion algorithm
 * 
 * Combines data from multiple sensors using weighted averaging
 * and optional filtering for improved accuracy and stability.
 */
template<typename T>
class SensorFusion{
public:
  SensorFusion();
  ~SensorFusion();
  
  /** Initialize fusion algorithm
   * @param config Fusion configuration parameters
   * @return true if successful
   */
  bool init(const SensorFusionConfig& config);
  
  /** Fuse two sensor readings
   * @param sensor_a First sensor reading
   * @param sensor_b Second sensor reading
   * @return Fused result
   */
  T fuse(const T& sensor_a, const T& sensor_b);
  
  /** Reset fusion state */
  void reset();
  
  /** Check if initialized */
  bool isInitialized() const { return initialized_; }
  
private:
  bool initialized_;
  SensorFusionConfig config_;
  T previous_output_;
  bool has_previous_;
  
  /** Apply low-pass filter to output */
  T applyFilter(const T& current, const T& previous);
};

} // namespace arcos::algorithms::fusion

// Include implementation
#include "sensor_fusion_impl.hpp"

#endif // ARCOS_ALGORITHMS_FUSION_SENSOR_FUSION_HPP_
