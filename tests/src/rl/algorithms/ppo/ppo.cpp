#include <backprop_tools/operations/cpu_mux.h>
#include <backprop_tools/nn/operations_cpu_mux.h>
#include <backprop_tools/nn_models/operations_cpu.h>
namespace bpt = backprop_tools;
#include "parameters_rl.h"
#include <backprop_tools/rl/components/on_policy_runner/operations_generic.h>
#include <backprop_tools/rl/algorithms/ppo/operations_generic.h>

#include <gtest/gtest.h>

namespace parameters = parameters_0;

using LOGGER = bpt::devices::logging::CPU_TENSORBOARD;
using DEV_SPEC = bpt::devices::cpu::Specification<bpt::devices::math::CPU, bpt::devices::random::CPU, LOGGER>;

using DEVICE = bpt::DEVICE_FACTORY<DEV_SPEC>;
using T = float;
using TI = typename DEVICE::index_t;




TEST(BACKPROP_TOOLS_RL_ALGORITHMS_PPO, TEST){
    using penv = parameters::environment<T, TI>;
    using prl = parameters::rl<T, TI, penv::ENVIRONMENT>;

    DEVICE::SPEC::LOGGING logger;
    DEVICE device;
    prl::OPTIMIZER actor_optimizer, critic_optimizer;
    auto rng = bpt::random::default_engine(DEVICE::SPEC::RANDOM(), 10);
    prl::PPO_TYPE ppo;
    prl::PPO_BUFFERS_TYPE ppo_buffers;
    prl::ON_POLICY_RUNNER_TYPE on_policy_runner;
    prl::ON_POLICY_RUNNER_DATASET_TYPE on_policy_runner_dataset;
    prl::ACTOR_EVAL_BUFFERS actor_eval_buffers;
    prl::ACTOR_BUFFERS actor_buffers;
    prl::CRITIC_BUFFERS critic_buffers;
    prl::CRITIC_BUFFERS_ALL critic_buffers_all;

    bpt::malloc(device, ppo);
    bpt::malloc(device, ppo_buffers);
    bpt::malloc(device, on_policy_runner_dataset);
    bpt::malloc(device, on_policy_runner);
    bpt::malloc(device, actor_eval_buffers);
    bpt::malloc(device, actor_buffers);
    bpt::malloc(device, critic_buffers);
    bpt::malloc(device, critic_buffers_all);

    penv::ENVIRONMENT envs[prl::N_ENVIRONMENTS];
    bpt::init(device, on_policy_runner, envs, rng);
    bpt::init(device, ppo, actor_optimizer, critic_optimizer, rng);
    device.logger = &logger;
    bpt::construct(device, device.logger);
    auto training_start = std::chrono::high_resolution_clock::now();
    for(TI ppo_step_i = 0; ppo_step_i < 1000; ppo_step_i++) {
        device.logger->step = on_policy_runner.step;

        if(ppo_step_i % 100 == 0){
            std::chrono::duration<T> training_elapsed = std::chrono::high_resolution_clock::now() - training_start;
            std::cout << "PPO step: " << ppo_step_i << " elapsed: " << training_elapsed.count() << "s" << std::endl;
            bpt::add_scalar(device, device.logger, "ppo/step", ppo_step_i);
        }
        for (TI action_i = 0; action_i < penv::ENVIRONMENT::ACTION_DIM; action_i++) {
            T action_log_std = bpt::get(ppo.actor.log_std.parameters, 0, action_i);
            std::stringstream topic;
            topic << "actor/action_std/" << action_i;
            bpt::add_scalar(device, device.logger, topic.str(), bpt::math::exp(DEVICE::SPEC::MATH(), action_log_std));
        }
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto start = std::chrono::high_resolution_clock::now();
            bpt::collect(device, on_policy_runner_dataset, on_policy_runner, ppo.actor, actor_eval_buffers, rng);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<T> elapsed = end - start;
//            std::cout << "Rollout: " << elapsed.count() << " s" << std::endl;
        }
        {
            auto start = std::chrono::high_resolution_clock::now();
            evaluate(device, ppo.critic, on_policy_runner_dataset.all_observations, on_policy_runner_dataset.all_values, critic_buffers_all);
            bpt::estimate_generalized_advantages(device, on_policy_runner_dataset, prl::PPO_SPEC::PARAMETERS{});
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<T> elapsed = end - start;
//            std::cout << "GAE: " << elapsed.count() << " s" << std::endl;
        }
        {
            auto start = std::chrono::high_resolution_clock::now();
            bpt::train(device, ppo, on_policy_runner_dataset, actor_optimizer, critic_optimizer, ppo_buffers, actor_buffers, critic_buffers, rng);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<T> elapsed = end - start;
//            std::cout << "Train: " << elapsed.count() << " s" << std::endl;
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<T> elapsed = end - start;
//        std::cout << "Total: " << elapsed.count() << " s" << std::endl;
    }

    bpt::free(device, ppo);
    bpt::free(device, ppo_buffers);
    bpt::free(device, on_policy_runner_dataset);
    bpt::free(device, on_policy_runner);
    bpt::free(device, actor_eval_buffers);
    bpt::free(device, actor_buffers);
    bpt::free(device, critic_buffers);
    bpt::free(device, critic_buffers_all);

}