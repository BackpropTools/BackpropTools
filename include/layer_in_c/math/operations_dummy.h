#ifndef LAYER_IN_C_CPU_GENERIC_MATH_H
#define LAYER_IN_C_CPU_GENERIC_MATH_H

#include <layer_in_c/devices/dummy.h>

#include "operations_generic.h"

namespace layer_in_c{
    using index_t = unsigned;
}


namespace layer_in_c::math {

    template<typename T>
    T sqrt(const devices::math::Dummy&, const T x) {
        return x;
    }
    template<typename T>
    T tanh(const devices::math::Dummy&, const T x) {
        return x;
    }
    template<typename T>
    T exp(const devices::math::Dummy&, const T x) {
        return x;
    }
    template<typename T>
    T sin(const devices::math::Dummy&, const T x) {
        return x;
    }
    template<typename T>
    T cos(const devices::math::Dummy&, const T x) {
        return x;
    }
    template<typename TX, typename TY>
    auto pow(const devices::math::Dummy&, const TX x, const TY y) {
        return 1;
    }
    template<typename T>
    auto log(const devices::math::Dummy&, const T x) {
        return 0;
    }
    template<typename T>
    auto floor(const devices::math::Dummy&, const T x) {
        return (int)x;
    }
}
#endif
