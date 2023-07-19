#ifndef BACKPROP_TOOLS_RL_ALGORITHMS_TD3
#define BACKPROP_TOOLS_RL_ALGORITHMS_TD3


namespace backprop_tools::rl::algorithms::td3 {
    template<typename T, typename TI>
    struct DefaultParameters {
        static constexpr T GAMMA = 0.99;
        static constexpr TI ACTOR_BATCH_SIZE = 32;
        static constexpr TI CRITIC_BATCH_SIZE = 32;
        static constexpr TI N_WARMUP_STEPS_CRITIC = 0;
        static constexpr TI N_WARMUP_STEPS_ACTOR = 0;
        static constexpr TI CRITIC_TRAINING_INTERVAL = 1;
        static constexpr TI ACTOR_TRAINING_INTERVAL = 2;
        static constexpr TI CRITIC_TARGET_UPDATE_INTERVAL = 2;
        static constexpr TI ACTOR_TARGET_UPDATE_INTERVAL = 2;
        static constexpr T ACTOR_POLYAK = 1.0 - 0.005;
        static constexpr T CRITIC_POLYAK = 1.0 - 0.005;
        static constexpr T TARGET_NEXT_ACTION_NOISE_STD = 0.2;
        static constexpr T TARGET_NEXT_ACTION_NOISE_CLIP = 0.5;
        static constexpr bool IGNORE_TERMINATION = false; // ignoring the termination flag is useful for training on environments with negative rewards, where the agent would try to terminate the episode as soon as possible otherwise
    };

    template<
        typename T_T,
        typename T_TI,
        typename T_ENVIRONMENT,
        typename T_ACTOR_NETWORK_TYPE,
        typename T_ACTOR_TARGET_NETWORK_TYPE,
        typename T_CRITIC_NETWORK_TYPE,
        typename T_CRITIC_TARGET_NETWORK_TYPE,
        typename T_OPTIMIZER,
        typename T_PARAMETERS,
        typename T_CONTAINER_TYPE_TAG = MatrixDynamicTag
    >
    struct Specification {
        using T = T_T;
        using TI = T_TI;
        using ENVIRONMENT = T_ENVIRONMENT;
        using ACTOR_NETWORK_TYPE = T_ACTOR_NETWORK_TYPE;
        using ACTOR_TARGET_NETWORK_TYPE = T_ACTOR_TARGET_NETWORK_TYPE;
        using CRITIC_NETWORK_TYPE = T_CRITIC_NETWORK_TYPE;
        using CRITIC_TARGET_NETWORK_TYPE = T_CRITIC_TARGET_NETWORK_TYPE;
        using OPTIMIZER = T_OPTIMIZER;
        using PARAMETERS = T_PARAMETERS;
        using CONTAINER_TYPE_TAG = T_CONTAINER_TYPE_TAG;
    };

    template<typename T_SPEC, typename T_CONTAINER_TYPE_TAG = typename T_SPEC::CONTAINER_TYPE_TAG>
    struct ActorTrainingBuffers{
        using SPEC = T_SPEC;
        using T = typename SPEC::T;
        using TI = typename SPEC::TI;
        using CONTAINER_TYPE_TAG = T_CONTAINER_TYPE_TAG;
        static constexpr TI BATCH_SIZE = SPEC::PARAMETERS::ACTOR_BATCH_SIZE;
        static constexpr TI OBSERVATION_DIM = SPEC::ENVIRONMENT::OBSERVATION_DIM;
        static constexpr TI ACTION_DIM = SPEC::ENVIRONMENT::ACTION_DIM;

