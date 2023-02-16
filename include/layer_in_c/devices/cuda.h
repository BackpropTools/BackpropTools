#ifndef LAYER_IN_C_DEVICES_CUDA_H
#define LAYER_IN_C_DEVICES_CUDA_H

#include <layer_in_c/utils/generic/typing.h>
#include "devices.h"
#include "cpu.h"
#include <cublas_v2.h>
namespace layer_in_c::devices{
    namespace cuda{
        struct Base{
            static constexpr Device DEVICE = Device::CUDA;
            using index_t = unsigned int;
        };
    }
    namespace math{
        struct CUDA: cuda::Base{
            static constexpr Type TYPE = Type::math;
        };
        struct CUDA_FAST: cuda::Base{
            static constexpr Type TYPE = Type::math;
        };
    }
    namespace random{
        struct CUDA: cuda::Base{
            static constexpr Type TYPE = Type::random;
        };
    }
    namespace logging{
        struct CUDA: cuda::Base{
            static constexpr Type TYPE = Type::logging;
        };
    }
    template <typename T_SPEC>
    struct CUDA: cuda::Base{
        template <typename OTHER_DEVICE>
        static constexpr bool compatible = OTHER_DEVICE::DEVICE == Device::CUDA;
        using SPEC = T_SPEC;
        typename SPEC::LOGGING* logger = nullptr;
        cublasHandle_t handle;
#ifdef LAYER_IN_C_DEBUG_DEVICE_CUDA_CHECK_INIT
        bool initialized = false;
#endif
    };
    struct DefaultCUDASpecification{
        using MATH = devices::math::CPU;
        using MATH_DEVICE = math::CUDA_FAST;
        using MATH_DEVICE_ACCURATE = math::CUDA;
        using RANDOM = random::CUDA;
        using LOGGING = logging::CUDA;
    };
    using DefaultCUDA = CUDA<DefaultCUDASpecification>;
}

#include <iostream>
namespace layer_in_c {
    template <typename SPEC>
    void init(devices::CUDA<SPEC>& device){
        cublasStatus_t stat;
        stat = cublasCreate(&device.handle);
#ifdef LAYER_IN_C_DEBUG_DEVICE_CUDA_CHECK_INIT
        device.initialized = true;
#endif
        if (stat != CUBLAS_STATUS_SUCCESS) {
//            logging::text(device.logger, (const char*)"CUBLAS initialization failed ", cublasGetStatusString(stat));
            std::cout << "CUBLAS initialization failed " << cublasGetStatusString(stat) << std::endl;
        }
    }
    template <typename SPEC>
    void check_status(devices::CUDA<SPEC>& device){
#ifdef LAYER_IN_C_DEBUG_DEVICE_CUDA_CHECK_INIT
        if(!device.initialized){
            std::cerr << "CUDA device not initialized" << std::endl;
        }
#endif
        cudaDeviceSynchronize();
        cudaError_t cudaStatus = cudaGetLastError();
        if (cudaStatus != cudaSuccess) {
            std::cerr << "cuda failed: " << cudaGetErrorString(cudaStatus) << std::endl;
        }
    }

}

#endif
