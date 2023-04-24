#ifndef BACKPROP_TOOLS_NN_OPTIMIZERS_ADAM_PERSIST_CODE_H
#define BACKPROP_TOOLS_NN_OPTIMIZERS_ADAM_PERSIST_CODE_H

#include "adam.h"

#include <string>
namespace backprop_tools{
    std::string get_type_string(nn::parameters::Adam p){
        return "backprop_tools::nn::parameters::Adam";
    }
    template<typename DEVICE, typename CONTAINER>
    persist::Code save_split(DEVICE &device, nn::parameters::Adam::instance<CONTAINER>& parameter, std::string name, bool const_declaration=false, typename DEVICE::index_t indent=0, bool output_memory_only=false){
        using TI = typename DEVICE::index_t;
        std::stringstream indent_ss;
        for(TI i=0; i < indent; i++){
            indent_ss << "    ";
        }
        std::string ind = indent_ss.str();
        std::stringstream ss, ss_header;
        auto plain = save_split(device, (nn::parameters::Gradient::instance<CONTAINER>&) parameter, name, const_declaration, indent, true);
        ss_header << plain.header;
        ss << plain.body;
        ss << ind << "namespace " << name << " {\n";
        auto gradient_first_order_moment = save_split(device, parameter.gradient_first_order_moment, "gradient_first_order_moment_memory", const_declaration, indent+1);
        ss_header << gradient_first_order_moment.header;
        ss << gradient_first_order_moment.body;
        auto gradient_second_order_moment = save_split(device, parameter.gradient_second_order_moment, "gradient_second_order_moment_memory", const_declaration, indent+1);
        ss_header << gradient_second_order_moment.header;
        ss << gradient_second_order_moment.body;
        if(!output_memory_only){
            ss << ind << "    " << "static_assert(backprop_tools::utils::typing::is_same_v<parameters_memory::CONTAINER_TYPE, gradient_memory::CONTAINER_TYPE>);\n";
            ss << ind << "    " << "static_assert(backprop_tools::utils::typing::is_same_v<gradient_memory::CONTAINER_TYPE, gradient_first_order_moment_memory::CONTAINER_TYPE>);\n";
            ss << ind << "    " << "static_assert(backprop_tools::utils::typing::is_same_v<gradient_memory::CONTAINER_TYPE, gradient_second_order_moment_memory::CONTAINER_TYPE>);\n";
            ss << ind << "    " << (const_declaration ? "const " : "") << "backprop_tools::nn::parameters::Adam::instance<parameters_memory::CONTAINER_TYPE> parameters = {parameters_memory::container, gradient_memory::container, gradient_first_order_moment_memory::container, gradient_second_order_moment_memory::container, };\n";
        }
        ss << ind << "}\n";
        return {"", ss.str()};
    }
}


#endif
