#ifndef LAYER_IN_C_NN_MODELS_MLP_NETWORK_H
#define LAYER_IN_C_NN_MODELS_MLP_NETWORK_H

#include <layer_in_c/nn/nn.h>
#include <layer_in_c/utils/generic/typing.h>

namespace layer_in_c::nn_models::mlp {
    template <typename DEVICE, typename T_T>
    struct ExampleStructureSpecification{
        typedef T_T T;
        static constexpr typename DEVICE::index_t INPUT_DIM = 10;
        static constexpr typename DEVICE::index_t OUTPUT_DIM = 5;
        static constexpr typename DEVICE::index_t NUM_LAYERS = 3; // The input and output layers count towards the total number of layers
        static constexpr typename DEVICE::index_t HIDDEN_DIM = 30;
        static constexpr nn::activation_functions::ActivationFunction HIDDEN_ACTIVATION_FUNCTION = nn::activation_functions::GELU;
        static constexpr nn::activation_functions::ActivationFunction OUTPUT_ACTIVATION_FUNCTION = nn::activation_functions::IDENTITY;
    };
    template <typename T_DEVICE, typename T_STRUCTURE_SPEC>
    struct InferenceSpecification{
        using DEVICE = T_DEVICE;
        using STRUCTURE_SPEC = T_STRUCTURE_SPEC;
        using S = STRUCTURE_SPEC;
        using T = typename S::T;

        using  INPUT_LAYER = nn::layers::dense::Layer<DEVICE, nn::layers::dense::LayerSpecification<T,  S::INPUT_DIM, S::HIDDEN_DIM, S::HIDDEN_ACTIVATION_FUNCTION>>;
        using HIDDEN_LAYER = nn::layers::dense::Layer<DEVICE, nn::layers::dense::LayerSpecification<T, S::HIDDEN_DIM, S::HIDDEN_DIM, S::HIDDEN_ACTIVATION_FUNCTION>>;
        using OUTPUT_LAYER = nn::layers::dense::Layer<DEVICE, nn::layers::dense::LayerSpecification<T, S::HIDDEN_DIM, S::OUTPUT_DIM, S::OUTPUT_ACTIVATION_FUNCTION>>;
    };

    template <typename T_DEVICE, typename T_STRUCTURE_SPEC>
    struct InferenceBackwardSpecification{
        using DEVICE = T_DEVICE;
        using STRUCTURE_SPEC = T_STRUCTURE_SPEC;
        using S = STRUCTURE_SPEC;
        using T = typename S::T;

        using  INPUT_LAYER = nn::layers::dense::LayerBackward<DEVICE, nn::layers::dense::LayerSpecification<T,  S::INPUT_DIM, S::HIDDEN_DIM, S::HIDDEN_ACTIVATION_FUNCTION>>;
        using HIDDEN_LAYER = nn::layers::dense::LayerBackward<DEVICE, nn::layers::dense::LayerSpecification<T, S::HIDDEN_DIM, S::HIDDEN_DIM, S::HIDDEN_ACTIVATION_FUNCTION>>;
        using OUTPUT_LAYER = nn::layers::dense::LayerBackward<DEVICE, nn::layers::dense::LayerSpecification<T, S::HIDDEN_DIM, S::OUTPUT_DIM, S::OUTPUT_ACTIVATION_FUNCTION>>;
    };

    template<typename T_DEVICE, typename T_STRUCTURE_SPEC, typename T_SGD_PARAMETERS>
    struct SGDSpecification{
        using DEVICE = T_DEVICE;
        using STRUCTURE_SPEC = T_STRUCTURE_SPEC;
        using S = STRUCTURE_SPEC;
        using T = typename S::T;
        using SGD_PARAMETERS = T_SGD_PARAMETERS;

