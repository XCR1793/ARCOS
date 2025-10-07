/*****************************************************************
 * File:      sensor_fusion_impl.hpp
 * Category:  algorithms/fusion/sensor_fusion
 * Author:    XCR1793 (Feather Forge)
 * 
 * Purpose:
 *    Implementation of generic sensor fusion algorithm template.
 *****************************************************************/

#ifndef ARCOS_ALGORITHMS_FUSION_SENSOR_FUSION_IMPL_HPP_
#define ARCOS_ALGORITHMS_FUSION_SENSOR_FUSION_IMPL_HPP_

namespace arcos::algorithms::fusion{

template<typename T>
SensorFusion<T>::SensorFusion()
  : initialized_(false)
  , has_previous_(false)
{
}

template<typename T>
SensorFusion<T>::~SensorFusion(){
}

template<typename T>
bool SensorFusion<T>::init(const SensorFusionConfig& config){
  config_ = config;
  has_previous_ = false;
  initialized_ = true;
  return true;
}

template<typename T>
T SensorFusion<T>::fuse(const T& sensor_a, const T& sensor_b){
  if(!initialized_){
    return T{};
  }
  
  // Weighted average
  T result = sensor_a * config_.weight_sensor_a + 
             sensor_b * config_.weight_sensor_b;
  
  // Apply filtering if enabled
  if(config_.enable_filtering && has_previous_){
    result = applyFilter(result, previous_output_);
  }
  
  previous_output_ = result;
  has_previous_ = true;
  
  return result;
}

template<typename T>
void SensorFusion<T>::reset(){
  has_previous_ = false;
}

template<typename T>
T SensorFusion<T>::applyFilter(const T& current, const T& previous){
  // Simple low-pass filter: output = alpha * current + (1 - alpha) * previous
  return current * config_.filter_alpha + 
         previous * (1.0f - config_.filter_alpha);
}

} // namespace arcos::algorithms::fusion

#endif // ARCOS_ALGORITHMS_FUSION_SENSOR_FUSION_IMPL_HPP_
