#ifndef LAYER_IN_C_NN_LAYERS_DENSE_OPERATIONS_CUDA_H
#define LAYER_IN_C_NN_LAYERS_DENSE_OPERATIONS_CUDA_H

//#include "operations_generic.h"
#include <layer_in_c/devices/cuda.h>
#include <layer_in_c/nn/nn.h>

#define LAYER_IN_C_CEIL(A, B) (A / B + (A % B == 0 ? 0 : 1))

#include <cublas_v2.h>

namespace layer_in_c{
    namespace nn::dense::cuda{
        template<typename DEV_SPEC, typename SPEC, typename OUTPUT_SPEC>
        __global__ void
        set_biases_kernel(devices::CUDA<DEV_SPEC>& device, const nn::layers::dense::Layer<SPEC> layer, Matrix<OUTPUT_SPEC> output) {
            using T = typename SPEC::T;
            using TI = typename devices::CUDA<DEV_SPEC>::index_t;
            constexpr TI INPUT_DIM = SPEC::INPUT_DIM;
            constexpr TI OUTPUT_DIM = SPEC::OUTPUT_DIM;
            constexpr TI BATCH_SIZE = OUTPUT_SPEC::ROWS;

            TI output_pos = blockIdx.x * blockDim.x + threadIdx.x;
            if(output_pos < OUTPUT_DIM){
                T bias = get(layer.biases, 0, output_pos);
                for(TI batch_i = 0; batch_i < BATCH_SIZE; batch_i++){
                    set(output, batch_i, output_pos, bias);
                }
            }
        }
        template<typename DEV_SPEC, typename SPEC, typename OUTPUT_SPEC>
        void set_biases(devices::CUDA<DEV_SPEC>& device, const nn::layers::dense::Layer<SPEC> layer, Matrix<OUTPUT_SPEC>& output) {
            constexpr typename devices::CUDA<DEV_SPEC>::index_t BLOCKSIZE_BIAS = 32;
            constexpr typename devices::CUDA<DEV_SPEC>::index_t N_BLOCKS_BIAS = LAYER_IN_C_CEIL(SPEC::OUTPUT_DIM, BLOCKSIZE_BIAS);
            dim3 bias_grid(N_BLOCKS_BIAS);
            dim3 bias_block(BLOCKSIZE_BIAS);
            nn::dense::cuda::set_biases_kernel<<<bias_grid, bias_block>>>(device, layer, output);
        }
        template<typename DEV_SPEC, typename SPEC, typename PRE_ACTIVATIONS_SPEC, typename OUTPUT_SPEC>
        __global__ void
        activation_kernel(devices::CUDA<DEV_SPEC>& device, const nn::layers::dense::Layer<SPEC> layer, Matrix<PRE_ACTIVATIONS_SPEC> pre_activations, Matrix<OUTPUT_SPEC> output) {
            using T = typename SPEC::T;
            using TI = typename devices::CUDA<DEV_SPEC>::index_t;
            static_assert(PRE_ACTIVATIONS_SPEC::ROWS == OUTPUT_SPEC::ROWS);
            constexpr TI INPUT_DIM = SPEC::INPUT_DIM;
            constexpr TI OUTPUT_DIM = SPEC::OUTPUT_DIM;
            constexpr TI BATCH_SIZE = PRE_ACTIVATIONS_SPEC::ROWS;

            TI output_pos_x = blockIdx.x * blockDim.x + threadIdx.x;
            TI output_pos_y = blockIdx.y * blockDim.y + threadIdx.y;
            if(output_pos_x < OUTPUT_DIM && output_pos_y < BATCH_SIZE){
                set(output, output_pos_y, output_pos_x, activation<typename DEV_SPEC::MATH_DEVICE, T, SPEC::ACTIVATION_FUNCTION>(get(pre_activations, output_pos_y, output_pos_x)));
            }
        }
        template<typename DEV_SPEC, typename SPEC, typename PRE_ACTIVATIONS_SPEC, typename OUTPUT_SPEC>
        void activation(devices::CUDA<DEV_SPEC>& device, const nn::layers::dense::Layer<SPEC> layer, Matrix<PRE_ACTIVATIONS_SPEC>& pre_activations, Matrix<OUTPUT_SPEC>& output) {
            using T = typename SPEC::T;
            using TI = typename devices::CUDA<DEV_SPEC>::index_t;
            static_assert(PRE_ACTIVATIONS_SPEC::ROWS == OUTPUT_SPEC::ROWS);
            constexpr TI BATCH_SIZE = PRE_ACTIVATIONS_SPEC::ROWS;
            constexpr typename devices::CUDA<DEV_SPEC>::index_t BLOCKSIZE_ACTIVATION_BATCH = 32;
            constexpr typename devices::CUDA<DEV_SPEC>::index_t BLOCKSIZE_ACTIVATION_OUTPUT = 32;
            constexpr typename devices::CUDA<DEV_SPEC>::index_t N_BLOCKS_ACTIVATION_BATCH = LAYER_IN_C_CEIL(BATCH_SIZE, BLOCKSIZE_ACTIVATION_BATCH);
            constexpr typename devices::CUDA<DEV_SPEC>::index_t N_BLOCKS_ACTIVATION_OUTPUT = LAYER_IN_C_CEIL(SPEC::OUTPUT_DIM, BLOCKSIZE_ACTIVATION_OUTPUT);
            dim3 activation_grid(N_BLOCKS_ACTIVATION_OUTPUT, N_BLOCKS_ACTIVATION_BATCH);
            dim3 activation_block(BLOCKSIZE_ACTIVATION_OUTPUT, BLOCKSIZE_ACTIVATION_BATCH);
            nn::dense::cuda::activation_kernel<<<activation_grid, activation_block>>>(device, layer, pre_activations, output);
        }
        template<typename DEV_SPEC, typename SPEC, typename PRE_ACTIVATIONS_SPEC, typename D_OUTPUT_SPEC, typename D_BIASES_SPEC, typename D_PRE_ACTIVATIONS_SPEC>
        __global__ void
        d_activation_kernel(devices::CUDA<DEV_SPEC>& device, const nn::layers::dense::Layer<SPEC> layer, Matrix<PRE_ACTIVATIONS_SPEC> pre_activations, Matrix<D_OUTPUT_SPEC> d_output, Matrix<D_BIASES_SPEC> d_biases, Matrix<D_PRE_ACTIVATIONS_SPEC> d_pre_activations) {
            using T = typename SPEC::T;
            using TI = typename devices::CUDA<DEV_SPEC>::index_t;
            constexpr TI OUTPUT_DIM = SPEC::OUTPUT_DIM;
            containers::check_structure<PRE_ACTIVATIONS_SPEC, D_OUTPUT_SPEC>;
            containers::check_structure<D_OUTPUT_SPEC, D_PRE_ACTIVATIONS_SPEC>;
            constexpr TI BATCH_SIZE = PRE_ACTIVATIONS_SPEC::ROWS;
            static_assert(PRE_ACTIVATIONS_SPEC::COLS == D_BIASES_SPEC::COLS);

            TI output_i = blockIdx.x * blockDim.x + threadIdx.x;
            if(output_i < OUTPUT_DIM){
                T acc = 0;
                for(TI batch_i = 0; batch_i < BATCH_SIZE; batch_i++){
                    T d_pre_activation_temp = d_activation_d_x<typename DEV_SPEC::MATH_DEVICE, T, SPEC::ACTIVATION_FUNCTION>(get(pre_activations, batch_i, output_i)) * get(d_output, batch_i, output_i);
                    set(d_pre_activations, batch_i, output_i, d_pre_activation_temp);
                    acc += d_pre_activation_temp;
                }
                increment(d_biases, 0, output_i, acc);
            }
        }
        template<typename DEV_SPEC, typename SPEC, typename PRE_ACTIVATIONS_SPEC, typename D_OUTPUT_SPEC, typename D_BIASES_SPEC, typename D_PRE_ACTIVATIONS_SPEC>
        void d_activation(devices::CUDA<DEV_SPEC>& device, const nn::layers::dense::Layer<SPEC>& layer, Matrix<PRE_ACTIVATIONS_SPEC>& pre_activations, Matrix<D_OUTPUT_SPEC>& d_output, Matrix<D_BIASES_SPEC>& d_biases, Matrix<D_PRE_ACTIVATIONS_SPEC>& d_pre_activations) {
            constexpr typename devices::CUDA<DEV_SPEC>::index_t BLOCKSIZE_ACTIVATION_OUTPUT = 32;
            constexpr typename devices::CUDA<DEV_SPEC>::index_t N_BLOCKS_ACTIVATION_OUTPUT = LAYER_IN_C_CEIL(SPEC::OUTPUT_DIM, BLOCKSIZE_ACTIVATION_OUTPUT);
            dim3 activation_grid(N_BLOCKS_ACTIVATION_OUTPUT);
            dim3 activation_block(BLOCKSIZE_ACTIVATION_OUTPUT);
            nn::dense::cuda::d_activation_kernel<<<activation_grid, activation_block>>>(device, layer, pre_activations, d_output, d_biases, d_pre_activations);
        }
        template<typename DEV_SPEC, typename SPEC, typename PARAMETERS>
        __global__
        void update_layer_kernel(devices::CUDA<DEV_SPEC>& device, nn::layers::dense::LayerBackwardAdam<SPEC, PARAMETERS> layer, typename SPEC::T first_order_moment_bias_correction, typename SPEC::T second_order_moment_bias_correction) {
            // fully fused adam update
            using DEVICE = devices::CUDA<DEV_SPEC>;
            using T = typename SPEC::T;
            using TI = typename DEVICE::index_t;
            constexpr TI INPUT_DIM = SPEC::INPUT_DIM;
            constexpr TI OUTPUT_DIM = SPEC::OUTPUT_DIM;

            TI input_i = blockIdx.x * blockDim.x + threadIdx.x;
            TI output_i = blockIdx.y * blockDim.y + threadIdx.y;
            if(input_i < INPUT_DIM && output_i < OUTPUT_DIM){
                if(input_i == 0){
                    T d_bias = get(layer.d_biases, 0, output_i);
                    T d_bias_first_order_moment = PARAMETERS::BETA_1 * get(layer.d_biases_first_order_moment, 0, output_i) + (1 - PARAMETERS::BETA_1) * d_bias;
                    set(layer.d_biases_first_order_moment, 0, output_i, d_bias_first_order_moment);
                    T d_bias_second_order_moment = PARAMETERS::BETA_2 * get(layer.d_biases_second_order_moment, 0, output_i) + (1 - PARAMETERS::BETA_2) * d_bias * d_bias;
                    set(layer.d_biases_second_order_moment, 0, output_i, d_bias_second_order_moment);
                    T bias_update = PARAMETERS::ALPHA * first_order_moment_bias_correction * d_bias_first_order_moment / (math::sqrt(typename DEVICE::SPEC::MATH_DEVICE_ACCURATE(), d_bias_second_order_moment * second_order_moment_bias_correction) + PARAMETERS::EPSILON);
                    increment(layer.biases, 0, output_i, -bias_update);
                }
                T d_weight = get(layer.d_weights, output_i, input_i);
                T d_weight_first_order_moment = PARAMETERS::BETA_1 * get(layer.d_weights_first_order_moment, output_i, input_i) + (1 - PARAMETERS::BETA_1) * d_weight;
                set(layer.d_weights_first_order_moment, output_i, input_i, d_weight_first_order_moment);
                T d_weight_second_order_moment = PARAMETERS::BETA_2 * get(layer.d_weights_second_order_moment, output_i, input_i) + (1 - PARAMETERS::BETA_2) * d_weight * d_weight;
                set(layer.d_weights_second_order_moment, output_i, input_i, d_weight_second_order_moment);
                T weight_update = PARAMETERS::ALPHA * first_order_moment_bias_correction * d_weight_first_order_moment / (math::sqrt(typename DEVICE::SPEC::MATH_DEVICE_ACCURATE(), d_weight_second_order_moment * second_order_moment_bias_correction) + PARAMETERS::EPSILON);
                increment(layer.weights, output_i, input_i, -weight_update);
            }
        }
    }

