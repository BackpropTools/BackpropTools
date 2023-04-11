#ifndef LAYER_IN_C_NN_LAYERS_DENSE_OPERATIONS_ARM_DSP_H
#define LAYER_IN_C_NN_LAYERS_DENSE_OPERATIONS_ARM_DSP_H

#include <layer_in_c/nn/layers/dense/operations_generic.h>
//#include <layer_in_c/utils/generic/memcpy.h>
#include <layer_in_c/devices/arm.h>
#include "arm_math.h"

namespace layer_in_c{
    template<typename DEV_SPEC, typename LAYER_SPEC, typename INPUT_SPEC, typename OUTPUT_SPEC>
    void evaluate(devices::arm::DSP<DEV_SPEC>& device, const nn::layers::dense::Layer<LAYER_SPEC>& layer, const Matrix<INPUT_SPEC>& input, Matrix<OUTPUT_SPEC>& output) {
        static_assert(nn::layers::dense::check_input_output<LAYER_SPEC, INPUT_SPEC, OUTPUT_SPEC>);
        static_assert(INPUT_SPEC::ROW_PITCH == INPUT_SPEC::COLS);
        static_assert(INPUT_SPEC::COL_PITCH == 1);
        static_assert(OUTPUT_SPEC::ROW_PITCH == OUTPUT_SPEC::COLS);
        static_assert(OUTPUT_SPEC::COL_PITCH == 1);
        static_assert(decltype(layer.weights.parameters)::ROW_PITCH == INPUT_SPEC::COLS);
        static_assert(decltype(layer.weights.parameters)::COL_PITCH == 1);
        static_assert(decltype(layer.biases.parameters)::COL_PITCH == 1);
        static_assert(decltype(layer.biases.parameters)::ROW_PITCH == decltype(layer.biases.parameters)::COLS);
        static_assert(utils::typing::is_same_v<typename LAYER_SPEC::T, float>);

        // Warning do not use the same buffer for input and output!
        constexpr auto BATCH_SIZE = INPUT_SPEC::ROWS;
        static_assert(BATCH_SIZE == 1);
        using DEVICE = devices::ARM<DEV_SPEC>;
        using T = typename LAYER_SPEC::T;
        using TI = typename DEVICE::index_t;

        arm_matrix_instance_f32 arm_weights = {
                .numRows = LAYER_SPEC::OUTPUT_DIM,
                .numCols = LAYER_SPEC::INPUT_DIM,
                .pData = layer.weights.parameters._data
        };

        arm_matrix_instance_f32 arm_input = {
                .numRows = LAYER_SPEC::INPUT_DIM,
                .numCols = BATCH_SIZE,
                .pData = input._data
        };
        arm_matrix_instance_f32 arm_output = {
                .numRows = LAYER_SPEC::OUTPUT_DIM,
                .numCols = BATCH_SIZE,
                .pData = output._data
        } ;
        arm_mat_mult_f32(&arm_weights, &arm_input, &arm_output);
        // beware this only works for batch size = 1
        arm_add_f32(output._data, layer.biases.parameters._data, output._data, LAYER_SPEC::OUTPUT_DIM);


        for(TI i = 0; i < BATCH_SIZE; i++){
            for(TI j = 0; j < LAYER_SPEC::OUTPUT_DIM; j++){
                set(output, i, j, activation<typename DEVICE::SPEC::MATH, T, LAYER_SPEC::ACTIVATION_FUNCTION>(get(output, i, j)));
            }
        }
    }
}

#endif