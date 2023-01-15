#ifndef LAYER_IN_C_NN_LAYERS_DENSE_OPERATIONS_GENERIC_H
#define LAYER_IN_C_NN_LAYERS_DENSE_OPERATIONS_GENERIC_H

#include <layer_in_c/containers.h>
#include <layer_in_c/nn/layers/dense/layer.h>
#include <layer_in_c/utils/generic/polyak.h>
#ifndef FUNCTION_PLACEMENT
#define FUNCTION_PLACEMENT
#endif

namespace layer_in_c{
    template<typename DEVICE, typename SPEC>
    FUNCTION_PLACEMENT void malloc(DEVICE& device, nn::layers::dense::Layer<SPEC>& layer) {
        malloc(device, layer.weights);
        malloc(device, layer.biases);
    }
    template<typename DEVICE, typename SPEC>
    FUNCTION_PLACEMENT void free(DEVICE& device, nn::layers::dense::Layer<SPEC>& layer) {
        free(device, layer.weights);
        free(device, layer.biases);
    }
    template<typename DEVICE, typename SPEC>
    FUNCTION_PLACEMENT void malloc(DEVICE& device, nn::layers::dense::LayerBackward<SPEC>& layer) {
        malloc(device, (nn::layers::dense::Layer<SPEC>&) layer);
        malloc(device, layer.pre_activations);
    }
    template<typename DEVICE, typename SPEC>
    FUNCTION_PLACEMENT void free(DEVICE& device, nn::layers::dense::LayerBackward<SPEC>& layer) {
        free(device, (nn::layers::dense::Layer<SPEC>&) layer);
        free(device, layer.pre_activations);
    }
    template<typename DEVICE, typename SPEC>
    FUNCTION_PLACEMENT void malloc(DEVICE& device, nn::layers::dense::LayerBackwardGradient<SPEC>& layer) {
        malloc(device, (nn::layers::dense::LayerBackward<SPEC>&) layer);
        malloc(device, layer.output);
        malloc(device, layer.d_biases);
        malloc(device, layer.d_weights);
    }
    template<typename DEVICE, typename SPEC>
    FUNCTION_PLACEMENT void free(DEVICE& device, nn::layers::dense::LayerBackwardGradient<SPEC>& layer) {
        free(device, (nn::layers::dense::LayerBackward<SPEC>&) layer);
        free(device, layer.output);
        free(device, layer.d_biases);
        free(device, layer.d_weights);
    }
    template<typename DEVICE, typename SPEC, typename PARAMETERS>
    FUNCTION_PLACEMENT void malloc(DEVICE& device, nn::layers::dense::LayerBackwardAdam<SPEC, PARAMETERS>& layer) {
        malloc(device, (nn::layers::dense::LayerBackwardGradient<SPEC>&) layer);
        malloc(device, layer.d_weights_first_order_moment);
        malloc(device, layer.d_weights_second_order_moment);
        malloc(device, layer.d_biases_first_order_moment);
        malloc(device, layer.d_biases_second_order_moment);
    }
    template<typename DEVICE, typename SPEC, typename PARAMETERS>
    FUNCTION_PLACEMENT void free(DEVICE& device, nn::layers::dense::LayerBackwardAdam<SPEC, PARAMETERS>& layer) {
        free(device, (nn::layers::dense::LayerBackwardGradient<SPEC>&) layer);
        free(device, layer.d_weights_first_order_moment);
        free(device, layer.d_weights_second_order_moment);
        free(device, layer.d_biases_first_order_moment);
        free(device, layer.d_biases_second_order_moment);
    }

