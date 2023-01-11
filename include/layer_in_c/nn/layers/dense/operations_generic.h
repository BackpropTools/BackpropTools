#ifndef LAYER_IN_C_NN_LAYERS_DENSE_OPERATIONS_GENERIC_H
#define LAYER_IN_C_NN_LAYERS_DENSE_OPERATIONS_GENERIC_H

#include <layer_in_c/nn/layers/dense/layer.h>
#include <layer_in_c/utils/generic/polyak.h>
#ifndef FUNCTION_PLACEMENT
#define FUNCTION_PLACEMENT
#endif

namespace layer_in_c{
    template<typename DEVICE, typename LS, typename RNG>
    FUNCTION_PLACEMENT void init_kaiming(nn::layers::dense::Layer<DEVICE, LS>& layer, RNG& rng) {
        typedef typename LS::T T;
        T negative_slope = math::sqrt(typename DEVICE::SPEC::MATH(), (T)5);
        T gain = math::sqrt(typename DEVICE::SPEC::MATH(), (T)2.0 / (1 + negative_slope * negative_slope));
        T fan = LS::INPUT_DIM;
        T std = gain / math::sqrt(typename DEVICE::SPEC::MATH(), fan);
        T weight_bound = math::sqrt(typename DEVICE::SPEC::MATH(), (T)3.0) * std;
        T bias_bound = 1/math::sqrt(typename DEVICE::SPEC::MATH(), (T)LS::INPUT_DIM);
        for(typename DEVICE::index_t i = 0; i < LS::OUTPUT_DIM; i++) {
            layer.biases[i] = random::uniform_real_distribution(typename DEVICE::SPEC::RANDOM(), -bias_bound, bias_bound, rng);
            for(typename DEVICE::index_t j = 0; j < LS::INPUT_DIM; j++) {
                layer.weights[i][j] = random::uniform_real_distribution(typename DEVICE::SPEC::RANDOM(), -weight_bound, weight_bound, rng);
            }
        }
    }
    // evaluating a layer does not change its state (like pre_activations and outputs). Before using backward, to fill the state, use the forward method instead
    template<typename DEVICE, typename T, typename SPEC>
    FUNCTION_PLACEMENT void evaluate(const nn::layers::dense::Layer<DEVICE, SPEC>& layer, const T input[SPEC::INPUT_DIM], T output[SPEC::OUTPUT_DIM]) {
        // Warning do not use the same buffer for input and output!
        for(typename DEVICE::index_t i = 0; i < SPEC::OUTPUT_DIM; i++) {
            output[i] = layer.biases[i];
            for(typename DEVICE::index_t j = 0; j < SPEC::INPUT_DIM; j++) {
                output[i] += layer.weights[i][j] * input[j];
            }
            output[i] = nn::activation_functions::activation<typename DEVICE::SPEC::MATH, T, SPEC::ACTIVATION_FUNCTION>(output[i]);
        }
    }

    template<typename DEVICE, typename T, typename SPEC>
    FUNCTION_PLACEMENT void forward(nn::layers::dense::LayerBackward<DEVICE, SPEC>& layer, const T input[SPEC::INPUT_DIM], T output[SPEC::OUTPUT_DIM]) {
        // Warning do not use the same buffer for input and output!
        for(typename DEVICE::index_t i = 0; i < SPEC::OUTPUT_DIM; i++) {
            layer.pre_activations[i] = layer.biases[i];
            for(typename DEVICE::index_t j = 0; j < SPEC::INPUT_DIM; j++) {
                layer.pre_activations[i] += layer.weights[i][j] * input[j];
            }
            output[i] = nn::activation_functions::activation<T, SPEC::ACTIVATION_FUNCTION>(layer.pre_activations[i]);
        }
    }

