#ifndef BACKPROP_TOOLS_UTILS_RANDOM_OPERATIONS_DUMMY_H
#define BACKPROP_TOOLS_UTILS_RANDOM_OPERATIONS_DUMMY_H

#include <backprop_tools/devices/dummy.h>
#include <backprop_tools/utils/generic/typing.h>

namespace backprop_tools::random{
    auto default_engine(const devices::random::Dummy& dev){
        return devices::random::Dummy::State(0);
    }

    template<typename T, typename RNG>
    T uniform_int_distribution(const devices::random::Dummy& dev, T low, T high, RNG& rng){
//        static_assert(utils::typing::is_same_v<T, typename DEVICE::index_t>);
//        static_assert(utils::typing::is_same_v<RNG, typename DEVICE::index_t>);
//        // https://en.wikipedia.org/wiki/Xorshift
//        rng ^= (rng << 21);
//        rng ^= (rng >> 35);
//        rng ^= (rng << 4);
//        return rng % (high-low) + low;
        return low;
    }
    template<typename T, typename RNG>
    T uniform_real_distribution(const devices::random::Dummy& dev, T low, T high, RNG& rng){
//        static_assert(utils::typing::is_same_v<RNG, typename DEVICE::index_t>);
//        T r = (T)uniform_int_distribution<typename DEVICE::index_t, typename DEVICE::index_t>((typename DEVICE::index_t)0, (typename DEVICE::index_t)1<<31, rng) / (T)(1<<31) - 1;
//        return r * (high-low) + low;
        return low;
    }
    template<typename T, typename RNG>
    T normal_distribution(const devices::random::Dummy& dev, T mean, T std, RNG& rng){
//        static_assert(utils::typing::is_same_v<RNG, typename DEVICE::index_t>);
//        //
//        T x, y, r, res;
//        x = 0;
//        y = 0;
//        r = 0;
//        res = 0;
//
//        do {
//            x = (T)2.0 * (T)uniform_int_distribution<typename DEVICE::index_t, typename DEVICE::index_t>(0, 1<<31, rng) / (T)(1<<31) - 1;
//            y = (T)2.0 * (T)uniform_int_distribution<typename DEVICE::index_t, typename DEVICE::index_t>(0, 1<<31, rng) / (T)(1<<31) - 1;
//            r = x * x + y * y;
//        } while (r == 0.0 || r > 1.0);
//
//        T d = math::sqrt(-2.0 * math::log(r) / r);
//
//        T n1 = x * d;
//        T n2 = y * d;
//
//        res = n1 * std + mean;
//
//        return res;
        return mean;
    }
}

#endif