    template<typename DEVICE, typename SPEC, typename RNG>
    FUNCTION_PLACEMENT void init_kaiming(DEVICE& device, nn::layers::dense::Layer<SPEC>& layer, RNG& rng) {
        using T = typename SPEC::T;
        using TI = typename SPEC::TI;
        T negative_slope = math::sqrt(typename DEVICE::SPEC::MATH(), (T)5);
        T gain = math::sqrt(typename DEVICE::SPEC::MATH(), (T)2.0 / (1 + negative_slope * negative_slope));
        T fan = SPEC::INPUT_DIM;
        T std = gain / math::sqrt(typename DEVICE::SPEC::MATH(), fan);
        T weight_bound = math::sqrt(typename DEVICE::SPEC::MATH(), (T)3.0) * std;
        T bias_bound = 1/math::sqrt(typename DEVICE::SPEC::MATH(), (T)SPEC::INPUT_DIM);
        for(TI i = 0; i < SPEC::OUTPUT_DIM; i++) {
            layer.biases.data[i] = random::uniform_real_distribution(typename DEVICE::SPEC::RANDOM(), -bias_bound, bias_bound, rng);
            for(TI j = 0; j < SPEC::INPUT_DIM; j++) {
                layer.weights.data[i * SPEC::INPUT_DIM + j] = random::uniform_real_distribution(typename DEVICE::SPEC::RANDOM(), -weight_bound, weight_bound, rng);
            }
        }
    }

    namespace nn::layers::dense{
        template <typename LAYER_SPEC, typename INPUT_SPEC, typename OUTPUT_SPEC>
        constexpr bool check_input_output =
                INPUT_SPEC::COLS == LAYER_SPEC::INPUT_DIM &&
                INPUT_SPEC::ROWS == OUTPUT_SPEC::ROWS &&
                OUTPUT_SPEC::COLS == LAYER_SPEC::OUTPUT_DIM &&
                (!LAYER_SPEC::ENFORCE_FLOATING_POINT_TYPE || ( utils::typing::is_same_v<typename LAYER_SPEC::T, typename INPUT_SPEC::T> && utils::typing::is_same_v<typename INPUT_SPEC::T, typename OUTPUT_SPEC::T>));
    }

    template<typename DEVICE, typename LAYER_SPEC, typename INPUT_SPEC, typename OUTPUT_SPEC>
    FUNCTION_PLACEMENT void evaluate(DEVICE& device, const nn::layers::dense::Layer<LAYER_SPEC>& layer, const Matrix<INPUT_SPEC>& input, Matrix<OUTPUT_SPEC>& output) {
        static_assert(nn::layers::dense::check_input_output<LAYER_SPEC, INPUT_SPEC, OUTPUT_SPEC>);
        // Warning do not use the same buffer for input and output!
        constexpr auto BATCH_SIZE = INPUT_SPEC::ROWS;
        using TI = typename DEVICE::index_t;
        for(TI batch_i=0; batch_i < BATCH_SIZE; batch_i++){
            for(TI output_i = 0; output_i < LAYER_SPEC::OUTPUT_DIM; output_i++) {
                TI output_index = batch_i * LAYER_SPEC::OUTPUT_DIM + output_i;
                output.data[output_index] = layer.biases.data[output_i];
                for(TI input_i = 0; input_i < LAYER_SPEC::INPUT_DIM; input_i++) {
                    TI input_index = batch_i * LAYER_SPEC::INPUT_DIM + input_i;
                    output.data[output_index] += layer.weights.data[output_i * LAYER_SPEC::INPUT_DIM + input_i] * input.data[input_index];
                }
                output.data[output_index] = activation<typename DEVICE::SPEC::MATH, typename LAYER_SPEC::T, LAYER_SPEC::ACTIVATION_FUNCTION>(output.data[output_index]);
            }
        }
    }

    template<typename DEVICE, typename LAYER_SPEC, typename INPUT_SPEC, typename OUTPUT_SPEC>
    FUNCTION_PLACEMENT void forward(DEVICE& device, nn::layers::dense::LayerBackward<LAYER_SPEC>& layer, const Matrix<INPUT_SPEC>& input, Matrix<OUTPUT_SPEC>& output) {
        // Warning do not use the same buffer for input and output!
        static_assert(nn::layers::dense::check_input_output<LAYER_SPEC, INPUT_SPEC, OUTPUT_SPEC>);
        constexpr auto BATCH_SIZE = INPUT_SPEC::ROWS;
        using T = typename LAYER_SPEC::T;
        using TI = typename DEVICE::index_t;

        for(TI batch_i=0; batch_i < BATCH_SIZE; batch_i++){
            for(TI i = 0; i < LAYER_SPEC::OUTPUT_DIM; i++) {
                TI output_index = batch_i * LAYER_SPEC::OUTPUT_DIM + i;
                layer.pre_activations.data[output_index] = layer.biases.data[i];
                for(TI j = 0; j < LAYER_SPEC::INPUT_DIM; j++) {
                    TI input_index = batch_i * LAYER_SPEC::INPUT_DIM + j;
                    layer.pre_activations.data[output_index] += layer.weights.data[i * LAYER_SPEC::INPUT_DIM + j] * input.data[input_index];
                }
                output.data[output_index] = activation<typename DEVICE::SPEC::MATH, T, LAYER_SPEC::ACTIVATION_FUNCTION>(layer.pre_activations.data[output_index]);
            }
        }
    }

