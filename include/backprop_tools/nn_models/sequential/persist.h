#ifndef BACKPROP_TOOLS_NN_MODELS_SEQUENTIAL_PERSIST_H
#define BACKPROP_TOOLS_NN_MODELS_SEQUENTIAL_PERSIST_H
#include <backprop_tools/nn/parameters/persist.h>
#include <backprop_tools/nn/optimizers/adam/persist.h>
#include <backprop_tools/nn/persist.h>
#include "model.h"

#include <highfive/H5Group.hpp>
#include <string>

namespace backprop_tools{
        template<typename DEVICE, typename SPEC>
        void save(DEVICE& device, nn_models::sequential::Module<SPEC>& model, HighFive::Group group, typename DEVICE::index_t layer_i = 0) {
            if(layer_i == 0){
                group = group.createGroup("layers");
            }
            save(device, model.content, group.createGroup(std::to_string(layer_i)));
            if constexpr (!utils::typing::is_same_v<typename SPEC::NEXT_MODULE, nn_models::sequential::OutputModule>){
                save(device, model.next_module, group, layer_i + 1);
            }
        }
    template<typename DEVICE, typename SPEC>
    void load(DEVICE& device, nn_models::sequential::Module<SPEC>& model, HighFive::Group group, typename DEVICE::index_t layer_i = 0) {
        if(layer_i == 0){
            group = group.getGroup("layers");
        }
        load(device, model.content, group.getGroup(std::to_string(layer_i)));
        if constexpr (!utils::typing::is_same_v<typename SPEC::NEXT_MODULE, nn_models::sequential::OutputModule>){
            load(device, model.next_module, group, layer_i + 1);
        }
    }
}
#endif
