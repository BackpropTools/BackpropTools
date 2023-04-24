namespace backprop_tools{
    namespace rl::components::on_policy_runner{
        template <typename T_SPEC>
        struct CollectionEvaluationBuffer{
            using SPEC = T_SPEC;
            using T = typename SPEC::T;
            using TI = typename SPEC::TI;
            typename SPEC::CONTAINER_TYPE_TAG::template type<matrix::Specification<T, TI, SPEC::N_ENVIRONMENTS, SPEC::ENVIRONMENT::OBSERVATION_DIM>> observations;
            typename SPEC::CONTAINER_TYPE_TAG::template type<matrix::Specification<T, TI, SPEC::N_ENVIRONMENTS, SPEC::ENVIRONMENT::ACTION_DIM>> actions;
        };
    }
    template <typename DEVICE, typename SPEC>
    void malloc(DEVICE& device, rl::components::on_policy_runner::CollectionEvaluationBuffer<SPEC>& buffer){
        malloc(device, buffer.observations);
        malloc(device, buffer.actions);
    }
    template <typename DEVICE, typename SPEC>
    void free(DEVICE& device, rl::components::on_policy_runner::CollectionEvaluationBuffer<SPEC>& buffer){
        free(device, buffer.observations);
        free(device, buffer.actions);
    }
    template <typename DEVICE, typename DEVICE_EVALUATION, typename DATASET_SPEC, typename ACTOR, typename ACTOR_EVALUATION, typename OBSERVATION_MEAN_SPEC, typename OBSERVATION_STD_SPEC, typename RNG> // todo: make this not PPO but general policy with output distribution
    void collect_hybrid(DEVICE& device, DEVICE_EVALUATION& device_evaluation, rl::components::on_policy_runner::Dataset<DATASET_SPEC>& dataset, rl::components::OnPolicyRunner<typename DATASET_SPEC::SPEC>& runner, ACTOR& actor, ACTOR_EVALUATION& actor_evaluation, typename ACTOR_EVALUATION::template Buffers<DATASET_SPEC::SPEC::N_ENVIRONMENTS>& policy_eval_buffers, rl::components::on_policy_runner::CollectionEvaluationBuffer<typename DATASET_SPEC::SPEC> evaluation_buffer, rl::components::on_policy_runner::CollectionEvaluationBuffer<typename DATASET_SPEC::SPEC>& evaluation_buffer_evaluation, Matrix<OBSERVATION_MEAN_SPEC>& observations_mean, Matrix<OBSERVATION_STD_SPEC>& observations_std, RNG& rng){
#ifdef BACKPROP_TOOLS_DEBUG_RL_COMPONENTS_ON_POLICY_RUNNER_CHECK_INIT
        utils::assert_exit(device, runner.initialized, "rl::components::on_policy_runner::collect: runner not initialized");
#endif
        using SPEC = typename DATASET_SPEC::SPEC;
        using BUFFER = rl::components::on_policy_runner::Dataset<SPEC>;
        using T = typename SPEC::T;
        using TI = typename SPEC::TI;
//        TI prologue_time = 0;
//        TI copy_observations_time = 0;
//        TI evaluate_time = 0;
//        TI copy_back_time = 0;
//        TI epilogue_time = 0;
        for(TI step_i = 0; step_i < DATASET_SPEC::STEPS_PER_ENV; step_i++){
            auto actions_mean            = view(device, dataset.actions_mean           , matrix::ViewSpec<SPEC::N_ENVIRONMENTS, SPEC::ENVIRONMENT::ACTION_DIM>()     , step_i*SPEC::N_ENVIRONMENTS, 0);
            auto actions                 = view(device, dataset.actions                , matrix::ViewSpec<SPEC::N_ENVIRONMENTS, SPEC::ENVIRONMENT::ACTION_DIM>()     , step_i*SPEC::N_ENVIRONMENTS, 0);
            auto observations            = view(device, dataset.observations           , matrix::ViewSpec<SPEC::N_ENVIRONMENTS, SPEC::ENVIRONMENT::OBSERVATION_DIM>(), step_i*SPEC::N_ENVIRONMENTS, 0);
            auto observations_normalized = view(device, dataset.observations_normalized, matrix::ViewSpec<SPEC::N_ENVIRONMENTS, SPEC::ENVIRONMENT::OBSERVATION_DIM>(), step_i*SPEC::N_ENVIRONMENTS, 0);

            {
//                auto start = std::chrono::high_resolution_clock::now();
                rl::components::on_policy_runner::prologue(device, observations, observations_normalized, runner, observations_mean, observations_std, rng, step_i);
//                auto end = std::chrono::high_resolution_clock::now();
//                prologue_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            }
            {
//                auto start = std::chrono::high_resolution_clock::now();
                copy(device_evaluation, device, evaluation_buffer_evaluation.observations, observations_normalized);
//                auto end = std::chrono::high_resolution_clock::now();
//                copy_observations_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            }
            {
//                auto start = std::chrono::high_resolution_clock::now();
                evaluate(device_evaluation, actor_evaluation, evaluation_buffer_evaluation.observations, evaluation_buffer_evaluation.actions, policy_eval_buffers);
                cudaDeviceSynchronize();
//                auto end = std::chrono::high_resolution_clock::now();
//                evaluate_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            }
            {

//                auto start = std::chrono::high_resolution_clock::now();
                copy(device, device_evaluation, evaluation_buffer.actions, evaluation_buffer_evaluation.actions);
                copy(device, device, actions_mean, evaluation_buffer.actions);
//                auto end = std::chrono::high_resolution_clock::now();
//                copy_back_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            }
            {
//                auto start = std::chrono::high_resolution_clock::now();
                rl::components::on_policy_runner::epilogue(device, dataset, runner, actions_mean, actions, actor.log_std.parameters, rng, step_i);
//                auto end = std::chrono::high_resolution_clock::now();
//                epilogue_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            }
        }
//        std::cout << "prologue_time: " << prologue_time << std::endl;
//        std::cout << "copy_observations_time: " << copy_observations_time << std::endl;
//        std::cout << "evaluate_time: " << evaluate_time << std::endl;
//        std::cout << "copy_back_time: " << copy_back_time << std::endl;
//        std::cout << "epilogue_time: " << epilogue_time << std::endl;

        // final observation
        for(TI env_i = 0; env_i < SPEC::N_ENVIRONMENTS; env_i++){
            auto& env = get(runner.environments, 0, env_i);
            auto& state = get(runner.states, 0, env_i);
            TI row_i = DATASET_SPEC::STEPS_PER_ENV * SPEC::N_ENVIRONMENTS + env_i;
            auto observation = row(device, dataset.all_observations, row_i);
            observe(device, env, state, observation);
            auto observation_normalized = row(device, dataset.all_observations_normalized, row_i);
            normalize(device, observations_mean, observations_std, observation, observation_normalized);
        }
        runner.step += SPEC::N_ENVIRONMENTS * DATASET_SPEC::STEPS_PER_ENV;
    }
    template <typename DEVICE, typename DEVICE_EVALUATION, typename DATASET_SPEC, typename ACTOR, typename ACTOR_EVALUATION, typename RNG> // todo: make this not PPO but general policy with output distribution
    void collect_hybrid(DEVICE& device, DEVICE_EVALUATION& device_evaluation, rl::components::on_policy_runner::Dataset<DATASET_SPEC>& dataset, rl::components::OnPolicyRunner<typename DATASET_SPEC::SPEC>& runner, ACTOR& actor, ACTOR_EVALUATION& actor_evaluation, typename ACTOR_EVALUATION::template Buffers<DATASET_SPEC::SPEC::N_ENVIRONMENTS>& policy_eval_buffers, rl::components::on_policy_runner::CollectionEvaluationBuffer<typename DATASET_SPEC::SPEC> evaluation_buffer, rl::components::on_policy_runner::CollectionEvaluationBuffer<typename DATASET_SPEC::SPEC>& evaluation_buffer_evaluation, RNG& rng){
        using T = typename DATASET_SPEC::SPEC::T;
        using TI = typename DEVICE::index_t;
        using ENVIRONMENT = typename DATASET_SPEC::SPEC::ENVIRONMENT;
        MatrixDynamic<matrix::Specification<T, TI, 1, ENVIRONMENT::OBSERVATION_DIM>> observation_mean;
        MatrixDynamic<matrix::Specification<T, TI, 1, ENVIRONMENT::OBSERVATION_DIM>> observation_std;
        malloc(device, observation_mean);
        malloc(device, observation_std);
        set_all(device, observation_mean, 0);
        set_all(device, observation_std, 1);
        collect_hybrid(device, device_evaluation, dataset, runner, actor, actor_evaluation, policy_eval_buffers, evaluation_buffer, evaluation_buffer_evaluation, observation_mean, observation_std, rng);
        free(device, observation_mean);
        free(device, observation_std);

    }

}