    template<typename DEVICE, typename LAYER_SPEC, typename INPUT_SPEC>
    FUNCTION_PLACEMENT void forward(DEVICE& device, nn::layers::dense::LayerBackwardGradient<LAYER_SPEC>& layer, const Matrix<INPUT_SPEC>& input) {
        static_assert(nn::layers::dense::check_input_output<LAYER_SPEC, INPUT_SPEC, typename decltype(layer.output)::SPEC>);
        forward(device, (nn::layers::dense::LayerBackward<LAYER_SPEC>&)layer, input, layer.output);
    }
    template<typename DEVICE, typename LAYER_SPEC, typename INPUT_SPEC, typename OUTPUT_SPEC>
    FUNCTION_PLACEMENT void forward(DEVICE& device, nn::layers::dense::LayerBackwardGradient<LAYER_SPEC>& layer, const Matrix<INPUT_SPEC>& input, Matrix<OUTPUT_SPEC>& output) {
        static_assert(nn::layers::dense::check_input_output<LAYER_SPEC, INPUT_SPEC, OUTPUT_SPEC>);
        // compile time warning if used
        forward(device, layer, input);
        copy(output, layer.output);
    }

    template<typename DEVICE, typename LAYER_SPEC, typename D_OUTPUT_SPEC, typename D_INPUT_SPEC>
    FUNCTION_PLACEMENT void backward(DEVICE& device, nn::layers::dense::LayerBackward<LAYER_SPEC>& layer, const Matrix<D_OUTPUT_SPEC>& d_output, Matrix<D_INPUT_SPEC>& d_input) {
        static_assert(nn::layers::dense::check_input_output<LAYER_SPEC, D_INPUT_SPEC, D_OUTPUT_SPEC>);
        // todo: create sparate function that does not set d_input (to save cost on backward pass for the first layer)
        using SPEC = LAYER_SPEC;
        constexpr auto BATCH_SIZE = D_INPUT_SPEC::ROWS;
        using T = typename LAYER_SPEC::T;
        using TI = typename DEVICE::index_t;
        for(TI batch_i=0; batch_i < BATCH_SIZE; batch_i++){
            for(TI output_i = 0; output_i < SPEC::OUTPUT_DIM; output_i++) {
                TI output_index = batch_i * LAYER_SPEC::OUTPUT_DIM + output_i;
                typename SPEC::T d_pre_activation = d_activation_d_x<typename SPEC::T, SPEC::ACTIVATION_FUNCTION>(layer.pre_activations.data[output_index]) * d_output[output_index];
                for(TI input_j = 0; input_j < SPEC::INPUT_DIM; input_j++) {
                    TI input_index = batch_i * LAYER_SPEC::INPUT_DIM + input_j;
                    if(output_i == 0){
                        d_input.data[input_index] = 0;
                    }
                    d_input[input_index] += layer.weights.data[output_i * SPEC::INPUT_DIM + input_j] * d_pre_activation;
                }
            }
        }
    }
    template<typename DEVICE, typename LAYER_SPEC, typename INPUT_SPEC, typename D_OUTPUT_SPEC, typename D_INPUT_SPEC>
    FUNCTION_PLACEMENT void backward(DEVICE& device, nn::layers::dense::LayerBackward<LAYER_SPEC>& layer, const Matrix<INPUT_SPEC>& input, const Matrix<D_OUTPUT_SPEC>& d_output, Matrix<D_INPUT_SPEC>& d_input) {
        static_assert(nn::layers::dense::check_input_output<LAYER_SPEC, D_INPUT_SPEC, D_OUTPUT_SPEC>);
        static_assert(nn::layers::dense::check_input_output<LAYER_SPEC, INPUT_SPEC, D_OUTPUT_SPEC>);
        backward(layer, d_output, d_input);
    }

