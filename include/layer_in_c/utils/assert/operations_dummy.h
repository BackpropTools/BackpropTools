#ifndef LAYER_IN_C_ASSERT_OPERATIONS_DUMMY_H
#define LAYER_IN_C_ASSERT_OPERATIONS_DUMMY_H

namespace layer_in_c::utils{
    template <typename DEV_SPEC, typename T>
    void assert(const devices::Dummy<DEV_SPEC>& dev, bool condition, T message){
        if(!condition){
            logging::text(dev.logger, message);
        }
    }
}

#endif