    template<typename DEVICE, typename T, typename SPEC>
    FUNCTION_PLACEMENT void forward(nn::layers::dense::LayerBackwardGradient<DEVICE, SPEC>& layer, const T input[SPEC::INPUT_DIM]) {
        // Warning do not use the same buffer for input and output!
        for(typename DEVICE::index_t i = 0; i < SPEC::OUTPUT_DIM; i++) {
            layer.pre_activations[i] = layer.biases[i];
            for(typename DEVICE::index_t j = 0; j < SPEC::INPUT_DIM; j++) {
                layer.pre_activations[i] += layer.weights[i][j] * input[j];
            }
            layer.output[i] = nn::activation_functions::activation<typename DEVICE::SPEC::MATH, T, SPEC::ACTIVATION_FUNCTION>(layer.pre_activations[i]);
        }
    }
    template<typename DEVICE, typename T, typename SPEC>
    [[deprecated("Calling forward with an output buffer on a layer requiring the gradient is not recommended. Consider using forward without an output buffer to avoid copies instead.")]]
    FUNCTION_PLACEMENT void forward(nn::layers::dense::LayerBackwardGradient<DEVICE, SPEC>& layer, const T input[SPEC::INPUT_DIM], T output[SPEC::OUTPUT_DIM]) {
        // compile time warning if used
        forward(layer, input);
        for(typename DEVICE::index_t i = 0; i < SPEC::OUTPUT_DIM; i++) {
            output[i] = layer.output[i];
        }
    }

    template<typename DEVICE, typename SPEC>
    FUNCTION_PLACEMENT void backward(nn::layers::dense::LayerBackward<DEVICE, SPEC>& layer, const typename SPEC::T d_output[SPEC::OUTPUT_DIM], typename SPEC::T d_input[SPEC::INPUT_DIM]) {
        // todo: create sparate function that does not set d_input (to save cost on backward pass for the first layer)
        for(typename DEVICE::index_t i = 0; i < SPEC::OUTPUT_DIM; i++) {
            typename SPEC::T d_pre_activation = nn::activation_functions::d_activation_d_x<typename SPEC::T, SPEC::ACTIVATION_FUNCTION>(layer.pre_activations[i]) * d_output[i];
            for(typename DEVICE::index_t j = 0; j < SPEC::INPUT_DIM; j++) {
                if(i == 0){
                    d_input[j] = 0;
                }
                d_input[j] += layer.weights[i][j] * d_pre_activation;
            }
        }
    }
    template<typename DEVICE, typename SPEC>
    FUNCTION_PLACEMENT void backward(nn::layers::dense::LayerBackward<DEVICE, SPEC>& layer, const typename SPEC::T input[SPEC::INPUT_DIM], const typename SPEC::T d_output[SPEC::OUTPUT_DIM], typename SPEC::T d_input[SPEC::INPUT_DIM]) {
        backward(layer, d_output, d_input);

    }

    template<typename DEVICE, typename LS>
    FUNCTION_PLACEMENT void backward(nn::layers::dense::LayerBackwardGradient<DEVICE, LS>& layer, const typename LS::T input[LS::INPUT_DIM], const typename LS::T d_output[LS::OUTPUT_DIM], typename LS::T d_input[LS::INPUT_DIM]) {
        // todo: create sparate function that does not set d_input (to save cost on backward pass for the first layer)
        // todo: think about storing gradient in column major order to avoid iterating over the minor dimension
        for(typename DEVICE::index_t i = 0; i < LS::OUTPUT_DIM; i++) {
            typename LS::T d_pre_activation = nn::activation_functions::d_activation_d_x<typename DEVICE::SPEC::MATH, typename LS::T, LS::ACTIVATION_FUNCTION>(layer.pre_activations[i]) * d_output[i];
            layer.d_biases[i] += d_pre_activation;
            for(typename DEVICE::index_t j = 0; j < LS::INPUT_DIM; j++) {
                if(i == 0){
                    d_input[j] = 0;
                }
                d_input[j] += layer.weights[i][j] * d_pre_activation;
                layer.d_weights[i][j] += d_pre_activation * input[j];
            }
        }
    }
    template<typename DEVICE, typename LS>
    FUNCTION_PLACEMENT void zero_gradient(nn::layers::dense::LayerBackwardGradient<DEVICE, LS>& layer) {
        for(typename DEVICE::index_t i = 0; i < LS::OUTPUT_DIM; i++) {
            layer.d_biases[i] = 0;
            for(typename DEVICE::index_t j = 0; j < LS::INPUT_DIM; j++) {
                layer.d_weights[i][j] = 0;
            }
        }
    }
    template<typename DEVICE, typename LS, typename PARAMETERS>
    FUNCTION_PLACEMENT void update_layer(nn::layers::dense::LayerBackwardSGD<DEVICE, LS, PARAMETERS>& layer){
        for(typename DEVICE::index_t i = 0; i < LS::OUTPUT_DIM; i++) {
            layer.biases[i] -= PARAMETERS::ALPHA * layer.d_biases[i];
            for(typename DEVICE::index_t j = 0; j < LS::INPUT_DIM; j++) {
                layer.weights[i][j] -= PARAMETERS::ALPHA * layer.d_weights[i][j];
            }
        }
    }

