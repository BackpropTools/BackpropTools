// Discerning device
#include <layer_in_c/devices/cpu.h>
#include <layer_in_c/devices/cpu_tensorboard.h>
namespace lic = layer_in_c;
using DEV_SPEC = lic::devices::cpu::Specification<lic::devices::math::CPU, lic::devices::random::CPU, lic::devices::logging::CPU_TENSORBOARD>;

#ifdef LAYER_IN_C_BACKEND_ENABLE_MKL
#include <layer_in_c/operations/cpu_mkl.h>
#include <layer_in_c/nn/operations_cpu_mkl.h>
using DEVICE = lic::devices::CPU_MKL<DEV_SPEC>;
#else
#ifdef LAYER_IN_C_BACKEND_ENABLE_ACCELERATE
#include <layer_in_c/operations/cpu_accelerate.h>
#include <layer_in_c/nn/operations_cpu_accelerate.h>
using DEVICE = lic::devices::CPU_ACCELERATE<DEV_SPEC>;
#else
#include <layer_in_c/operations/cpu.h>
#include <layer_in_c/nn/operations_generic.h>
using DEVICE = lic::devices::CPU<DEV_SPEC>;
#endif
#endif

// importing logging operations (required by many parts of the library)
#include <layer_in_c/operations/cpu_tensorboard.h>

// generic nn_model operations use the specialized layer operations depending on the backend device
#include <layer_in_c/nn_models/operations_generic.h>
// simulation is run on the cpu and the environments functions are required in the off_policy_runner operations included afterwards
#include <layer_in_c/rl/environments/multirotor/operations_cpu.h>
#include <layer_in_c/rl/algorithms/td3/operations_cpu.h>

// additional includes for the ui and persisting
#include <layer_in_c/rl/environments/multirotor/ui.h>
#include <layer_in_c/nn_models/persist.h>

#include <layer_in_c/rl/utils/evaluation.h>

#include "parameters.h"

#include <gtest/gtest.h>
#include <iostream>
#include <highfive/H5File.hpp>

using DTYPE = float;


namespace parameter_set = parameters_0;

using parameters_environment = parameter_set::environment<DEVICE, DTYPE>;
using ENVIRONMENT = typename parameters_environment::ENVIRONMENT;

using parameters_rl = parameter_set::rl<DEVICE, DTYPE, ENVIRONMENT>;
static_assert(parameters_rl::ActorCriticType::SPEC::PARAMETERS::ACTOR_BATCH_SIZE == parameters_rl::ActorCriticType::SPEC::PARAMETERS::CRITIC_BATCH_SIZE);