    template<typename DEVICE, typename LAYER_SPEC, typename INPUT_SPEC, typename D_OUTPUT_SPEC, typename D_INPUT_SPEC>
    FUNCTION_PLACEMENT void backward(DEVICE& device, nn::layers::dense::LayerBackwardGradient<LAYER_SPEC>& layer, const Matrix<INPUT_SPEC>& input, const Matrix<D_OUTPUT_SPEC>& d_output, Matrix<D_INPUT_SPEC>& d_input) {
        // todo: create sparate function that does not set d_input (to save cost on backward pass for the first layer)
        // todo: think about storing gradient in column major order to avoid iterating over the minor dimension
        static_assert(nn::layers::dense::check_input_output<LAYER_SPEC, D_INPUT_SPEC, D_OUTPUT_SPEC>);
        static_assert(nn::layers::dense::check_input_output<LAYER_SPEC, INPUT_SPEC, D_OUTPUT_SPEC>);
        constexpr auto INPUT_DIM = LAYER_SPEC::INPUT_DIM;
        constexpr auto OUTPUT_DIM = LAYER_SPEC::OUTPUT_DIM;
        constexpr auto BATCH_SIZE = D_INPUT_SPEC::ROWS;
        using T = typename LAYER_SPEC::T;
        using TI = typename DEVICE::index_t;

        for(TI batch_i=0; batch_i < BATCH_SIZE; batch_i++){
            for(TI output_i = 0; output_i < OUTPUT_DIM; output_i++) {
                TI output_index = batch_i * LAYER_SPEC::OUTPUT_DIM + output_i;
                T d_pre_activation = d_activation_d_x<typename DEVICE::SPEC::MATH, T, LAYER_SPEC::ACTIVATION_FUNCTION>(layer.pre_activations.data[output_index]) * d_output.data[output_index];
                layer.d_biases.data[output_i] += d_pre_activation;
                for(TI input_i = 0; input_i < INPUT_DIM; input_i++) {
                    TI input_index = batch_i * LAYER_SPEC::INPUT_DIM + input_i;
                    if(output_i == 0){
                        d_input.data[input_index] = 0;
                    }
                    d_input.data[input_index] += layer.weights.data[output_i * INPUT_DIM + input_i] * d_pre_activation;
                    layer.d_weights.data[output_i * INPUT_DIM + input_i] += d_pre_activation * input.data[input_index];
                }
            }
        }
    }
    template<typename DEVICE, typename SPEC>
    FUNCTION_PLACEMENT void zero_gradient(DEVICE& device, nn::layers::dense::LayerBackwardGradient<SPEC>& layer) {
        for(typename DEVICE::index_t i = 0; i < SPEC::OUTPUT_DIM; i++) {
            layer.d_biases.data[i] = 0;
            for(typename DEVICE::index_t j = 0; j < SPEC::INPUT_DIM; j++) {
                layer.d_weights.data[i * SPEC::INPUT_DIM + j] = 0;
            }
        }
    }
    template<typename DEVICE, typename SPEC, typename PARAMETERS>
    FUNCTION_PLACEMENT void update_layer(DEVICE& device, nn::layers::dense::LayerBackwardSGD<SPEC, PARAMETERS>& layer){
        for(typename DEVICE::index_t i = 0; i < SPEC::OUTPUT_DIM; i++) {
            layer.biases.data[i] -= PARAMETERS::ALPHA * layer.d_biases.data[i];
            for(typename DEVICE::index_t j = 0; j < SPEC::INPUT_DIM; j++) {
                layer.weights.data[i * SPEC::INPUT_DIM + j] -= PARAMETERS::ALPHA * layer.d_weights.data[i * SPEC::INPUT_DIM + j];
            }
        }
    }