    template<typename DEVICE, typename LS, typename PARAMETERS>
    FUNCTION_PLACEMENT void reset_optimizer_state(nn::layers::dense::LayerBackwardAdam<DEVICE, LS, PARAMETERS>& layer) {
        for(typename DEVICE::index_t i = 0; i < LS::OUTPUT_DIM; i++) {
            layer.d_biases_first_order_moment [i] = 0;
            layer.d_biases_second_order_moment[i] = 0;
            for(typename DEVICE::index_t j = 0; j < LS::INPUT_DIM; j++) {
                layer.d_weights_first_order_moment [i][j] = 0;
                layer.d_weights_second_order_moment[i][j] = 0;
            }
        }
    }
    template<typename DEVICE, typename LS, typename PARAMETERS>
    FUNCTION_PLACEMENT void gradient_descent(nn::layers::dense::LayerBackwardAdam<DEVICE, LS, PARAMETERS>& layer, typename LS::T first_order_moment_bias_correction, typename LS::T second_order_moment_bias_correction){
        for(typename DEVICE::index_t i = 0; i < LS::OUTPUT_DIM; i++) {
            typename LS::T bias_update = PARAMETERS::ALPHA * first_order_moment_bias_correction * layer.d_biases_first_order_moment[i] / (math::sqrt(typename DEVICE::SPEC::MATH(), layer.d_biases_second_order_moment[i] * second_order_moment_bias_correction) + PARAMETERS::EPSILON);
            layer.biases[i] -= bias_update;
            for(typename DEVICE::index_t j = 0; j < LS::INPUT_DIM; j++) {
                typename LS::T weight_update = PARAMETERS::ALPHA * first_order_moment_bias_correction * layer.d_weights_first_order_moment[i][j] / (math::sqrt(typename DEVICE::SPEC::MATH(), layer.d_weights_second_order_moment[i][j] * second_order_moment_bias_correction) + PARAMETERS::EPSILON);
                layer.weights[i][j] -= weight_update;
            }
        }
    }

    template<typename DEVICE, typename LS, typename PARAMETERS>
    FUNCTION_PLACEMENT void update_layer(nn::layers::dense::LayerBackwardAdam<DEVICE, LS, PARAMETERS>& layer, typename LS::T first_order_moment_bias_correction, typename LS::T second_order_moment_bias_correction) {
        // todo remove template params (auto inference)
        utils::polyak::update_matrix<DEVICE, typename LS::T, LS::OUTPUT_DIM, LS::INPUT_DIM>(layer.device, layer.d_weights_first_order_moment, layer.d_weights, PARAMETERS::BETA_1);
        utils::polyak::update       <DEVICE, typename LS::T, LS::OUTPUT_DIM>               (layer.device, layer. d_biases_first_order_moment, layer.d_biases , PARAMETERS::BETA_1);

        utils::polyak::update_squared_matrix<DEVICE, typename LS::T, LS::OUTPUT_DIM, LS::INPUT_DIM>(layer.device, layer.d_weights_second_order_moment, layer.d_weights, PARAMETERS::BETA_2);
        utils::polyak::update_squared       <DEVICE, typename LS::T, LS::OUTPUT_DIM>               (layer.device, layer. d_biases_second_order_moment, layer.d_biases , PARAMETERS::BETA_2);

        gradient_descent(layer, first_order_moment_bias_correction, second_order_moment_bias_correction);
    }

