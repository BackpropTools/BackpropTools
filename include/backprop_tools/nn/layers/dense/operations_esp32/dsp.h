#include <backprop_tools/devices/esp32.h>
#include <backprop_tools/nn/layers/dense/layer.h>

#include "esp_dsp.h"

namespace backprop_tools{
    template<typename DEV_SPEC, typename LAYER_SPEC, typename INPUT_SPEC, typename OUTPUT_SPEC>
    void evaluate(devices::esp32::DSP<DEV_SPEC>& device, const nn::layers::dense::Layer<LAYER_SPEC>& layer, const Matrix<INPUT_SPEC>& input, Matrix<OUTPUT_SPEC>& output) {
        // For performance reasons: restricted to dense row-major matrices (row-pitch is allowed)
        static_assert(nn::layers::dense::check_input_output<LAYER_SPEC, INPUT_SPEC, OUTPUT_SPEC>);
        static_assert(INPUT_SPEC::ROWS == 1); // only supporting batch size of 1 for now
        static_assert(INPUT_SPEC::COL_PITCH == 1);
        static_assert(OUTPUT_SPEC::COL_PITCH == 1);
        static_assert(decltype(layer.weights.parameters)::COL_PITCH == 1);
        static_assert(decltype(layer.biases.parameters)::COL_PITCH == 1);
        static_assert(DEV_SPEC::HARDWARE == devices::esp32::Hardware::ORIG || DEV_SPEC::HARDWARE == devices::esp32::Hardware::C3);

        using DEVICE = devices::esp32::DSP<DEV_SPEC>;
        using T = typename LAYER_SPEC::T;
        using TI = typename DEVICE::index_t;

        constexpr TI BATCH_SIZE = INPUT_SPEC::ROWS;


        dspm::Mat input_mat(input._data, LAYER_SPEC::INPUT_DIM, 1);
        dspm::Mat weights_mat(layer.weights.parameters._data, LAYER_SPEC::OUTPUT_DIM, LAYER_SPEC::INPUT_DIM);
        dspm::Mat output_mat(output._data, LAYER_SPEC::OUTPUT_DIM, 1);

        if constexpr(DEV_SPEC::HARDWARE == devices::esp32::Hardware::ORIG){
            dspm_mult_f32_ae32(layer.weights.parameters._data, input._data, output._data, LAYER_SPEC::OUTPUT_DIM, LAYER_SPEC::INPUT_DIM, 1);
        }
        else{
            if constexpr(DEV_SPEC::HARDWARE == devices::esp32::Hardware::C3){
                dspm_mult_f32_ansi(layer.weights.parameters._data, input._data, output._data, LAYER_SPEC::OUTPUT_DIM, LAYER_SPEC::INPUT_DIM, 1);
            }
        }

        for(TI i = 0; i < BATCH_SIZE; i++){
            for(TI j = 0; j < LAYER_SPEC::OUTPUT_DIM; j++){
                T acc = get(output, i, j);
                acc += get(layer.biases.parameters, 0, j);
                set(output, i, j, activation<typename DEVICE::SPEC::MATH, T, LAYER_SPEC::ACTIVATION_FUNCTION>(acc));
            }
        }

    }

}
