#include "layer.h"

namespace backprop_tools{
    template<typename DEVICE, typename LAYER_SPEC, typename INPUT_SPEC, typename OUTPUT_SPEC>
    void evaluate(DEVICE& device, const nn::layers::concat_constant::Layer<LAYER_SPEC>& layer, const Matrix<INPUT_SPEC>& input, Matrix<OUTPUT_SPEC>& output) {
        static_assert(nn::layers::concat_constant::check_input_output<LAYER_SPEC, INPUT_SPEC, OUTPUT_SPEC>);
        // Warning do not use the same buffer for input and output!
        constexpr auto BATCH_SIZE = INPUT_SPEC::ROWS;
        using TI = typename DEVICE::index_t;
        auto input_target_view = view(device, output, matrix::ViewSpec<BATCH_SIZE, LAYER_SPEC::INPUT_DIM>{});
        auto constant_target_view = view(device, output, matrix::ViewSpec<BATCH_SIZE, LAYER_SPEC::INPUT_DIM>{}, 0, LAYER_SPEC::INPUT_DIM);
        copy(device, device, input_target_view, input);
        set_broadcast(device, constant_target_view, layer.constants.parameters);
    }
}
