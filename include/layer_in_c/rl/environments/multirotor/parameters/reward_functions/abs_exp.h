#ifndef LAYER_IN_C_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_REWARD_FUNCTIONS_ABS_EXP_H
#define LAYER_IN_C_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_REWARD_FUNCTIONS_ABS_EXP_H

#include "../../multirotor.h"
#include <layer_in_c/utils/generic/typing.h>
#include <layer_in_c/utils/generic/vector_operations.h>

namespace layer_in_c::rl::environments::multirotor::parameters::reward_functions{
template<typename T>
    struct AbsExp{
        T scale;
        T position;
        T orientation;
        T linear_velocity;
        T angular_velocity;
        T action_baseline;
        T action;
    };
    template<typename DEVICE, typename SPEC>
    static typename SPEC::T reward(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::Multirotor<SPEC>::State& state, const typename SPEC::T action[rl::environments::Multirotor<SPEC>::ACTION_DIM], const typename rl::environments::Multirotor<SPEC>::State& next_state) {
        using T = typename SPEC::T;
        using TI = typename DEVICE::index_t;
        constexpr TI ACTION_DIM = rl::environments::Multirotor<SPEC>::ACTION_DIM;
        auto params = env.parameters.mdp.reward;
        T quaternion_w = state.state[3];
        T orientation_cost = math::abs(2 * math::acos(typename DEVICE::SPEC::MATH(), quaternion_w));
        T position_cost = utils::vector_operations::norm<DEVICE, T, 3>(state.state);
        T linear_vel_cost = utils::vector_operations::norm<DEVICE, T, 3>(&state.state[3+4]);
        T angular_vel_cost = utils::vector_operations::norm<DEVICE, T, 3>(&state.state[3+4+3]);
        T action_diff[ACTION_DIM];
        utils::vector_operations::sub<DEVICE, T, ACTION_DIM>(action, utils::vector_operations::mean<DEVICE, T, ACTION_DIM>(action), action_diff);
        T action_cost = utils::vector_operations::norm<DEVICE, T, ACTION_DIM>(action_diff);
        T weighted_abs_cost = params.position * position_cost + params.orientation * orientation_cost + params.linear_velocity * linear_vel_cost + params.angular_velocity * angular_vel_cost + params.action * action_cost;
        T r = math::exp(typename DEVICE::SPEC::MATH(), -weighted_abs_cost);
        return r * params.scale;
//            return -weighted_abs_cost;
    }
}

#endif