    template<typename DEVICE, typename SPEC, typename PARAMETERS>
    FUNCTION_PLACEMENT void reset_optimizer_state(DEVICE& device, nn::layers::dense::LayerBackwardAdam<SPEC, PARAMETERS>& layer) {
        for(typename DEVICE::index_t i = 0; i < SPEC::OUTPUT_DIM; i++) {
            layer.d_biases_first_order_moment.data [i] = 0;
            layer.d_biases_second_order_moment.data[i] = 0;
            for(typename DEVICE::index_t j = 0; j < SPEC::INPUT_DIM; j++) {
                layer.d_weights_first_order_moment.data [i * SPEC::INPUT_DIM + j] = 0;
                layer.d_weights_second_order_moment.data[i * SPEC::INPUT_DIM + j] = 0;
            }
        }
    }
    template<typename DEVICE, typename SPEC, typename PARAMETERS>
    FUNCTION_PLACEMENT void gradient_descent(DEVICE& device, nn::layers::dense::LayerBackwardAdam<SPEC, PARAMETERS>& layer, typename SPEC::T first_order_moment_bias_correction, typename SPEC::T second_order_moment_bias_correction){
        for(typename DEVICE::index_t i = 0; i < SPEC::OUTPUT_DIM; i++) {
            typename SPEC::T bias_update = PARAMETERS::ALPHA * first_order_moment_bias_correction * layer.d_biases_first_order_moment.data[i] / (math::sqrt(typename DEVICE::SPEC::MATH(), layer.d_biases_second_order_moment.data[i] * second_order_moment_bias_correction) + PARAMETERS::EPSILON);
            layer.biases.data[i] -= bias_update;
            for(typename DEVICE::index_t j = 0; j < SPEC::INPUT_DIM; j++) {
                typename SPEC::T weight_update = PARAMETERS::ALPHA * first_order_moment_bias_correction * layer.d_weights_first_order_moment.data[i * SPEC::INPUT_DIM + j] / (math::sqrt(typename DEVICE::SPEC::MATH(), layer.d_weights_second_order_moment.data[i * SPEC::INPUT_DIM + j] * second_order_moment_bias_correction) + PARAMETERS::EPSILON);
                layer.weights.data[i * SPEC::INPUT_DIM + j] -= weight_update;
            }
        }
    }

    template<typename DEVICE, typename SPEC, typename PARAMETERS>
    FUNCTION_PLACEMENT void update_layer(DEVICE& device, nn::layers::dense::LayerBackwardAdam<SPEC, PARAMETERS>& layer, typename SPEC::T first_order_moment_bias_correction, typename SPEC::T second_order_moment_bias_correction) {
        // todo remove template params (auto inference)
        utils::polyak::update(device, layer.d_weights_first_order_moment, layer.d_weights, PARAMETERS::BETA_1);
        utils::polyak::update(device, layer. d_biases_first_order_moment, layer.d_biases , PARAMETERS::BETA_1);

        utils::polyak::update_squared(device, layer.d_weights_second_order_moment, layer.d_weights, PARAMETERS::BETA_2);
        utils::polyak::update_squared(device, layer. d_biases_second_order_moment, layer.d_biases , PARAMETERS::BETA_2);

        gradient_descent(device, layer, first_order_moment_bias_correction, second_order_moment_bias_correction);
    }

