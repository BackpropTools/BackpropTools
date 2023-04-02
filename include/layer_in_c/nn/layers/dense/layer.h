#ifndef LAYER_IN_C_NN_LAYERS_DENSE_LAYER_H
#define LAYER_IN_C_NN_LAYERS_DENSE_LAYER_H
#include <layer_in_c/nn/activation_functions.h>
#include <layer_in_c/utils/generic/typing.h>
#include <layer_in_c/containers.h>

#include <layer_in_c/nn/parameters/parameters.h>

namespace layer_in_c::nn::layers::dense {
    template <typename LAYER_SPEC, typename INPUT_SPEC, typename OUTPUT_SPEC>
    constexpr bool check_input_output =
            INPUT_SPEC::COLS == LAYER_SPEC::INPUT_DIM &&
            INPUT_SPEC::ROWS == OUTPUT_SPEC::ROWS &&
            //                INPUT_SPEC::ROWS <= OUTPUT_SPEC::ROWS && // todo: could be relaxed to not fill the full output
            OUTPUT_SPEC::COLS == LAYER_SPEC::OUTPUT_DIM &&
            (!LAYER_SPEC::ENFORCE_FLOATING_POINT_TYPE || ( utils::typing::is_same_v<typename LAYER_SPEC::T, typename INPUT_SPEC::T> && utils::typing::is_same_v<typename INPUT_SPEC::T, typename OUTPUT_SPEC::T>));
    template<typename T_T, typename T_TI, T_TI T_INPUT_DIM, T_TI T_OUTPUT_DIM, nn::activation_functions::ActivationFunction T_ACTIVATION_FUNCTION, typename T_PARAMETER_TYPE = parameters::Plain, T_TI T_BATCH_SIZE=1, bool T_ENFORCE_FLOATING_POINT_TYPE=true, typename T_MEMORY_LAYOUT = matrix::layouts::RowMajorAlignmentOptimized<T_TI>>
    struct Specification {
        using T = T_T;
        using TI = T_TI;
        static constexpr auto INPUT_DIM = T_INPUT_DIM;
        static constexpr auto OUTPUT_DIM = T_OUTPUT_DIM;
        static constexpr nn::activation_functions::ActivationFunction ACTIVATION_FUNCTION = T_ACTIVATION_FUNCTION;
        using PARAMETER_TYPE = T_PARAMETER_TYPE;
        static constexpr auto BATCH_SIZE = T_BATCH_SIZE;
        static constexpr bool ENFORCE_FLOATING_POINT_TYPE = T_ENFORCE_FLOATING_POINT_TYPE;
        using MEMORY_LAYOUT = T_MEMORY_LAYOUT;
        // Summary
        static constexpr auto NUM_WEIGHTS = OUTPUT_DIM * INPUT_DIM + OUTPUT_DIM;
    };
    template<typename SPEC_1, typename SPEC_2>
    constexpr bool check_spec_memory =
            utils::typing::is_same_v<typename SPEC_1::T, typename SPEC_2::T>
            && SPEC_1::INPUT_DIM == SPEC_2::INPUT_DIM
            && SPEC_1::OUTPUT_DIM == SPEC_2::OUTPUT_DIM;
    template<typename SPEC_1, typename SPEC_2>
    constexpr bool check_spec =
        check_spec_memory<SPEC_1, SPEC_2>
        && SPEC_1::ACTIVATION_FUNCTION == SPEC_2::ACTIVATION_FUNCTION;

    template<typename T_SPEC>
    struct Layer {
        using SPEC = T_SPEC;
        using T = typename SPEC::T;
        using TI = typename SPEC::TI;
        static constexpr TI INPUT_DIM = SPEC::INPUT_DIM;
        static constexpr TI OUTPUT_DIM = SPEC::OUTPUT_DIM;
        static constexpr TI NUM_WEIGHTS = SPEC::NUM_WEIGHTS;
        typename SPEC::PARAMETER_TYPE::template instance<Matrix<matrix::Specification<T, TI, OUTPUT_DIM, INPUT_DIM, typename SPEC::MEMORY_LAYOUT>>> weights;
        typename SPEC::PARAMETER_TYPE::template instance<Matrix<matrix::Specification<T, TI, 1, OUTPUT_DIM, typename SPEC::MEMORY_LAYOUT>>> biases;
    };
    template<typename SPEC>
    struct LayerBackward : public Layer<SPEC> {
        // This layer supports backpropagation wrt its input but not its weights (for this it stores the intermediate pre_activations)
        Matrix<matrix::Specification<typename SPEC::T, typename SPEC::TI, SPEC::BATCH_SIZE, SPEC::OUTPUT_DIM>> pre_activations;
    };
    template<typename SPEC>
    struct LayerBackwardGradient : public LayerBackward<SPEC> {
        // This layer supports backpropagation wrt its input but including its weights (for this it stores the intermediate outputs in addition to the pre_activations because they determine the gradient wrt the weights of the following layer)
        Matrix<matrix::Specification<typename SPEC::T, typename SPEC::TI, SPEC::BATCH_SIZE, SPEC::OUTPUT_DIM, typename SPEC::MEMORY_LAYOUT>> output;
    };
}

#endif