    template<typename TARGET_DEVICE, typename SOURCE_DEVICE, typename TARGET_SPEC, typename SOURCE_SPEC>
    FUNCTION_PLACEMENT void copy(nn::layers::dense::Layer<TARGET_DEVICE, TARGET_SPEC>* target, const nn::layers::dense::Layer<SOURCE_DEVICE, SOURCE_SPEC>* source){
        static_assert(nn::layers::dense::check_spec_memory<TARGET_SPEC, SOURCE_SPEC>);
        for(typename TARGET_DEVICE::index_t i = 0; i < TARGET_SPEC::OUTPUT_DIM; i++) {
            target->biases[i] = source->biases[i];
            for(typename TARGET_DEVICE::index_t j = 0; j < TARGET_SPEC::INPUT_DIM; j++) {
                target->weights[i][j] = source->weights[i][j];
            }
        }
    }
    template<typename TARGET_DEVICE, typename SOURCE_DEVICE, typename TARGET_SPEC, typename SOURCE_SPEC>
    FUNCTION_PLACEMENT void copy(nn::layers::dense::Layer<TARGET_DEVICE, TARGET_SPEC>& target, const nn::layers::dense::Layer<SOURCE_DEVICE, SOURCE_SPEC>& source){
        static_assert(nn::layers::dense::check_spec_memory<TARGET_SPEC, SOURCE_SPEC>);
        copy(&target, &source);
    }
    template<typename TARGET_DEVICE, typename SOURCE_DEVICE, typename TARGET_SPEC, typename SOURCE_SPEC>
    FUNCTION_PLACEMENT void copy(nn::layers::dense::LayerBackward<TARGET_DEVICE, TARGET_SPEC>* target, const nn::layers::dense::LayerBackward<SOURCE_DEVICE, SOURCE_SPEC>* source){
        static_assert(nn::layers::dense::check_spec_memory<TARGET_SPEC, SOURCE_SPEC>);
        copy((nn::layers::dense::Layer<TARGET_DEVICE, TARGET_SPEC>*) target, (nn::layers::dense::Layer<TARGET_DEVICE, TARGET_SPEC>*) source);
        for(typename TARGET_DEVICE::index_t i = 0; i < TARGET_SPEC::OUTPUT_DIM; i++) {
            target->pre_activations[i] = source->pre_activations[i];
        }
    }
    template<typename TARGET_DEVICE, typename SOURCE_DEVICE, typename TARGET_SPEC, typename SOURCE_SPEC>
    FUNCTION_PLACEMENT void copy(nn::layers::dense::LayerBackward<TARGET_DEVICE, TARGET_SPEC>& target, const nn::layers::dense::LayerBackward<SOURCE_DEVICE, SOURCE_SPEC>& source){
        static_assert(nn::layers::dense::check_spec_memory<TARGET_SPEC, SOURCE_SPEC>);
        copy(&target, &source);
    }
    template<typename TARGET_DEVICE, typename SOURCE_DEVICE, typename TARGET_SPEC, typename SOURCE_SPEC>
    FUNCTION_PLACEMENT void copy(nn::layers::dense::LayerBackwardGradient<TARGET_DEVICE, TARGET_SPEC>* target, const nn::layers::dense::LayerBackwardGradient<SOURCE_DEVICE, SOURCE_SPEC>* source){
        static_assert(nn::layers::dense::check_spec_memory<TARGET_SPEC, SOURCE_SPEC>);
        copy((nn::layers::dense::LayerBackward<TARGET_DEVICE, TARGET_SPEC>*)target, (nn::layers::dense::LayerBackward<SOURCE_DEVICE, SOURCE_SPEC>*)source);
        for(typename TARGET_DEVICE::index_t i = 0; i < TARGET_SPEC::OUTPUT_DIM; i++) {
            target->d_biases[i] = source->d_biases[i];
            target->output[i] = source->output[i];
            for(typename TARGET_DEVICE::index_t j = 0; j < TARGET_SPEC::INPUT_DIM; j++) {
                target->d_weights[i][j] = source->d_weights[i][j];
            }
        }
    }
    template<typename TARGET_DEVICE, typename SOURCE_DEVICE, typename TARGET_SPEC, typename SOURCE_SPEC>
    FUNCTION_PLACEMENT void copy(nn::layers::dense::LayerBackwardGradient<TARGET_DEVICE, TARGET_SPEC>& target, const nn::layers::dense::LayerBackwardGradient<SOURCE_DEVICE, SOURCE_SPEC>& source){
        static_assert(nn::layers::dense::check_spec_memory<TARGET_SPEC, SOURCE_SPEC>);
        copy(&target, &source);
    }