    template<typename DEV_SPEC, typename LAYER_SPEC, typename INPUT_SPEC, typename OUTPUT_SPEC>
    void evaluate(devices::CUDA<DEV_SPEC>& device, const nn::layers::dense::Layer<LAYER_SPEC>& layer, const Matrix<INPUT_SPEC>& input, Matrix<OUTPUT_SPEC>& output){
        static_assert(nn::layers::dense::check_input_output<LAYER_SPEC, INPUT_SPEC, OUTPUT_SPEC>);
        // Warning do not use the same buffer for input and output!
        constexpr auto BATCH_SIZE = INPUT_SPEC::ROWS;
        using DEVICE = devices::CUDA<DEV_SPEC>;
        using T = typename LAYER_SPEC::T;
        using TI = typename DEVICE::index_t;
        {
            nn::dense::cuda::set_biases(device, layer, output);

            constexpr T alpha = 1;
            constexpr T beta = 1;
            // op(A) m x k = input^T   (B x I)
            // op(B) k x n = weights   (I x O)
            // op(C) m x n = OUTPUT    (B x O)
            constexpr auto m = BATCH_SIZE;
            constexpr auto k = LAYER_SPEC::INPUT_DIM;
            constexpr auto n = LAYER_SPEC::OUTPUT_DIM;
            cublasStatus_t stat;
            if constexpr(utils::typing::is_same_v<T, float>){
                stat = cublasSgemm(device.handle, CUBLAS_OP_T, CUBLAS_OP_N, m, n, k, &alpha, (T*)layer.weights._data, row_pitch(layer.weights), (T*)input._data, row_pitch(input), &beta, (T*)output._data, row_pitch(output));
            }
            else{
                stat = cublasDgemm(device.handle, CUBLAS_OP_T, CUBLAS_OP_N, m, n, k, &alpha, (T*)layer.weights._data, row_pitch(layer.weights), (T*)input._data, row_pitch(input), &beta, (T*)output._data, row_pitch(output));
            }

            nn::dense::cuda::activation(device, layer, output, output);
        }
    }

