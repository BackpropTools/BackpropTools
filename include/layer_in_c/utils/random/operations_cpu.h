#ifndef LAYER_IN_C_UTILS_RANDOM_OPERATIONS_CPU_H
#define LAYER_IN_C_UTILS_RANDOM_OPERATIONS_CPU_H

#include <random>

namespace layer_in_c::utils::random{
    using default_engine = std::default_random_engine;

    template<typename T, typename RNG>
    T uniform_int_distribution(T low, T high, RNG& rng){
        return std::uniform_int_distribution<T>(low, high)(rng);
    }
    template<typename T, typename RNG>
    T uniform_real_distribution(T low, T high, RNG& rng){
        return std::uniform_real_distribution<T>(low, high)(rng);
    }
//    template<typename T, typename RNG>
//    const std::normal_distribution<T> standard_normal_distribution(0, 1);
    template<typename T, typename RNG>
    T normal_distribution(T mean, T std, RNG& rng){
//        return standard_normal_distribution<T, RNG>(rng) * std + mean;
        return std::normal_distribution<T>(mean, std)(rng);
    }
}

#endif