    template<typename DEVICE, typename TARGET_SPEC, typename SOURCE_SPEC>
    FUNCTION_PLACEMENT void copy(DEVICE& device, nn::layers::dense::Layer<TARGET_SPEC>* target, const nn::layers::dense::Layer<SOURCE_SPEC>* source){
        static_assert(nn::layers::dense::check_spec_memory<TARGET_SPEC, SOURCE_SPEC>);
        for(typename TARGET_SPEC::TI i = 0; i < TARGET_SPEC::OUTPUT_DIM; i++) {
            target->biases.data[i] = source->biases.data[i];
            for(typename TARGET_SPEC::TI j = 0; j < TARGET_SPEC::INPUT_DIM; j++) {
                target->weights.data[i * TARGET_SPEC::INPUT_DIM + j] = source->weights.data[i * TARGET_SPEC::INPUT_DIM + j];
            }
        }
    }
    template<typename DEVICE, typename TARGET_SPEC, typename SOURCE_SPEC>
    FUNCTION_PLACEMENT void copy(DEVICE& device, nn::layers::dense::Layer<TARGET_SPEC>& target, const nn::layers::dense::Layer<SOURCE_SPEC>& source){
        static_assert(nn::layers::dense::check_spec_memory<TARGET_SPEC, SOURCE_SPEC>);
        copy(device, &target, &source);
    }
    template<typename DEVICE, typename TARGET_SPEC, typename SOURCE_SPEC>
    FUNCTION_PLACEMENT void copy(DEVICE& device, nn::layers::dense::LayerBackward<TARGET_SPEC>* target, const nn::layers::dense::LayerBackward<SOURCE_SPEC>* source){
        static_assert(nn::layers::dense::check_spec_memory<TARGET_SPEC, SOURCE_SPEC>);
        copy(device, (nn::layers::dense::Layer<TARGET_SPEC>*) target, (nn::layers::dense::Layer<TARGET_SPEC>*) source);
        for(typename TARGET_SPEC::TI i = 0; i < TARGET_SPEC::OUTPUT_DIM; i++) {
            target->pre_activations.data[i] = source->pre_activations.data[i];
        }
    }
    template<typename DEVICE, typename TARGET_SPEC, typename SOURCE_SPEC>
    FUNCTION_PLACEMENT void copy(DEVICE& device, nn::layers::dense::LayerBackward<TARGET_SPEC>& target, const nn::layers::dense::LayerBackward<SOURCE_SPEC>& source){
        static_assert(nn::layers::dense::check_spec_memory<TARGET_SPEC, SOURCE_SPEC>);
        copy(device, &target, &source);
    }
    template<typename DEVICE, typename TARGET_SPEC, typename SOURCE_SPEC>
    FUNCTION_PLACEMENT void copy(DEVICE& device, nn::layers::dense::LayerBackwardGradient<TARGET_SPEC>* target, const nn::layers::dense::LayerBackwardGradient<SOURCE_SPEC>* source){
        static_assert(nn::layers::dense::check_spec_memory<TARGET_SPEC, SOURCE_SPEC>);
        copy(device, (nn::layers::dense::LayerBackward<TARGET_SPEC>*)target, (nn::layers::dense::LayerBackward<SOURCE_SPEC>*)source);
        for(typename TARGET_SPEC::TI i = 0; i < TARGET_SPEC::OUTPUT_DIM; i++) {
            target->d_biases.data[i] = source->d_biases.data[i];
            target->output.data[i] = source->output.data[i];
            for(typename TARGET_SPEC::TI j = 0; j < TARGET_SPEC::INPUT_DIM; j++) {
                target->d_weights.data[i * TARGET_SPEC::INPUT_DIM + j] = source->d_weights.data[i * TARGET_SPEC::INPUT_DIM + j];
            }
        }
    }
    template<typename DEVICE, typename TARGET_SPEC, typename SOURCE_SPEC>
    FUNCTION_PLACEMENT void copy(DEVICE& device, nn::layers::dense::LayerBackwardGradient<TARGET_SPEC>& target, const nn::layers::dense::LayerBackwardGradient<SOURCE_SPEC>& source){
        static_assert(nn::layers::dense::check_spec_memory<TARGET_SPEC, SOURCE_SPEC>);
        copy(device, &target, &source);
    }

