#ifndef LAYER_IN_C_RL_ALGORITHMS_OFF_POLICY_RUNNER
#define LAYER_IN_C_RL_ALGORITHMS_OFF_POLICY_RUNNER
namespace layer_in_c::rl::components::replay_buffer{
    template<typename T_T, typename T_TI, T_TI T_OBSERVATION_DIM, T_TI T_ACTION_DIM, T_TI T_CAPACITY>
    struct Specification{
        using T = T_T;
        using TI = T_TI;
        static constexpr TI OBSERVATION_DIM = T_OBSERVATION_DIM;
        static constexpr TI ACTION_DIM = T_ACTION_DIM;
        static constexpr TI CAPACITY = T_CAPACITY;
    };
    template<typename SPEC, typename SPEC::TI T_BATCH_SIZE>
    struct Batch{
        using T = typename SPEC::T;
        using TI = typename SPEC::TI;

        static constexpr TI BATCH_SIZE = T_BATCH_SIZE;
        static constexpr TI OBSERVATION_DIM = SPEC::OBSERVATION_DIM;
        static constexpr TI ACTION_DIM = SPEC::ACTION_DIM;

        Matrix<MatrixSpecification<T, TI, BATCH_SIZE, OBSERVATION_DIM>> observations;
        Matrix<MatrixSpecification<T, TI, BATCH_SIZE, ACTION_DIM>> actions;
        Matrix<MatrixSpecification<T, TI, BATCH_SIZE, 1>> rewards;
        Matrix<MatrixSpecification<T, TI, BATCH_SIZE, OBSERVATION_DIM>> next_observations;
        Matrix<MatrixSpecification<bool, TI, BATCH_SIZE, 1>> terminated;
        Matrix<MatrixSpecification<bool, TI, BATCH_SIZE, 1>> truncated;
    };

}
namespace layer_in_c::rl::components {
    template <typename T_SPEC>
    struct ReplayBuffer {
        using SPEC = T_SPEC;
        using T = typename SPEC::T;
        using TI = typename SPEC::TI;
        static constexpr typename SPEC::TI CAPACITY = SPEC::CAPACITY;
        T observations[SPEC::CAPACITY][SPEC::OBSERVATION_DIM];
        T actions[SPEC::CAPACITY][SPEC::ACTION_DIM];
        T rewards[SPEC::CAPACITY];
        T next_observations[SPEC::CAPACITY][SPEC::OBSERVATION_DIM];
        bool terminated[SPEC::CAPACITY];
        bool truncated[SPEC::CAPACITY];
        TI position = 0;
        bool full = false;
    };
}
#endif