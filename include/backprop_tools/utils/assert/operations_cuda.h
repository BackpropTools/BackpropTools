#ifndef BACKPROP_TOOLS_UTILS_ASSERT_OPERATIONS_CUDA_H
#define BACKPROP_TOOLS_UTILS_ASSERT_OPERATIONS_CUDA_H
#include <assert.h>
namespace backprop_tools::utils{
    template <typename DEV_SPEC, typename T>
    BACKPROP_TOOLS_FUNCTION_PLACEMENT void assert_exit(devices::CUDA<DEV_SPEC>& dev, bool condition, T message){
        if(!condition){
//            logging::text(dev.logger, message);
            printf("Assertion false: %s\n", message);
            assert(condition);
        }
    }
}

#endif