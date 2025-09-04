#ifndef ARCOS_ABSTRACTION_CORE_HAL_GPIO_HPP_
#define ARCOS_ABSTRACTION_CORE_HAL_GPIO_HPP_

#if defined(__has_include)
  #if __has_include(<type_traits>)
    #include <type_traits>
    #define ARCOS_HAS_TYPE_TRAITS 1
  #else
    #define ARCOS_HAS_TYPE_TRAITS 0
  #endif
#else
  #define ARCOS_HAS_TYPE_TRAITS 0
#endif

enum class PinState{Reset, Set};
enum class PinMode {Input, Output};

template <typename PlatformGpio, typename GpioAddressBank = int>
class HAL_GPIO{
  #if ARCOS_HAS_TYPE_TRAITS
    static_assert(std::is_integral<GpioAddressBank>::value, "GpioAddressBank must be an integral type");
  #endif

  public:
    HAL_GPIO(GpioAddressBank pin, PinMode mode = PinMode::Input) : impl(pin, mode){}

    void write(PinState state){impl.write(state);}

    void toggle(){impl.toggle();}

    PinState read(){return impl.read();}

  private:
    PlatformGpio impl;
};

#endif // ARCOS_ABSTRACTION_CORE_HAL_GPIO_HPP_