    template<typename DEV_SPEC, typename LAYER_SPEC, typename INPUT_SPEC, typename OUTPUT_SPEC>
    void forward(devices::CUDA<DEV_SPEC>& device, nn::layers::dense::LayerBackward<LAYER_SPEC>& layer, const Matrix<INPUT_SPEC>& input, Matrix<OUTPUT_SPEC>& output) {
        // Warning do not use the same buffer for input and output!
        static_assert(nn::layers::dense::check_input_output<LAYER_SPEC, INPUT_SPEC, OUTPUT_SPEC>);
        constexpr auto BATCH_SIZE = INPUT_SPEC::ROWS;
        using T = typename LAYER_SPEC::T;
        using TI = typename devices::CUDA<DEV_SPEC>::index_t;

        constexpr T alpha = 1;
        constexpr T beta = 1;
        // op(A) m x k = weights^T^T (O x I)
        // op(B) k x n = input^T     (I x B)
        // op(C) m x n = output^T    (O x B)
        constexpr auto m = LAYER_SPEC::OUTPUT_DIM;
        constexpr auto k = LAYER_SPEC::INPUT_DIM;
        constexpr auto n = BATCH_SIZE;

        nn::dense::cuda::set_biases(device, layer, output);

        cublasStatus_t stat;
        if constexpr(utils::typing::is_same_v<T, float>){
            stat = cublasSgemm(device.handle, CUBLAS_OP_T, CUBLAS_OP_N, m, n, k, &alpha, (T*)layer.weights._data, row_pitch(layer.weights), (T*)input._data, row_pitch(input), &beta, (T*)output._data, row_pitch(output));
        }
        else{
            stat = cublasDgemm(device.handle, CUBLAS_OP_T, CUBLAS_OP_N, m, n, k, &alpha, (T*)layer.weights._data, row_pitch(layer.weights), (T*)input._data, row_pitch(input), &beta, (T*)output._data, row_pitch(output));
        }

        copy(device, device, layer.pre_activations, output);

        nn::dense::cuda::activation(device, layer, output, output);
    }