        typename CONTAINER_TYPE_TAG::template type<matrix::Specification<T, TI, BATCH_SIZE, OBSERVATION_DIM + ACTION_DIM>> state_action_value_input;
        template<typename SPEC::TI DIM>
        using STATE_ACTION_VALUE_VIEW = typename decltype(state_action_value_input)::template VIEW<BATCH_SIZE, DIM>;
        STATE_ACTION_VALUE_VIEW<OBSERVATION_DIM> observations;
        STATE_ACTION_VALUE_VIEW<ACTION_DIM> actions;
        typename CONTAINER_TYPE_TAG::template type<matrix::Specification<T, TI, BATCH_SIZE, 1>> state_action_value;
        typename CONTAINER_TYPE_TAG::template type<matrix::Specification<T, TI, BATCH_SIZE, 1>> d_output;
        typename CONTAINER_TYPE_TAG::template type<matrix::Specification<T, TI, BATCH_SIZE, OBSERVATION_DIM + ACTION_DIM>> d_critic_input;
        typename CONTAINER_TYPE_TAG::template type<matrix::Specification<T, TI, BATCH_SIZE, ACTION_DIM>> d_actor_output;
        typename CONTAINER_TYPE_TAG::template type<matrix::Specification<T, TI, BATCH_SIZE, OBSERVATION_DIM>> d_actor_input;
    };
    template<typename T_SPEC, typename T_CONTAINER_TYPE_TAG = typename T_SPEC::CONTAINER_TYPE_TAG>
    struct CriticTrainingBuffers{
        using SPEC = T_SPEC;
        using T = typename SPEC::T;
        using TI = typename SPEC::TI;
        using CONTAINER_TYPE_TAG = T_CONTAINER_TYPE_TAG;
        static constexpr TI BATCH_SIZE = SPEC::PARAMETERS::CRITIC_BATCH_SIZE;
        static constexpr TI OBSERVATION_DIM = SPEC::ENVIRONMENT::OBSERVATION_DIM;
        static constexpr TI ACTION_DIM = SPEC::ENVIRONMENT::ACTION_DIM;


        typename CONTAINER_TYPE_TAG::template type<matrix::Specification<T, TI, BATCH_SIZE, ACTION_DIM>> target_next_action_noise;
        typename CONTAINER_TYPE_TAG::template type<matrix::Specification<T, TI, BATCH_SIZE, OBSERVATION_DIM + ACTION_DIM>> next_state_action_value_input;
        template<typename SPEC::TI DIM>
        using NEXT_STATE_ACTION_VALUE_VIEW = typename decltype(next_state_action_value_input)::template VIEW<BATCH_SIZE, DIM>;
        NEXT_STATE_ACTION_VALUE_VIEW<OBSERVATION_DIM> next_observations;
        NEXT_STATE_ACTION_VALUE_VIEW<ACTION_DIM> next_actions;
        typename CONTAINER_TYPE_TAG::template type<matrix::Specification<T, TI, BATCH_SIZE, 1>> action_value;
        typename CONTAINER_TYPE_TAG::template type<matrix::Specification<T, TI, BATCH_SIZE, 1>> target_action_value;
        typename CONTAINER_TYPE_TAG::template type<matrix::Specification<T, TI, BATCH_SIZE, 1>> next_state_action_value_critic_1;
        typename CONTAINER_TYPE_TAG::template type<matrix::Specification<T, TI, BATCH_SIZE, 1>> next_state_action_value_critic_2;
    };

    template<typename T_SPEC>
    struct ActorCritic {
        using SPEC = T_SPEC;
        using T = typename SPEC::T;
        using TI = typename SPEC::TI;

        typename SPEC::ACTOR_NETWORK_TYPE actor;
        typename SPEC::ACTOR_TARGET_NETWORK_TYPE actor_target;

        typename SPEC::CRITIC_NETWORK_TYPE critic_1;
        typename SPEC::CRITIC_NETWORK_TYPE critic_2;
        typename SPEC::CRITIC_TARGET_NETWORK_TYPE critic_target_1;
        typename SPEC::CRITIC_TARGET_NETWORK_TYPE critic_target_2;

        typename SPEC::OPTIMIZER actor_optimizer;
        typename SPEC::OPTIMIZER critic_optimizers[2];
    };
}



#endif