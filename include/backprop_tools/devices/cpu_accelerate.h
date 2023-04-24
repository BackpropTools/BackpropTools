#ifndef BACKPROP_TOOLS_DEVICES_CPU_ACCELERATE_H
#define BACKPROP_TOOLS_DEVICES_CPU_ACCELERATE_H

#include <backprop_tools/utils/generic/typing.h>
#include "devices.h"

#include "cpu_blas.h"

namespace backprop_tools::devices{
    template <typename T_SPEC>
    struct CPU_ACCELERATE: CPU_BLAS<T_SPEC>{
        static constexpr DeviceId DEVICE_ID = DeviceId::CPU_ACCELERATE;
    };
    using DefaultCPU_ACCELERATE = CPU_ACCELERATE<DefaultCPUSpecification>;
}

#endif