    template<typename DEV_SPEC, typename LAYER_SPEC, typename INPUT_SPEC, typename D_OUTPUT_SPEC, typename D_INPUT_SPEC>
    void backward(devices::CUDA<DEV_SPEC>& device, nn::layers::dense::LayerBackwardGradient<LAYER_SPEC>& layer, const Matrix<INPUT_SPEC>& input, Matrix<D_OUTPUT_SPEC>& d_output, Matrix<D_INPUT_SPEC>& d_input) {
        // Warning do not reuse d_output as d_output is used as a temporary buffer
        // todo: create sparate function that does not set d_input (to save cost on backward pass for the first layer)
        // todo: think about storing gradient in column major order to avoid iterating over the minor dimension
        static_assert(nn::layers::dense::check_input_output<LAYER_SPEC, D_INPUT_SPEC, D_OUTPUT_SPEC>);
        static_assert(nn::layers::dense::check_input_output<LAYER_SPEC, INPUT_SPEC, D_OUTPUT_SPEC>);
        constexpr auto INPUT_DIM = LAYER_SPEC::INPUT_DIM;
        constexpr auto OUTPUT_DIM = LAYER_SPEC::OUTPUT_DIM;
        constexpr auto BATCH_SIZE = D_INPUT_SPEC::ROWS;
        using T = typename LAYER_SPEC::T;
        using TI = typename devices::CUDA<DEV_SPEC>::index_t;

        {
            // d_weights
            constexpr T alpha = 1;
            constexpr T beta = 1;
            // op(A) m x k = input^T       (I x B)
            // op(B) k x n = d_output^T^T  (B x O)
            // op(C) m x n = d_weights^T   (I x O)

            constexpr auto m = LAYER_SPEC::INPUT_DIM;
            constexpr auto n = LAYER_SPEC::OUTPUT_DIM;
            constexpr auto k = BATCH_SIZE;

            nn::dense::cuda::d_activation(device, layer, layer.pre_activations, d_output, layer.d_biases, d_output);

            if constexpr(utils::typing::is_same_v<T, float>){
                cublasSgemm(device.handle, CUBLAS_OP_N, CUBLAS_OP_T, m, n, k, &alpha, (T*)input._data, row_pitch(input), (T*)d_output._data, row_pitch(d_output), &beta, (T*)layer.d_weights._data, row_pitch(layer.d_weights));
            }
            else{
                cublasDgemm(device.handle, CUBLAS_OP_N, CUBLAS_OP_T, m, n, k, &alpha, (T*)input._data, row_pitch(input), (T*)d_output._data, row_pitch(d_output), &beta, (T*)layer.d_weights._data, row_pitch(layer.d_weights));
            }
        }
        {
            // d_input
            constexpr T alpha = 1;
            constexpr T beta = 0;
            // op(A) m x k = weights^T  (I x O)
            // op(B) k x n = d_output^T (O x B)
            // op(C) m x n = d_input^T  (I x B)

            constexpr auto m = LAYER_SPEC::INPUT_DIM;
            constexpr auto n = BATCH_SIZE;
            constexpr auto k = LAYER_SPEC::OUTPUT_DIM;

            if constexpr(utils::typing::is_same_v<T, float>){
                cublasSgemm(device.handle, CUBLAS_OP_N, CUBLAS_OP_N, m, n, k, &alpha, (T*)layer.weights._data, row_pitch(layer.weights), (T*)d_output._data, row_pitch(d_output), &beta, (T*)d_input._data, row_pitch(d_input));
            }
            else{
                cublasDgemm(device.handle, CUBLAS_OP_N, CUBLAS_OP_N, m, n, k, &alpha, (T*)layer.weights._data, row_pitch(layer.weights), (T*)d_output._data, row_pitch(d_output), &beta, (T*)d_input._data, row_pitch(d_input));
            }
        }
    }
    template<typename DEV_SPEC, typename SPEC>
    LAYER_IN_C_FUNCTION_PLACEMENT void zero_gradient(devices::CUDA<DEV_SPEC>& device, nn::layers::dense::LayerBackwardGradient<SPEC>& layer) {
        cudaMemset(layer.d_weights._data, 0, SPEC::INPUT_DIM * SPEC::OUTPUT_DIM * sizeof(typename SPEC::T));
        cudaMemset(layer.d_biases._data, 0, SPEC::OUTPUT_DIM * sizeof(typename SPEC::T));
    }
    template<typename DEV_SPEC, typename SPEC, typename PARAMETERS>
    LAYER_IN_C_FUNCTION_PLACEMENT void reset_optimizer_state(devices::CUDA<DEV_SPEC>& device, nn::layers::dense::LayerBackwardAdam<SPEC, PARAMETERS>& layer) {
        cudaMemset(layer.d_weights_first_order_moment._data, 0, SPEC::INPUT_DIM * SPEC::OUTPUT_DIM * sizeof(typename SPEC::T));
        cudaMemset(layer.d_weights_second_order_moment._data, 0, SPEC::INPUT_DIM * SPEC::OUTPUT_DIM * sizeof(typename SPEC::T));
        cudaMemset(layer.d_biases_first_order_moment._data, 0, SPEC::OUTPUT_DIM * sizeof(typename SPEC::T));
        cudaMemset(layer.d_biases_second_order_moment._data, 0, SPEC::OUTPUT_DIM * sizeof(typename SPEC::T));
    }