TEST(LAYER_IN_C_RL_ENVIRONMENTS_MULTIROTOR, TEST_FULL_TRAINING) {
    std::mt19937 rng(4);

    // device
    typename DEVICE::SPEC::LOGGING logger;
    lic::construct(logger);
    DEVICE device(logger);

    // environment
    DTYPE ui_speed_factor = 1;
    auto parameters = parameters_environment::parameters;
    ENVIRONMENT env({parameters});
#if LAYER_IN_C_ENABLE_MULTIROTOR_UI
    lic::rl::environments::multirotor::UI<ENVIRONMENT> ui;
    ui.host = "localhost";
    ui.port = "8080";
    lic::init(device, env, ui);
#else
    bool ui = false;
#endif

    // rl
    parameters_rl::ActorCriticType actor_critic;
    lic::malloc(device, actor_critic);
    lic::init(device, actor_critic, rng);

    lic::rl::components::OffPolicyRunner<parameters_rl::OFF_POLICY_RUNNER_SPEC> off_policy_runner = {env};
    lic::malloc(device, off_policy_runner);

    lic::rl::components::replay_buffer::Batch<decltype(off_policy_runner.replay_buffer)::SPEC, parameters_rl::ActorCriticType::SPEC::PARAMETERS::CRITIC_BATCH_SIZE> critic_batch;
    lic::rl::algorithms::td3::CriticTrainingBuffers<parameters_rl::ActorCriticType::SPEC> critic_training_buffers;
    lic::malloc(device, critic_batch);
    lic::malloc(device, critic_training_buffers);

    lic::rl::components::replay_buffer::Batch<decltype(off_policy_runner.replay_buffer)::SPEC, parameters_rl::ActorCriticType::SPEC::PARAMETERS::ACTOR_BATCH_SIZE> actor_batch;
    lic::rl::algorithms::td3::ActorTrainingBuffers<parameters_rl::ActorCriticType::SPEC> actor_training_buffers;
    lic::malloc(device, actor_batch);
    lic::malloc(device, actor_training_buffers);

    // training
    for(int step_i = 0; step_i < 500000; step_i++){
        device.logger.step = step_i;
        lic::step(device, off_policy_runner, actor_critic.actor, rng);
        if(step_i % 1000 == 0){
            std::cout << "step_i: " << step_i << std::endl;
        }
        if(off_policy_runner.replay_buffer.full || off_policy_runner.replay_buffer.position > std::max(parameters_rl::ACTOR_CRITIC_PARAMETERS::ACTOR_BATCH_SIZE, parameters_rl::ACTOR_CRITIC_PARAMETERS::CRITIC_BATCH_SIZE)){
            if(step_i >= parameters_rl::N_WARMUP_STEPS_CRITIC){
                if(step_i % parameters_rl::ActorCriticType::SPEC::PARAMETERS::CRITIC_TRAINING_INTERVAL == 0){
                    for(int critic_i = 0; critic_i < 2; critic_i++){
                        lic::target_action_noise(device, actor_critic, critic_training_buffers.target_next_action_noise, rng);
                        lic::gather_batch(device, off_policy_runner.replay_buffer, critic_batch, rng);
                        DTYPE critic_loss = lic::train_critic(device, actor_critic, critic_i == 0 ? actor_critic.critic_1 : actor_critic.critic_2, critic_batch, critic_training_buffers);
                        if(critic_i == 0){
                            lic::add_scalar(device.logger, "critic_1_loss", critic_loss, 100);
                        }
                    }
                }
                if(step_i % parameters_rl::ActorCriticType::SPEC::PARAMETERS::CRITIC_TARGET_UPDATE_INTERVAL == 0) {
                    lic::update_critic_targets(device, actor_critic);
                }
            }
            if(step_i >= parameters_rl::N_WARMUP_STEPS_ACTOR){
                if(step_i % parameters_rl::ActorCriticType::SPEC::PARAMETERS::ACTOR_TRAINING_INTERVAL == 0){
                    lic::gather_batch(device, off_policy_runner.replay_buffer, actor_batch, rng);
                    DTYPE actor_value = lic::train_actor(device, actor_critic, actor_batch, actor_training_buffers);
                    lic::add_scalar(device.logger, "actor_value", actor_value, 100);
                }
                if(step_i % parameters_rl::ActorCriticType::SPEC::PARAMETERS::ACTOR_TARGET_UPDATE_INTERVAL == 0) {
                    lic::update_actor_target(device, actor_critic);
                }
            }
        }
        if(step_i % 10000 == 0){
            DTYPE mean_return = lic::evaluate<DEVICE, ENVIRONMENT, decltype(ui), decltype(actor_critic.actor), decltype(rng), parameters_rl::ENVIRONMENT_STEP_LIMIT, true>(device, env, ui, actor_critic.actor, 1, rng);
            std::cout << "Mean return: " << mean_return << std::endl;
        }
    }
    {
        std::string actor_output_path = "actor.h5";
        auto actor_file = HighFive::File(actor_output_path, HighFive::File::Overwrite);
        lic::save(device, actor_critic.actor, actor_file.createGroup("actor"));
    }
    lic::destruct(logger);
    lic::free(device, critic_batch);
    lic::free(device, critic_training_buffers);
    lic::free(device, actor_batch);
    lic::free(device, actor_training_buffers);
}