        using  INPUT_LAYER = nn::layers::dense::LayerBackwardSGD<DEVICE, nn::layers::dense::LayerSpecification<T,  S::INPUT_DIM, S::HIDDEN_DIM, S::HIDDEN_ACTIVATION_FUNCTION>, SGD_PARAMETERS>;
        using HIDDEN_LAYER = nn::layers::dense::LayerBackwardSGD<DEVICE, nn::layers::dense::LayerSpecification<T, S::HIDDEN_DIM, S::HIDDEN_DIM, S::HIDDEN_ACTIVATION_FUNCTION>, SGD_PARAMETERS>;
        using OUTPUT_LAYER = nn::layers::dense::LayerBackwardSGD<DEVICE, nn::layers::dense::LayerSpecification<T, S::HIDDEN_DIM, S::OUTPUT_DIM, S::OUTPUT_ACTIVATION_FUNCTION>, SGD_PARAMETERS>;
    };

    template<typename T_DEVICE, typename T_STRUCTURE_SPEC, typename T_ADAM_PARAMETERS>
    struct AdamSpecification{
        using DEVICE = T_DEVICE;
        using STRUCTURE_SPEC = T_STRUCTURE_SPEC;
        using S = STRUCTURE_SPEC;
        using T = typename S::T ;
        using ADAM_PARAMETERS = T_ADAM_PARAMETERS;

        using  INPUT_LAYER = nn::layers::dense::LayerBackwardAdam<DEVICE, nn::layers::dense::LayerSpecification<T,  S::INPUT_DIM, S::HIDDEN_DIM, S::HIDDEN_ACTIVATION_FUNCTION>, ADAM_PARAMETERS>;
        using HIDDEN_LAYER = nn::layers::dense::LayerBackwardAdam<DEVICE, nn::layers::dense::LayerSpecification<T, S::HIDDEN_DIM, S::HIDDEN_DIM, S::HIDDEN_ACTIVATION_FUNCTION>, ADAM_PARAMETERS>;
        using OUTPUT_LAYER = nn::layers::dense::LayerBackwardAdam<DEVICE, nn::layers::dense::LayerSpecification<T, S::HIDDEN_DIM, S::OUTPUT_DIM, S::OUTPUT_ACTIVATION_FUNCTION>, ADAM_PARAMETERS>;
    };

    template<typename T_DEVICE, typename T_SPEC>
    struct NeuralNetwork{
        typedef T_DEVICE DEVICE;
        static_assert(utils::typing::is_same_v<DEVICE, typename T_SPEC::DEVICE>);
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
        DEVICE& device;
        typename SPEC::HIDDEN_LAYER hidden_layers[NUM_HIDDEN_LAYERS] = {typename SPEC::HIDDEN_LAYER(device)};
        typename SPEC::OUTPUT_LAYER output_layer;

        explicit NeuralNetwork(DEVICE& device) : device(device), input_layer(device), output_layer(device) { };

//        NeuralNetwork& operator= (const NeuralNetwork& other) = default;
    };

    template<typename DEVICE, typename SPEC>
    struct NeuralNetworkBackward: public NeuralNetwork<DEVICE, SPEC>{
        explicit NeuralNetworkBackward(DEVICE& device) : NeuralNetwork<DEVICE, SPEC>(device) {};
    };
    template<typename DEVICE, typename SPEC>
    struct NeuralNetworkBackwardGradient: public NeuralNetworkBackward<DEVICE, SPEC>{
        explicit NeuralNetworkBackwardGradient(DEVICE& device) : NeuralNetworkBackward<DEVICE, SPEC>(device) {};
    };

    template<typename DEVICE, typename SPEC>
    struct NeuralNetworkSGD: public NeuralNetworkBackwardGradient<DEVICE, SPEC>{
        explicit NeuralNetworkSGD(DEVICE& device) : NeuralNetworkBackwardGradient<DEVICE, SPEC>(device) {};
    };

    template<typename DEVICE, typename SPEC>
    struct NeuralNetworkAdam: public NeuralNetworkBackwardGradient<DEVICE, SPEC>{
        typename DEVICE::index_t age = 1;
        explicit NeuralNetworkAdam(DEVICE& device) : NeuralNetworkBackwardGradient<DEVICE, SPEC>(device) {};
    };


}
#endif
