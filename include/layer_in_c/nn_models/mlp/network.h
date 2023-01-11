#ifndef LAYER_IN_C_NN_MODELS_MLP_NETWORK_H
#define LAYER_IN_C_NN_MODELS_MLP_NETWORK_H

#include <layer_in_c/nn/nn.h>
#include <layer_in_c/utils/generic/typing.h>

namespace layer_in_c::nn_models::mlp {
    template <typename T_T, typename T_TI, T_TI T_INPUT_DIM, T_TI T_OUTPUT_DIM, T_TI T_NUM_LAYERS, T_TI T_HIDDEN_DIM, nn::activation_functions::ActivationFunction T_HIDDEN_ACTIVATION_FUNCTION, nn::activation_functions::ActivationFunction T_OUTPUT_ACTIVATION_FUNCTION>
    struct StructureSpecification{
        typedef T_T T;
        static constexpr T_TI INPUT_DIM = T_INPUT_DIM;
        static constexpr T_TI OUTPUT_DIM = T_OUTPUT_DIM;
        static constexpr T_TI NUM_LAYERS = T_NUM_LAYERS; // The input and output layers count towards the total number of layers
        static constexpr T_TI HIDDEN_DIM = T_HIDDEN_DIM;
        static constexpr auto HIDDEN_ACTIVATION_FUNCTION = T_HIDDEN_ACTIVATION_FUNCTION;
        static constexpr auto OUTPUT_ACTIVATION_FUNCTION = T_OUTPUT_ACTIVATION_FUNCTION;
    };
    template<typename SPEC_1, typename SPEC_2>
    constexpr bool check_spec_memory =
            utils::typing::is_same_v<typename SPEC_1::T, typename SPEC_2::T>
            && SPEC_1::INPUT_DIM == SPEC_2::INPUT_DIM
            && SPEC_1::OUTPUT_DIM == SPEC_2::OUTPUT_DIM
            && SPEC_1::NUM_LAYERS == SPEC_2::NUM_LAYERS
            && SPEC_1::HIDDEN_DIM == SPEC_2::HIDDEN_DIM;
    template<typename SPEC_1, typename SPEC_2>
    constexpr bool check_spec =
            check_spec_memory<SPEC_1, SPEC_2>
            && SPEC_1::HIDDEN_ACTIVATION_FUNCTION == SPEC_2::HIDDEN_ACTIVATION_FUNCTION
            && SPEC_1::OUTPUT_ACTIVATION_FUNCTION == SPEC_2::OUTPUT_ACTIVATION_FUNCTION;


    template <typename T_DEVICE, typename T_STRUCTURE_SPEC>
    struct Specification{
        using DEVICE = T_DEVICE;
        using STRUCTURE_SPEC = T_STRUCTURE_SPEC;
        using S = STRUCTURE_SPEC;
        using T = typename S::T;
        using INPUT_LAYER_SPEC = nn::layers::dense::Specification<T,  S::INPUT_DIM, S::HIDDEN_DIM, S::HIDDEN_ACTIVATION_FUNCTION>;
        using HIDDEN_LAYER_SPEC = nn::layers::dense::Specification<T, S::HIDDEN_DIM, S::HIDDEN_DIM, S::HIDDEN_ACTIVATION_FUNCTION>;
        using OUTPUT_LAYER_SPEC = nn::layers::dense::Specification<T, S::HIDDEN_DIM, S::OUTPUT_DIM, S::OUTPUT_ACTIVATION_FUNCTION>;
    };

    template <typename T_DEVICE, typename T_STRUCTURE_SPEC>
    struct InferenceSpecification: Specification<T_DEVICE, T_STRUCTURE_SPEC>{
        using  INPUT_LAYER = nn::layers::dense::Layer<T_DEVICE, typename Specification<T_DEVICE, T_STRUCTURE_SPEC>::INPUT_LAYER_SPEC>;
        using HIDDEN_LAYER = nn::layers::dense::Layer<T_DEVICE, typename Specification<T_DEVICE, T_STRUCTURE_SPEC>::HIDDEN_LAYER_SPEC>;
        using OUTPUT_LAYER = nn::layers::dense::Layer<T_DEVICE, typename Specification<T_DEVICE, T_STRUCTURE_SPEC>::OUTPUT_LAYER_SPEC>;
    };

    template <typename T_DEVICE, typename T_STRUCTURE_SPEC>
    struct InferenceBackwardSpecification: Specification<T_DEVICE, T_STRUCTURE_SPEC>{
        using  INPUT_LAYER = nn::layers::dense::LayerBackward<T_DEVICE, typename Specification<T_DEVICE, T_STRUCTURE_SPEC>::INPUT_LAYER_SPEC>;
        using HIDDEN_LAYER = nn::layers::dense::LayerBackward<T_DEVICE, typename Specification<T_DEVICE, T_STRUCTURE_SPEC>::HIDDEN_LAYER_SPEC>;
        using OUTPUT_LAYER = nn::layers::dense::LayerBackward<T_DEVICE, typename Specification<T_DEVICE, T_STRUCTURE_SPEC>::OUTPUT_LAYER_SPEC>;
    };