    template<typename DEV_SPEC, typename SPEC, typename PARAMETERS>
    void update_layer(devices::CUDA<DEV_SPEC>& device, nn::layers::dense::LayerBackwardAdam<SPEC, PARAMETERS>& layer, typename SPEC::T first_order_moment_bias_correction, typename SPEC::T second_order_moment_bias_correction) {
        constexpr typename devices::CUDA<DEV_SPEC>::index_t BLOCKSIZE_ACTIVATION_OUTPUT = 32;
        constexpr typename devices::CUDA<DEV_SPEC>::index_t BLOCKSIZE_ACTIVATION_INPUT = 32;
        constexpr typename devices::CUDA<DEV_SPEC>::index_t N_BLOCKS_ACTIVATION_OUTPUT = LAYER_IN_C_CEIL(SPEC::OUTPUT_DIM, BLOCKSIZE_ACTIVATION_OUTPUT);
        constexpr typename devices::CUDA<DEV_SPEC>::index_t N_BLOCKS_ACTIVATION_INPUT = LAYER_IN_C_CEIL(SPEC::INPUT_DIM, BLOCKSIZE_ACTIVATION_INPUT);
        dim3 activation_grid(N_BLOCKS_ACTIVATION_INPUT, N_BLOCKS_ACTIVATION_OUTPUT);
        dim3 activation_block(BLOCKSIZE_ACTIVATION_INPUT, BLOCKSIZE_ACTIVATION_OUTPUT);
        nn::dense::cuda::update_layer_kernel<<<activation_grid, activation_block>>>(device, layer, first_order_moment_bias_correction, second_order_moment_bias_correction);
    }
}
#endif