    template<typename TARGET_DEVICE, typename SOURCE_DEVICE, typename TARGET_SPEC, typename SOURCE_SPEC, typename TARGET_PARAMETERS, typename SOURCE_PARAMETERS>
    FUNCTION_PLACEMENT void copy(nn::layers::dense::LayerBackwardAdam<TARGET_DEVICE, TARGET_SPEC, TARGET_PARAMETERS>* target, const nn::layers::dense::LayerBackwardAdam<SOURCE_DEVICE, SOURCE_SPEC, SOURCE_PARAMETERS>* source){
        static_assert(nn::layers::dense::check_spec_memory<TARGET_SPEC, SOURCE_SPEC>);
        copy((nn::layers::dense::LayerBackwardGradient<TARGET_DEVICE, TARGET_SPEC>*)target, (nn::layers::dense::LayerBackwardGradient<SOURCE_DEVICE, SOURCE_SPEC>*)source);
        for(typename TARGET_DEVICE::index_t i = 0; i < TARGET_SPEC::OUTPUT_DIM; i++) {
            target->d_biases_first_order_moment [i] = source->d_biases_first_order_moment [i];
            target->d_biases_second_order_moment[i] = source->d_biases_second_order_moment[i];
            for(typename TARGET_DEVICE::index_t j = 0; j < TARGET_SPEC::INPUT_DIM; j++) {
                target->d_weights_first_order_moment [i][j] = source->d_weights_first_order_moment [i][j];
                target->d_weights_second_order_moment[i][j] = source->d_weights_second_order_moment[i][j];
            }
        }
    }
    template<typename TARGET_DEVICE, typename SOURCE_DEVICE, typename TARGET_SPEC, typename SOURCE_SPEC, typename TARGET_PARAMETERS, typename SOURCE_PARAMETERS>
    FUNCTION_PLACEMENT void copy(nn::layers::dense::LayerBackwardAdam<TARGET_DEVICE, TARGET_SPEC, TARGET_PARAMETERS>& target, const nn::layers::dense::LayerBackwardAdam<SOURCE_DEVICE, SOURCE_SPEC, SOURCE_PARAMETERS>& source){
        static_assert(nn::layers::dense::check_spec_memory<TARGET_SPEC, SOURCE_SPEC>);
        copy(&target, &source);
    }

    namespace nn::layers::dense::helper{
        template <typename T, int N_ROWS, int N_COLS>
        T abs_diff_matrix(const T A[N_ROWS][N_COLS], const T B[N_ROWS][N_COLS]) {
            T acc = 0;
            for (int i = 0; i < N_ROWS; i++){
                for (int j = 0; j < N_COLS; j++){
                    acc += math::abs(A[i][j] - B[i][j]);
                }
            }
            return acc;
        }

