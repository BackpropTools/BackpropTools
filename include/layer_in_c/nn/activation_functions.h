#ifndef LAYER_IN_C_NN_ACTIVATION_FUNCTIONS
#define LAYER_IN_C_NN_ACTIVATION_FUNCTIONS
#include "math.h"
#include <cmath>
namespace layer_in_c::nn::activation_functions {
    enum ActivationFunction{
        RELU,
        GELU,
        GELU_SQUARE,
        TANH,
        LINEAR,
    };

    template<typename T, ActivationFunction F>
    T activation(T x){
        if (F == RELU){
            return x > 0 ? x : 0;
        }
        else if (F == GELU){
            constexpr T a = M_2_SQRTPI * M_SQRT1_2 * (T)0.5;
            return (T)0.5 * (x + x * std::tanh(a * ((T)0.044715f * x * x * x + x)));
        }
        else if (F == GELU_SQUARE){
            constexpr T a = M_2_SQRTPI * M_SQRT1_2 * (T)0.5;
            return (T)0.5 * (x + x * std::tanh(a * ((T)0.044715f * x * x + x)));
        }
        else if (F == TANH){
            return std::tanh(x);
        }
        else if (F == LINEAR){
            return x;
        }
        else{
            return 0;
        }
    }

    template<typename T, ActivationFunction F>
    T d_activation_d_x(T x){
        if (F == RELU){
            return x > 0 ? 1 : 0;
        }
        else if (F == GELU){
            constexpr T a = M_2_SQRTPI * M_SQRT1_2 * (T)0.5;
            constexpr T b = 0.044715f;
            T tanh_term = std::tanh(a * (b * x * x * x + x));
            return (T)0.5*(1 + tanh_term) + (T)0.5 * x * (1 - tanh_term * tanh_term) * a * (3 * b * x * x + 1);
        }
        else if (F == GELU_SQUARE){
            constexpr T a = M_2_SQRTPI/(T)2 * M_SQRT2;
            constexpr T b = 0.044715f;
            T tanh_term = std::tanh(a * (b * x * x * x + x));
            return (T)0.5*(1 + tanh_term) + (T)0.5 * x * (1 - tanh_term * tanh_term) * a * (3 * b * x * x + 1);
        }
        else if (F == TANH){
            T a = std::tanh(x);
            return 1 - a * a;
        }
        else if (F == LINEAR){
            return 1;
        }
        else{
            return 0;
        }
    }
}


#endif