    template <typename T_DEVICE, typename T_STRUCTURE_SPEC>
    struct BackwardGradientSpecification: Specification<T_DEVICE, T_STRUCTURE_SPEC>{
        using  INPUT_LAYER = nn::layers::dense::LayerBackwardGradient<T_DEVICE, typename Specification<T_DEVICE, T_STRUCTURE_SPEC>::INPUT_LAYER_SPEC>;
        using HIDDEN_LAYER = nn::layers::dense::LayerBackwardGradient<T_DEVICE, typename Specification<T_DEVICE, T_STRUCTURE_SPEC>::HIDDEN_LAYER_SPEC>;
        using OUTPUT_LAYER = nn::layers::dense::LayerBackwardGradient<T_DEVICE, typename Specification<T_DEVICE, T_STRUCTURE_SPEC>::OUTPUT_LAYER_SPEC>;
    };

    template<typename T_DEVICE, typename T_STRUCTURE_SPEC, typename T_SGD_PARAMETERS>
    struct SGDSpecification: Specification<T_DEVICE, T_STRUCTURE_SPEC>{
        using SGD_PARAMETERS = T_SGD_PARAMETERS;
        using  INPUT_LAYER = nn::layers::dense::LayerBackwardSGD<T_DEVICE, typename Specification<T_DEVICE, T_STRUCTURE_SPEC>::INPUT_LAYER_SPEC, T_SGD_PARAMETERS>;
        using HIDDEN_LAYER = nn::layers::dense::LayerBackwardSGD<T_DEVICE, typename Specification<T_DEVICE, T_STRUCTURE_SPEC>::HIDDEN_LAYER_SPEC, T_SGD_PARAMETERS>;
        using OUTPUT_LAYER = nn::layers::dense::LayerBackwardSGD<T_DEVICE, typename Specification<T_DEVICE, T_STRUCTURE_SPEC>::OUTPUT_LAYER_SPEC, T_SGD_PARAMETERS>;
    };

    template<typename T_DEVICE, typename T_STRUCTURE_SPEC, typename T_ADAM_PARAMETERS>
    struct AdamSpecification: Specification<T_DEVICE, T_STRUCTURE_SPEC>{
        using ADAM_PARAMETERS = T_ADAM_PARAMETERS;
        using  INPUT_LAYER = nn::layers::dense::LayerBackwardAdam<T_DEVICE, typename Specification<T_DEVICE, T_STRUCTURE_SPEC>::INPUT_LAYER_SPEC, T_ADAM_PARAMETERS>;
        using HIDDEN_LAYER = nn::layers::dense::LayerBackwardAdam<T_DEVICE, typename Specification<T_DEVICE, T_STRUCTURE_SPEC>::HIDDEN_LAYER_SPEC, T_ADAM_PARAMETERS>;
        using OUTPUT_LAYER = nn::layers::dense::LayerBackwardAdam<T_DEVICE, typename Specification<T_DEVICE, T_STRUCTURE_SPEC>::OUTPUT_LAYER_SPEC, T_ADAM_PARAMETERS>;
    };

    template<typename T_DEVICE, typename T_SPEC>
    struct NeuralNetwork{
        typedef T_DEVICE DEVICE;
        static_assert(DEVICE::template compatible<typename T_SPEC::DEVICE>);
        typedef T_SPEC SPEC;
        typedef typename SPEC::T T;

        // Convenience
        static_assert(SPEC::STRUCTURE_SPEC::NUM_LAYERS >= 2); // At least input and output layer are required
        static constexpr typename DEVICE::index_t NUM_HIDDEN_LAYERS = SPEC::STRUCTURE_SPEC::NUM_LAYERS - 2;

        // Interface
        static constexpr typename DEVICE::index_t  INPUT_DIM = SPEC::INPUT_LAYER ::SPEC::INPUT_DIM;
        static constexpr typename DEVICE::index_t OUTPUT_DIM = SPEC::OUTPUT_LAYER::SPEC::OUTPUT_DIM;
        static constexpr typename DEVICE::index_t NUM_WEIGHTS = SPEC::INPUT_LAYER::NUM_WEIGHTS + SPEC::HIDDEN_LAYER::NUM_WEIGHTS * NUM_HIDDEN_LAYERS + SPEC::OUTPUT_LAYER::NUM_WEIGHTS;


        // Storage
        typename SPEC:: INPUT_LAYER input_layer;
        typename SPEC::HIDDEN_LAYER hidden_layers[NUM_HIDDEN_LAYERS];
        typename SPEC::OUTPUT_LAYER output_layer;
    };

    template<typename DEVICE, typename SPEC>
    struct NeuralNetworkBackward: public NeuralNetwork<DEVICE, SPEC>{
    };
    template<typename DEVICE, typename SPEC>
    struct NeuralNetworkBackwardGradient: public NeuralNetworkBackward<DEVICE, SPEC>{
    };

    template<typename DEVICE, typename SPEC>
    struct NeuralNetworkSGD: public NeuralNetworkBackwardGradient<DEVICE, SPEC>{
    };

    template<typename DEVICE, typename SPEC>
    struct NeuralNetworkAdam: public NeuralNetworkBackwardGradient<DEVICE, SPEC>{
        typename DEVICE::index_t age = 1;
    };


}
#endif