        template <typename T, int N_ROWS>
        T abs_diff_vector(const T A[N_ROWS], const T B[N_ROWS]) {
            T acc = 0;
            for (int i = 0; i < N_ROWS; i++){
                acc += math::abs(A[i] - B[i]);
            }
            return acc;
        }
    }
    template <typename DEVICE_1, typename DEVICE_2, typename SPEC_1, typename SPEC_2>
    typename SPEC_1::T abs_diff(const layer_in_c::nn::layers::dense::Layer<DEVICE_1, SPEC_1>* l1, const layer_in_c::nn::layers::dense::Layer<DEVICE_2, SPEC_2>* l2) {
        static_assert(nn::layers::dense::check_spec_memory<SPEC_1, SPEC_2>);
        using T = typename SPEC_1::T;
        T acc = 0;
        acc += nn::layers::dense::helper::abs_diff_matrix<T, SPEC_1::OUTPUT_DIM, SPEC_1::INPUT_DIM>(l1->weights, l2->weights);
        acc += nn::layers::dense::helper::abs_diff_vector<T, SPEC_1::OUTPUT_DIM>(l1->biases, l2->biases);
        return acc;
    }
    template <typename DEVICE_1, typename DEVICE_2, typename SPEC_1, typename SPEC_2>
    typename SPEC_1::T abs_diff(const layer_in_c::nn::layers::dense::Layer<DEVICE_1, SPEC_1>& l1, const layer_in_c::nn::layers::dense::Layer<DEVICE_2, SPEC_2>& l2) {
        static_assert(nn::layers::dense::check_spec_memory<SPEC_1, SPEC_2>);
        return abs_diff(&l1, &l2);
    }
    template <typename DEVICE_1, typename DEVICE_2, typename SPEC_1, typename SPEC_2>
    typename SPEC_1::T abs_diff(const layer_in_c::nn::layers::dense::LayerBackward<DEVICE_1, SPEC_1>* l1, const layer_in_c::nn::layers::dense::LayerBackward<DEVICE_2, SPEC_2>* l2) {
        static_assert(nn::layers::dense::check_spec_memory<SPEC_1, SPEC_2>);
        using T = typename SPEC_1::T;
        T acc = abs_diff((layer_in_c::nn::layers::dense::Layer<DEVICE_1, SPEC_1>*) l1, (layer_in_c::nn::layers::dense::Layer<DEVICE_2, SPEC_2>*) l2);
        acc += nn::layers::dense::helper::abs_diff_vector<T, SPEC_1::OUTPUT_DIM>(l1->pre_activations, l2->pre_activations);
        return acc;
    }
    template <typename DEVICE_1, typename DEVICE_2, typename SPEC_1, typename SPEC_2>
    typename SPEC_1::T abs_diff(const layer_in_c::nn::layers::dense::LayerBackward<DEVICE_1, SPEC_1>& l1, const layer_in_c::nn::layers::dense::LayerBackward<DEVICE_2, SPEC_2>& l2) {
        static_assert(nn::layers::dense::check_spec_memory<SPEC_1, SPEC_2>);
        return abs_diff(&l1, &l2);
    }
    template <typename DEVICE_1, typename DEVICE_2, typename SPEC_1, typename SPEC_2>
    typename SPEC_1::T abs_diff(const layer_in_c::nn::layers::dense::LayerBackwardGradient<DEVICE_1, SPEC_1>* l1, const layer_in_c::nn::layers::dense::LayerBackwardGradient<DEVICE_2, SPEC_2>* l2) {
        static_assert(nn::layers::dense::check_spec_memory<SPEC_1, SPEC_2>);
        using T = typename SPEC_1::T;
        T acc = abs_diff((layer_in_c::nn::layers::dense::LayerBackward<DEVICE_1, SPEC_1>*) l1, (layer_in_c::nn::layers::dense::LayerBackward<DEVICE_2, SPEC_2>*) l2);
        acc += nn::layers::dense::helper::abs_diff_vector<T, SPEC_1::OUTPUT_DIM>(l1->output, l2->output);
        acc += nn::layers::dense::helper::abs_diff_matrix<T, SPEC_1::OUTPUT_DIM, SPEC_1::INPUT_DIM>(l1->d_weights, l2->d_weights);
        acc += nn::layers::dense::helper::abs_diff_vector<T, SPEC_1::OUTPUT_DIM>(l1->d_biases, l2->d_biases);
        return acc;
    }
    template <typename DEVICE_1, typename DEVICE_2, typename SPEC_1, typename SPEC_2>
    typename SPEC_1::T abs_diff(const layer_in_c::nn::layers::dense::LayerBackwardGradient<DEVICE_1, SPEC_1>& l1, const layer_in_c::nn::layers::dense::LayerBackwardGradient<DEVICE_2, SPEC_2>& l2) {
        static_assert(nn::layers::dense::check_spec_memory<SPEC_1, SPEC_2>);
        return abs_diff(&l1, &l2);
    }
    template <typename DEVICE_1, typename DEVICE_2, typename SPEC_1, typename SPEC_2, typename PARAMETERS_1, typename PARAMETERS_2>
    typename SPEC_1::T abs_diff(const layer_in_c::nn::layers::dense::LayerBackwardAdam<DEVICE_1, SPEC_1, PARAMETERS_1>* l1, const layer_in_c::nn::layers::dense::LayerBackwardAdam<DEVICE_2, SPEC_2, PARAMETERS_2>* l2) {
        static_assert(nn::layers::dense::check_spec_memory<SPEC_1, SPEC_2>);
        using T = typename SPEC_1::T;
        T acc = abs_diff((layer_in_c::nn::layers::dense::LayerBackwardGradient<DEVICE_1, SPEC_1>*) l1, (layer_in_c::nn::layers::dense::LayerBackwardGradient<DEVICE_2, SPEC_2>*) l2);
        acc += nn::layers::dense::helper::abs_diff_matrix<T, SPEC_1::OUTPUT_DIM, SPEC_1::INPUT_DIM>(l1->d_weights_first_order_moment, l2->d_weights_first_order_moment);
        acc += nn::layers::dense::helper::abs_diff_matrix<T, SPEC_1::OUTPUT_DIM, SPEC_1::INPUT_DIM>(l1->d_weights_second_order_moment, l2->d_weights_second_order_moment);
        acc += nn::layers::dense::helper::abs_diff_vector<T, SPEC_1::OUTPUT_DIM>(l1->d_biases_first_order_moment, l2->d_biases_first_order_moment);
        acc += nn::layers::dense::helper::abs_diff_vector<T, SPEC_1::OUTPUT_DIM>(l1->d_biases_second_order_moment, l2->d_biases_second_order_moment);
        return acc;
    }
    template <typename DEVICE_1, typename DEVICE_2, typename SPEC_1, typename SPEC_2, typename PARAMETERS_1, typename PARAMETERS_2>
    typename SPEC_1::T abs_diff(const layer_in_c::nn::layers::dense::LayerBackwardAdam<DEVICE_1, SPEC_1, PARAMETERS_1>& l1, const layer_in_c::nn::layers::dense::LayerBackwardAdam<DEVICE_2, SPEC_2, PARAMETERS_2>& l2) {
        static_assert(nn::layers::dense::check_spec_memory<SPEC_1, SPEC_2>);
        return abs_diff(&l1, &l2);
    }
    template <typename DEVICE, typename SPEC>
    void reset_forward_state(layer_in_c::nn::layers::dense::LayerBackward<DEVICE, SPEC>* l) {
        for(typename DEVICE::index_t i = 0; i < SPEC::OUTPUT_DIM; i++){
            l->pre_activations[i] = 0;
        }
    }
    template <typename DEVICE, typename SPEC>
    void reset_forward_state(layer_in_c::nn::layers::dense::LayerBackward<DEVICE, SPEC>& l) {
        reset_forward_state((layer_in_c::nn::layers::dense::Layer<DEVICE, SPEC>*) l);
    }
    template <typename DEVICE, typename SPEC>
    void reset_forward_state(layer_in_c::nn::layers::dense::LayerBackwardGradient<DEVICE, SPEC>* l) {
        reset_forward_state((layer_in_c::nn::layers::dense::LayerBackward<DEVICE, SPEC>*) l);
        for(typename DEVICE::index_t i = 0; i < SPEC::OUTPUT_DIM; i++){
            l->output[i] = 0;
        }
    }
    template <typename DEVICE, typename SPEC>
    void reset_forward_state(layer_in_c::nn::layers::dense::LayerBackwardGradient<DEVICE, SPEC>& l) {
        reset_forward_state(&l);
    }
}

#endif