    template<typename DEVICE, typename TARGET_SPEC, typename SOURCE_SPEC, typename TARGET_PARAMETERS, typename SOURCE_PARAMETERS>
    FUNCTION_PLACEMENT void copy(DEVICE& device, nn::layers::dense::LayerBackwardAdam<TARGET_SPEC, TARGET_PARAMETERS>* target, const nn::layers::dense::LayerBackwardAdam<SOURCE_SPEC, SOURCE_PARAMETERS>* source){
        static_assert(nn::layers::dense::check_spec_memory<TARGET_SPEC, SOURCE_SPEC>);
        copy(device, (nn::layers::dense::LayerBackwardGradient<TARGET_SPEC>*)target, (nn::layers::dense::LayerBackwardGradient<SOURCE_SPEC>*)source);
        for(typename TARGET_SPEC::TI i = 0; i < TARGET_SPEC::OUTPUT_DIM; i++) {
            target->d_biases_first_order_moment.data[i] = source->d_biases_first_order_moment.data[i];
            target->d_biases_second_order_moment.data[i] = source->d_biases_second_order_moment.data[i];
            for(typename TARGET_SPEC::TI j = 0; j < TARGET_SPEC::INPUT_DIM; j++) {
                target->d_weights_first_order_moment.data[i * TARGET_SPEC::INPUT_DIM + j] = source->d_weights_first_order_moment.data[i * TARGET_SPEC::INPUT_DIM + j];
                target->d_weights_second_order_moment.data[i * TARGET_SPEC::INPUT_DIM + j] = source->d_weights_second_order_moment.data[i * TARGET_SPEC::INPUT_DIM + j];
            }
        }
    }
    template<typename DEVICE, typename TARGET_SPEC, typename SOURCE_SPEC, typename TARGET_PARAMETERS, typename SOURCE_PARAMETERS>
    FUNCTION_PLACEMENT void copy(DEVICE& device, nn::layers::dense::LayerBackwardAdam<TARGET_SPEC, TARGET_PARAMETERS>& target, const nn::layers::dense::LayerBackwardAdam<SOURCE_SPEC, SOURCE_PARAMETERS>& source){
        static_assert(nn::layers::dense::check_spec_memory<TARGET_SPEC, SOURCE_SPEC>);
        copy(device, &target, &source);
    }

//    namespace nn::layers::dense::helper{
//        template <typename DEVICE, typename T, typename DEVICE::index_t N_ROWS, typename DEVICE::index_t N_COLS>
//        T abs_diff(DEVICE& device, const Matrix<T, typename DEVICE::index_t, N_ROWS, N_COLS, RowMajor>& A, const Matrix<T, typename DEVICE::index_t, N_ROWS, N_COLS, RowMajor>& B) {
//            T acc = 0;
//            for (int i = 0; i < N_ROWS; i++){
//                for (int j = 0; j < N_COLS; j++){
//                    acc += math::abs(A.data[i * N_COLS + j] - B.data[i * N_COLS + j]);
//                }
//            }
//            return acc;
//        }
//    }
    template <typename DEVICE, typename SPEC_1, typename SPEC_2>
    typename SPEC_1::T abs_diff(DEVICE& device, const layer_in_c::nn::layers::dense::Layer<SPEC_1>* l1, const layer_in_c::nn::layers::dense::Layer<SPEC_2>* l2) {
        static_assert(nn::layers::dense::check_spec_memory<SPEC_1, SPEC_2>);
        using T = typename SPEC_1::T;
        T acc = 0;
        acc += abs_diff(device, l1->weights, l2->weights);
        acc += abs_diff(device, l1->biases, l2->biases);
        return acc;
    }
    template <typename DEVICE, typename SPEC_1, typename SPEC_2>
    typename SPEC_1::T abs_diff(DEVICE& device, const layer_in_c::nn::layers::dense::Layer<SPEC_1>& l1, const layer_in_c::nn::layers::dense::Layer<SPEC_2>& l2) {
        static_assert(nn::layers::dense::check_spec_memory<SPEC_1, SPEC_2>);
        return abs_diff(device, &l1, &l2);
    }
    template <typename DEVICE, typename SPEC_1, typename SPEC_2>
    typename SPEC_1::T abs_diff(DEVICE& device, const layer_in_c::nn::layers::dense::LayerBackward<SPEC_1>* l1, const layer_in_c::nn::layers::dense::LayerBackward<SPEC_2>* l2) {
        static_assert(nn::layers::dense::check_spec_memory<SPEC_1, SPEC_2>);
        using T = typename SPEC_1::T;
        T acc = abs_diff(device, (layer_in_c::nn::layers::dense::Layer<SPEC_1>*) l1, (layer_in_c::nn::layers::dense::Layer<SPEC_2>*) l2);
        acc += abs_diff(device, l1->pre_activations, l2->pre_activations);
        return acc;
    }
    template <typename DEVICE, typename SPEC_1, typename SPEC_2>
    typename SPEC_1::T abs_diff(DEVICE& device, const layer_in_c::nn::layers::dense::LayerBackward<SPEC_1>& l1, const layer_in_c::nn::layers::dense::LayerBackward<SPEC_2>& l2) {
        static_assert(nn::layers::dense::check_spec_memory<SPEC_1, SPEC_2>);
        return abs_diff(device, &l1, &l2);
    }
    template <typename DEVICE, typename SPEC_1, typename SPEC_2>
    typename SPEC_1::T abs_diff(DEVICE& device, const layer_in_c::nn::layers::dense::LayerBackwardGradient<SPEC_1>* l1, const layer_in_c::nn::layers::dense::LayerBackwardGradient<SPEC_2>* l2) {
        static_assert(nn::layers::dense::check_spec_memory<SPEC_1, SPEC_2>);
        using T = typename SPEC_1::T;
        T acc = abs_diff(device, (layer_in_c::nn::layers::dense::LayerBackward<SPEC_1>*) l1, (layer_in_c::nn::layers::dense::LayerBackward<SPEC_2>*) l2);
        acc += abs_diff(device, l1->output, l2->output);
        acc += abs_diff(device, l1->d_weights, l2->d_weights);
        acc += abs_diff(device, l1->d_biases, l2->d_biases);
        return acc;
    }
    template <typename DEVICE, typename SPEC_1, typename SPEC_2>
    typename SPEC_1::T abs_diff(DEVICE& device, const layer_in_c::nn::layers::dense::LayerBackwardGradient<SPEC_1>& l1, const layer_in_c::nn::layers::dense::LayerBackwardGradient<SPEC_2>& l2) {
        static_assert(nn::layers::dense::check_spec_memory<SPEC_1, SPEC_2>);
        return abs_diff(device, &l1, &l2);
    }
    template <typename DEVICE, typename SPEC_1, typename SPEC_2, typename PARAMETERS_1, typename PARAMETERS_2>
    typename SPEC_1::T abs_diff(DEVICE& device, const layer_in_c::nn::layers::dense::LayerBackwardAdam<SPEC_1, PARAMETERS_1>* l1, const layer_in_c::nn::layers::dense::LayerBackwardAdam<SPEC_2, PARAMETERS_2>* l2) {
        static_assert(nn::layers::dense::check_spec_memory<SPEC_1, SPEC_2>);
        using T = typename SPEC_1::T;
        T acc = abs_diff(device, (layer_in_c::nn::layers::dense::LayerBackwardGradient<SPEC_1>*) l1, (layer_in_c::nn::layers::dense::LayerBackwardGradient<SPEC_2>*) l2);
        acc += abs_diff(device, l1->d_weights_first_order_moment, l2->d_weights_first_order_moment);
        acc += abs_diff(device, l1->d_weights_second_order_moment, l2->d_weights_second_order_moment);
        acc += abs_diff(device, l1->d_biases_first_order_moment, l2->d_biases_first_order_moment);
        acc += abs_diff(device, l1->d_biases_second_order_moment, l2->d_biases_second_order_moment);
        return acc;
    }
    template <typename DEVICE, typename SPEC_1, typename SPEC_2, typename PARAMETERS_1, typename PARAMETERS_2>
    typename SPEC_1::T abs_diff(DEVICE& device, const layer_in_c::nn::layers::dense::LayerBackwardAdam<SPEC_1, PARAMETERS_1>& l1, const layer_in_c::nn::layers::dense::LayerBackwardAdam<SPEC_2, PARAMETERS_2>& l2) {
        static_assert(nn::layers::dense::check_spec_memory<SPEC_1, SPEC_2>);
        return abs_diff(device, &l1, &l2);
    }
    template <typename DEVICE, typename SPEC>
    void reset_forward_state(DEVICE& device, layer_in_c::nn::layers::dense::LayerBackward<SPEC>* l) {
        set(device, l->pre_activations, 0);
    }
    template <typename DEVICE, typename SPEC>
    void reset_forward_state(DEVICE& device, layer_in_c::nn::layers::dense::LayerBackward<SPEC>& l) {
        reset_forward_state(device, (layer_in_c::nn::layers::dense::Layer<SPEC>*) l);
    }
    template <typename DEVICE, typename SPEC>
    void reset_forward_state(DEVICE& device, layer_in_c::nn::layers::dense::LayerBackwardGradient<SPEC>* l) {
        reset_forward_state(device, (layer_in_c::nn::layers::dense::LayerBackward<SPEC>*) l);
        set(device, l->output, 0);
    }
    template <typename DEVICE, typename SPEC>
    void reset_forward_state(DEVICE& device, layer_in_c::nn::layers::dense::LayerBackwardGradient<SPEC>& l) {
        reset_forward_state(device, &l);
    }
}

#endif