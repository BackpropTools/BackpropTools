#ifndef BACKPROP_TOOLS_RL_ENVIRONMENTS_MUJOCO_ANT_ANT_H
#define BACKPROP_TOOLS_RL_ENVIRONMENTS_MUJOCO_ANT_ANT_H

#include <mujoco/mujoco.h>

namespace backprop_tools::rl::environments::mujoco{
    namespace ant{
        template <typename T_T, typename T_TI>
        struct DefaultParameters{
            using T = T_T;
            using TI = T_TI;
            constexpr static T RESET_NOISE_SCALE = 0.1;
            constexpr static T CONTROL_COST_WEIGHT = 0.5;
            constexpr static T HEALTY_Z_MIN = 0.2;
            constexpr static T HEALTY_Z_MAX = 1.0;
            constexpr static bool TERMINATE_WHEN_UNHEALTHY = true;
            constexpr static T HEALTHY_REWARD = 1.0;
            constexpr static T DT = 0.01;
            constexpr static TI FRAME_SKIP = 5;
        };

        template <typename T_T, typename T_TI, typename T_PARAMETERS>
        struct Specification{
            using T = T_T;
            using TI = T_TI;
            using PARAMETERS = T_PARAMETERS;
            constexpr static TI STATE_DIM_Q = 15;
            constexpr static TI STATE_DIM_Q_DOT = 14;
            constexpr static TI STATE_DIM = STATE_DIM_Q + STATE_DIM_Q_DOT;
            constexpr static TI ACTION_DIM = 8;

        };
        template <typename SPEC>
        struct State{
            using T = typename SPEC::T;
            using TI = typename SPEC::TI;
            static constexpr TI DIM = SPEC::STATE_DIM_Q + SPEC::STATE_DIM_Q_DOT;
            T q[SPEC::STATE_DIM_Q];
            T q_dot[SPEC::STATE_DIM_Q_DOT];
        };
    }
    template <typename T_SPEC>
    struct Ant{
        using SPEC = T_SPEC;
        using T = typename SPEC::T;
        static_assert(utils::typing::is_same_v<T, mjtNum>);
        using TI = typename SPEC::TI;
        using State = ant::State<SPEC>;
        mjModel* model;
        mjData* data;
        T init_q[SPEC::STATE_DIM_Q];
        T init_q_dot[SPEC::STATE_DIM_Q_DOT];
        TI torso_id;
        T last_reward;
        bool last_terminated;
        static constexpr TI OBSERVATION_DIM = SPEC::STATE_DIM_Q - 2 + SPEC::STATE_DIM_Q_DOT;
        static constexpr TI ACTION_DIM = SPEC::ACTION_DIM;
    };
}
#endif
