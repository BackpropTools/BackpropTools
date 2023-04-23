#ifndef BACKPROP_TOOLS_RL_ENVIRONMENTS_MUJOCO_ANT_OPERATIONS_CPU_H
#define BACKPROP_TOOLS_RL_ENVIRONMENTS_MUJOCO_ANT_OPERATIONS_CPU_H

#include "ant.h"
namespace backprop_tools::rl::environments::mujoco::ant{
    #include "model.h"
}
namespace backprop_tools{
    template <typename DEVICE, typename SPEC>
    void malloc(DEVICE& device, rl::environments::mujoco::Ant<SPEC>& env) {
        using TI = typename DEVICE::index_t;
        constexpr typename DEVICE::index_t error_length = 1000;
        char error[error_length] = "Could not load model";
        {
            mjVFS vfs;
            mj_defaultVFS(&vfs);
            mj_makeEmptyFileVFS(&vfs, "model.xml", backprop_tools::rl::environments::mujoco::ant::model_xml_len);
            int file_idx = mj_findFileVFS(&vfs, "model.xml");
            memcpy(vfs.filedata[file_idx], backprop_tools::rl::environments::mujoco::ant::model_xml, backprop_tools::rl::environments::mujoco::ant::model_xml_len);
            env.model = mj_loadXML("model.xml", &vfs, error, error_length);
            mj_deleteFileVFS(&vfs, "model.xml");
        }
#ifdef BACKPROP_TOOLS_DEBUG_RL_ENVIRONMENTS_MUJOCO_CHECK_INIT
        utils::assert_exit(device, env.model != nullptr, error);
#endif
        env.data = mj_makeData(env.model);
        for(TI state_i = 0; state_i < SPEC::STATE_DIM_Q; state_i++){
            env.init_q[state_i] = env.data->qpos[state_i];
        }
        for(TI state_i = 0; state_i < SPEC::STATE_DIM_Q_DOT; state_i++){
            env.init_q_dot[state_i] = env.data->qvel[state_i];
        }
        env.torso_id = mj_name2id(env.model, mjOBJ_XBODY, "torso");

    }
    template <typename DEVICE, typename SPEC>
    void free(DEVICE& device, rl::environments::mujoco::Ant<SPEC>& env){
        mj_deleteData(env.data);
        mj_deleteModel(env.model);
    }
    template<typename DEVICE, typename SPEC, typename RNG>
    BACKPROP_TOOLS_FUNCTION_PLACEMENT static void sample_initial_state(DEVICE& device, const rl::environments::mujoco::Ant<SPEC>& env, typename rl::environments::mujoco::ant::State<SPEC>& state, RNG& rng){
        using T = typename SPEC::T;
        using TI = typename DEVICE::index_t;
        mj_resetData(env.model, env.data);
        for(TI state_i = 0; state_i < SPEC::STATE_DIM_Q; state_i++){
            state.q    [state_i] = env.init_q    [state_i] + random::uniform_real_distribution(typename DEVICE::SPEC::RANDOM(), -SPEC::PARAMETERS::RESET_NOISE_SCALE, SPEC::PARAMETERS::RESET_NOISE_SCALE, rng);
        }
        for(TI state_i = 0; state_i < SPEC::STATE_DIM_Q_DOT; state_i++){
            state.q_dot[state_i] = env.init_q_dot[state_i] + random::normal_distribution(typename DEVICE::SPEC::RANDOM(), (T)0, SPEC::PARAMETERS::RESET_NOISE_SCALE, rng);
        }
        mj_forward(env.model, env.data);
    }
    template<typename DEVICE, typename SPEC>
    static void initial_state(DEVICE& device, const rl::environments::mujoco::Ant<SPEC>& env, typename rl::environments::mujoco::ant::State<SPEC>& state){
        using TI = typename DEVICE::index_t;
        mj_resetData(env.model, env.data);
        for(TI state_i = 0; state_i < SPEC::STATE_DIM_Q; state_i++){
            state.q    [state_i] = env.init_q[state_i];
        }
        for(TI state_i = 0; state_i < SPEC::STATE_DIM_Q_DOT; state_i++){
            state.q_dot[state_i] = env.init_q_dot[state_i];
        }
        mj_forward(env.model, env.data);
    }
    template<typename DEVICE, typename SPEC, typename ACTION_SPEC>
    BACKPROP_TOOLS_FUNCTION_PLACEMENT typename SPEC::T step(DEVICE& device, rl::environments::mujoco::Ant<SPEC>& env, const rl::environments::mujoco::ant::State<SPEC>& state, const Matrix<ACTION_SPEC>& action, rl::environments::mujoco::ant::State<SPEC>& next_state) {
        using T = typename SPEC::T;
        using TI = typename DEVICE::index_t;
        T x_pre = env.data->xpos[env.torso_id * 3];

        T control_cost = 0;
        for(TI action_i = 0; action_i < SPEC::ACTION_DIM; action_i++){
            T control = get(action, 0, action_i);
            control = math::clamp<T>(typename DEVICE::SPEC::MATH(), control, -1, 1);
            env.data->ctrl[action_i] = control;
            control_cost += control * control;
        }
        control_cost *= SPEC::PARAMETERS::CONTROL_COST_WEIGHT;
        for(TI state_i = 0; state_i < SPEC::STATE_DIM_Q; state_i++){
            env.data->qpos[state_i] = state.q[state_i];
        }
        for(TI state_i = 0; state_i < SPEC::STATE_DIM_Q_DOT; state_i++){
            env.data->qvel[state_i] = state.q_dot[state_i];
        }
        for(TI frame_i = 0; frame_i < SPEC::PARAMETERS::FRAME_SKIP; frame_i++){
            mj_step(env.model, env.data);
        }
        for(TI state_i = 0; state_i < SPEC::STATE_DIM_Q; state_i++){
            next_state.q[state_i] = env.data->qpos[state_i];
        }
        for(TI state_i = 0; state_i < SPEC::STATE_DIM_Q_DOT; state_i++){
            next_state.q_dot[state_i] = env.data->qvel[state_i];
        }
        mj_rnePostConstraint(env.model, env.data);

        T x_post = env.data->xpos[env.torso_id * 3];
        bool healthy = is_finite(device, wrap<DEVICE, T, SPEC::STATE_DIM>(device, (T*)next_state.q)) && is_finite(device, wrap<DEVICE, T, SPEC::STATE_DIM>(device, (T*)next_state.q_dot));
        healthy &= next_state.q[2] >= SPEC::PARAMETERS::HEALTY_Z_MIN && next_state.q[2] <= SPEC::PARAMETERS::HEALTY_Z_MAX;
        T healty_reward = healthy || SPEC::PARAMETERS::TERMINATE_WHEN_UNHEALTHY ? SPEC::PARAMETERS::HEALTHY_REWARD : 0;
        T forward_reward = (x_post - x_pre) / SPEC::PARAMETERS::DT / SPEC::PARAMETERS::FRAME_SKIP;
        env.last_reward = forward_reward + healty_reward - control_cost;
        env.last_terminated = SPEC::PARAMETERS::TERMINATE_WHEN_UNHEALTHY && !healthy;
        return SPEC::PARAMETERS::DT * SPEC::PARAMETERS::FRAME_SKIP;
    }
    template<typename DEVICE, typename SPEC, typename ACTION_SPEC>
    BACKPROP_TOOLS_FUNCTION_PLACEMENT static typename SPEC::T reward(DEVICE& device, const rl::environments::mujoco::Ant<SPEC>& env, const rl::environments::mujoco::ant::State<SPEC>& state, const Matrix<ACTION_SPEC>& action, const rl::environments::mujoco::ant::State<SPEC>& next_state){
        return env.last_reward;
    }

    template<typename DEVICE, typename SPEC, typename OBS_SPEC>
    BACKPROP_TOOLS_FUNCTION_PLACEMENT static void observe(DEVICE& device, const rl::environments::mujoco::Ant<SPEC>& env, const rl::environments::mujoco::ant::State<SPEC>& state, Matrix<OBS_SPEC>& observation){
        using T = typename SPEC::T;
        using TI = typename DEVICE::index_t;
        for(TI state_i = 0; state_i < SPEC::STATE_DIM_Q - 2; state_i++){
            set(observation, 0, state_i, state.q[state_i + 2]);
        }
        for(TI state_i = 0; state_i < SPEC::STATE_DIM_Q_DOT; state_i++){
            set(observation, 0, state_i + SPEC::STATE_DIM_Q - 2, state.q_dot[state_i]);
        }
    }
    template<typename DEVICE, typename SPEC, typename RNG>
    BACKPROP_TOOLS_FUNCTION_PLACEMENT static bool terminated(DEVICE& device, const rl::environments::mujoco::Ant<SPEC>& env, const typename rl::environments::mujoco::ant::State<SPEC> state, RNG& rng){
        return env.last_terminated;
    }
}
#endif
