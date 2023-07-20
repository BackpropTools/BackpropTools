#include <backprop_tools/operations/cpu_mux.h>
#include <backprop_tools/rl/components/off_policy_runner/off_policy_runner.h>
namespace bpt = backprop_tools;
#if defined(BACKPROP_TOOLS_ENABLE_TENSORBOARD) && !defined(BACKPROP_TOOLS_DISABLE_TENSORBOARD)
using LOGGER = bpt::devices::logging::CPU_TENSORBOARD;
#else
using LOGGER = bpt::devices::logging::CPU;
#endif


using DEV_SPEC_SUPER = bpt::devices::cpu::Specification<bpt::devices::math::CPU, bpt::devices::random::CPU, LOGGER>;
using TI = typename bpt::DEVICE_FACTORY<DEV_SPEC_SUPER>::index_t;
namespace execution_hints{
    struct HINTS: bpt::rl::components::off_policy_runner::ExecutionHints<TI, 1>{};
}
struct DEV_SPEC: DEV_SPEC_SUPER{
    using EXECUTION_HINTS = execution_hints::HINTS;
};

using DEVICE = bpt::DEVICE_FACTORY<DEV_SPEC>;

#include <backprop_tools/nn/operations_cpu_mux.h>

// generic nn_model operations use the specialized layer operations depending on the backend device
#include <backprop_tools/nn_models/operations_generic.h>
// simulation is run on the cpu and the environments functions are required in the off_policy_runner operations included afterwards
#include <backprop_tools/rl/environments/mujoco/ant/operations_cpu.h>

#include <backprop_tools/rl/algorithms/td3/operations_cpu_mux.h>

// additional includes for the ui and persisting
#if defined(BACKPROP_TOOLS_ENABLE_HDF5) && !defined(BACKPROP_TOOLS_DISABLE_HDF5)
#include <backprop_tools/nn_models/persist.h>
#include <backprop_tools/rl/components/replay_buffer/persist.h>
#endif

#include <backprop_tools/rl/utils/evaluation.h>

#include "parameters.h"

#include <iostream>
#if defined(BACKPROP_TOOLS_ENABLE_HDF5) && !defined(BACKPROP_TOOLS_DISABLE_HDF5)
#include <highfive/H5File.hpp>
#endif
#include <filesystem>
#include <thread>
#include <future>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <string>
#include <sstream>

using DTYPE = float;

namespace parameter_set = parameters_0;

using parameters_environment = parameter_set::environment<double, typename DEVICE::index_t>;
using ENVIRONMENT = typename parameters_environment::ENVIRONMENT;

using parameters_rl = parameter_set::rl<DTYPE, typename DEVICE::index_t, ENVIRONMENT>;
static_assert(parameters_rl::ActorCriticType::SPEC::PARAMETERS::ACTOR_BATCH_SIZE == parameters_rl::ActorCriticType::SPEC::PARAMETERS::CRITIC_BATCH_SIZE);

#if !defined(BACKPROP_TOOLS_RL_ENVIRONMENTS_MUJOCO_ANT_DISABLE_EVALUATION)
constexpr bool ENABLE_EVALUATION = false;
#else
constexpr bool ENABLE_EVALUATION = true;
#endif
constexpr DEVICE::index_t performance_logging_interval = 100;
constexpr DEVICE::index_t ACTOR_CRITIC_EVALUATION_SYNC_INTERVAL = 100;
constexpr DEVICE::index_t DETERMINISTIC_EVALUATION_INTERVAL = 10000;
constexpr bool ACTOR_ENABLE_CHECKPOINTS = true;
constexpr bool ACTOR_OVERWRITE_CHECKPOINTS = false;
constexpr DEVICE::index_t ACTOR_CHECKPOINT_INTERVAL = 10000;
const std::string ACTOR_CHECKPOINT_DIRECTORY = "checkpoints/td3_ant";
const std::string REPLAY_BUFFER_OUTPUT_PATH = "replay_buffer.h5";
constexpr bool BACKPROP_TOOLS_SAVE_REPLAY_BUFFER = false;

#ifdef BACKPROP_TOOLS_TEST_RL_ENVIRONMENTS_MULTIROTOR_TRAINING_DEBUG
constexpr DEVICE::index_t STEP_LIMIT = parameters_rl::N_WARMUP_STEPS_ACTOR + 5000;
#else
#ifdef BACKPROP_TOOLS_TEST_RL_ENVIRONMENTS_MUJOCO_ANT_TRAINING_TD3_TEST
constexpr DEVICE::index_t STEP_LIMIT = 30000;
#else
constexpr DEVICE::index_t STEP_LIMIT = parameters_rl::REPLAY_BUFFER_CAP * 100;
#endif
#endif
constexpr DEVICE::index_t NUM_RUNS = 1;

std::string sanitize_file_name(const std::string &input) {
    std::string output = input;

    const std::string invalid_chars = R"(<>:\"/\|?*)";

    std::replace_if(output.begin(), output.end(), [&invalid_chars](const char &c) {
        return invalid_chars.find(c) != std::string::npos;
    }, '_');

    return output;
}

void run(){
#if defined(BACKPROP_TOOLS_ENABLE_HDF5) && !defined(BACKPROP_TOOLS_DISABLE_HDF5)
    if(ACTOR_ENABLE_CHECKPOINTS){
        std::cout << "Saving checkpoints to: " << ACTOR_CHECKPOINT_DIRECTORY << std::endl;
    }
#endif
    std::string DATA_FILE_PATH = "learning_curves.h5";
    std::vector<std::vector<DTYPE>> episode_step;
    std::vector<std::vector<DTYPE>> episode_returns;
    std::vector<std::vector<DTYPE>> episode_steps;

    std::vector<std::vector<DTYPE>> eval_step;
    std::vector<std::vector<DTYPE>> eval_return;

    for(typename DEVICE::index_t run_i = 0; run_i < NUM_RUNS; run_i++){
        std::string run_name;
        {
            auto now = std::chrono::system_clock::now();
            auto local_time = std::chrono::system_clock::to_time_t(now);
            std::tm* tm = std::localtime(&local_time);

            std::ostringstream oss;
            oss << std::put_time(tm, "%FT%T%z");
            run_name = sanitize_file_name(oss.str());
        }

        episode_step.push_back({});
        episode_returns.push_back({});
        episode_steps.push_back({});

        eval_step.push_back({});
        eval_return.push_back({});

        auto& run_episode_step = episode_step.back();
        auto& run_episode_returns = episode_returns.back();
        auto& run_episode_steps = episode_steps.back();

        auto& run_eval_step = eval_step.back();
        auto& run_eval_return = eval_return.back();

        auto rng = bpt::random::default_engine(DEVICE::SPEC::RANDOM(), run_i);
        auto evaluation_rng = bpt::random::default_engine(DEVICE::SPEC::RANDOM(), run_i); // separate evaluation rng to make runs reproducible/deterministic even if evaluation is turned on or off

        // device
        typename DEVICE::SPEC::LOGGING logger;
        DEVICE device;
        device.logger = &logger;
        bpt::construct(device, device.logger);

        // optimizer
        parameters_rl::OPTIMIZER actor_optimizer;
        parameters_rl::OPTIMIZER critic_optimizers[2];

        // environment
        DTYPE ui_speed_factor = 1;
//        auto parameters = parameters_environment::parameters;
        bool ui = false;

        // rl
        parameters_rl::ActorCriticType actor_critic;
        bpt::malloc(device, actor_critic);
        bpt::init(device, actor_critic, rng);

        bpt::rl::components::OffPolicyRunner<parameters_rl::OFF_POLICY_RUNNER_SPEC> off_policy_runner;
        bpt::malloc(device, off_policy_runner);

        ENVIRONMENT envs[decltype(off_policy_runner)::N_ENVIRONMENTS], evaluation_env;
        for (auto& env : envs) {
            bpt::malloc(device, env);
        }
        bpt::malloc(device, evaluation_env);

        bpt::init(device, off_policy_runner, envs);

        using CRITIC_BATCH_SPEC = bpt::rl::components::off_policy_runner::BatchSpecification<decltype(off_policy_runner)::SPEC, parameters_rl::ActorCriticType::SPEC::PARAMETERS::CRITIC_BATCH_SIZE>;
        bpt::rl::components::off_policy_runner::Batch<CRITIC_BATCH_SPEC> critic_batches[2];
        bpt::rl::algorithms::td3::CriticTrainingBuffers<parameters_rl::ActorCriticType::SPEC> critic_training_buffers[2];
        parameters_rl::CRITIC_TYPE::BuffersForwardBackward<> critic_buffers[2];
        bpt::malloc(device, critic_batches[0]);
        bpt::malloc(device, critic_batches[1]);
        bpt::malloc(device, critic_training_buffers[0]);
        bpt::malloc(device, critic_training_buffers[1]);
        bpt::malloc(device, critic_buffers[0]);
        bpt::malloc(device, critic_buffers[1]);

        using ACTOR_BATCH_SPEC = bpt::rl::components::off_policy_runner::BatchSpecification<decltype(off_policy_runner)::SPEC, parameters_rl::ActorCriticType::SPEC::PARAMETERS::ACTOR_BATCH_SIZE>;
        bpt::rl::components::off_policy_runner::Batch<ACTOR_BATCH_SPEC> actor_batch;
        bpt::rl::algorithms::td3::ActorTrainingBuffers<parameters_rl::ActorCriticType::SPEC> actor_training_buffers;
        parameters_rl::ACTOR_TYPE::Buffers<> actor_buffers[2];
        parameters_rl::ACTOR_TYPE::Buffers<decltype(off_policy_runner)::N_ENVIRONMENTS> actor_buffers_eval;
        parameters_rl::ACTOR_TYPE::Buffers<1> actor_buffers_deterministic_eval;
        bpt::malloc(device, actor_batch);
        bpt::malloc(device, actor_training_buffers);
        bpt::malloc(device, actor_buffers[0]);
        bpt::malloc(device, actor_buffers[1]);
        bpt::malloc(device, actor_buffers_eval);
        bpt::malloc(device, actor_buffers_deterministic_eval);


        // training
        for(int step_i = 0; step_i < STEP_LIMIT; step_i++){
            auto step_start = std::chrono::high_resolution_clock::now();
            bpt::set_step(device, device.logger, step_i);
            bpt::step(device, off_policy_runner, actor_critic.actor, actor_buffers_eval, rng);
            if(step_i % 1000 == 0){
                std::cout << "run_i: " << run_i << " step_i: " << step_i << std::endl;
            }
            if(step_i > std::max(parameters_rl::ACTOR_CRITIC_PARAMETERS::ACTOR_BATCH_SIZE, parameters_rl::ACTOR_CRITIC_PARAMETERS::CRITIC_BATCH_SIZE)){
                if(step_i >= parameters_rl::N_WARMUP_STEPS_CRITIC){
                    if(step_i % parameters_rl::ActorCriticType::SPEC::PARAMETERS::CRITIC_TRAINING_INTERVAL == 0) {
                        auto train_critic = [&device, &actor_critic, &off_policy_runner](parameters_rl::CRITIC_TYPE& critic, decltype(critic_batches[0])& critic_batch, parameters_rl::OPTIMIZER& optimizer, decltype(actor_buffers[0])& actor_buffers, decltype(critic_buffers[0])& critic_buffers, decltype(critic_training_buffers[0])& critic_training_buffers, decltype(rng)& rng){
                            auto gather_batch_start = std::chrono::high_resolution_clock::now();
                            bpt::target_action_noise(device, actor_critic, critic_training_buffers.target_next_action_noise, rng);
                            bpt::gather_batch(device, off_policy_runner, critic_batch, rng);
                            auto gather_batch_end = std::chrono::high_resolution_clock::now();
                            bpt::add_scalar(device, device.logger, "performance/gather_batch_duration", std::chrono::duration_cast<std::chrono::microseconds>(gather_batch_end - gather_batch_start).count(), performance_logging_interval);
                            auto critic_training_start = std::chrono::high_resolution_clock::now();
                            bpt::train_critic(device, actor_critic, critic, critic_batch, optimizer, actor_buffers, critic_buffers, critic_training_buffers);
                            auto critic_training_end = std::chrono::high_resolution_clock::now();
                            bpt::add_scalar(device, device.logger, "performance/critic_training_duration", std::chrono::duration_cast<std::chrono::microseconds>(critic_training_end - critic_training_start).count(), performance_logging_interval);
                        };
                        auto rng1 = bpt::random::default_engine(DEVICE::SPEC::RANDOM(), std::uniform_int_distribution<DEVICE::index_t>()(rng));
                        auto rng2 = bpt::random::default_engine(DEVICE::SPEC::RANDOM(), std::uniform_int_distribution<DEVICE::index_t>()(rng));

                        if(std::getenv("BACKPROP_TOOLS_TEST_RL_ENVIRONMENTS_MULTIROTOR_TRAINING_CONCURRENT") != nullptr){
                            auto critic_1_training = std::async([&](){return train_critic(actor_critic.critic_1, critic_batches[0], critic_optimizers[0], actor_buffers[0], critic_buffers[0], critic_training_buffers[0], rng1);});
                            auto critic_2_training = std::async([&](){return train_critic(actor_critic.critic_2, critic_batches[1], critic_optimizers[1], actor_buffers[1], critic_buffers[1], critic_training_buffers[1], rng2);});
                            critic_1_training.wait();
                            critic_2_training.wait();
                        }
                        else{
                            train_critic(actor_critic.critic_1, critic_batches[0], critic_optimizers[0], actor_buffers[0], critic_buffers[0], critic_training_buffers[0], rng1);
                            train_critic(actor_critic.critic_2, critic_batches[1], critic_optimizers[1], actor_buffers[1], critic_buffers[1], critic_training_buffers[1], rng2);
                        }
                    }
                    if(step_i % parameters_rl::ActorCriticType::SPEC::PARAMETERS::CRITIC_TARGET_UPDATE_INTERVAL == 0) {
                        auto update_critic_targets_start = std::chrono::high_resolution_clock::now();
                        bpt::update_critic_targets(device, actor_critic);
                        auto update_critic_targets_end = std::chrono::high_resolution_clock::now();
                        bpt::add_scalar(device, device.logger, "performance/update_critic_targets_duration", std::chrono::duration_cast<std::chrono::microseconds>(update_critic_targets_end - update_critic_targets_start).count(), performance_logging_interval);
                    }
                }
                if(step_i >= parameters_rl::N_WARMUP_STEPS_ACTOR){
                    if(step_i % parameters_rl::ActorCriticType::SPEC::PARAMETERS::ACTOR_TRAINING_INTERVAL == 0){
                        bpt::gather_batch(device, off_policy_runner, actor_batch, rng);
                        auto actor_training_start = std::chrono::high_resolution_clock::now();
                        bpt::train_actor(device, actor_critic, actor_batch, actor_optimizer, actor_buffers[0], critic_buffers[0], actor_training_buffers);
                        auto actor_training_end = std::chrono::high_resolution_clock::now();
                        bpt::add_scalar(device, device.logger, "performance/actor_training_duration", std::chrono::duration_cast<std::chrono::microseconds>(actor_training_end - actor_training_start).count(), performance_logging_interval);
                    }
                    if(step_i % parameters_rl::ActorCriticType::SPEC::PARAMETERS::ACTOR_TARGET_UPDATE_INTERVAL == 0) {
                        bpt::update_actor_target(device, actor_critic);
                    }
                }
                if(step_i % ACTOR_CRITIC_EVALUATION_SYNC_INTERVAL == 0){
                    bpt::gather_batch(device, off_policy_runner, critic_batches[0], rng);
                    DTYPE critic_1_loss = bpt::critic_loss(device, actor_critic, actor_critic.critic_1, critic_batches[0], actor_buffers[0], critic_buffers[0], critic_training_buffers[0]);
                    bpt::add_scalar(device, device.logger, "critic_1_loss", critic_1_loss, 100);

                    bpt::gather_batch(device, off_policy_runner, actor_batch, rng);
                    DTYPE actor_value = bpt::mean(device, actor_training_buffers.state_action_value);
                    bpt::add_scalar(device, device.logger, "actor_value", actor_value, 100);

                    {
                        typename DEVICE::index_t num_episodes = 0;
                        DTYPE mean_return = 0;
                        DTYPE mean_steps = 0;

                        for(typename DEVICE::index_t env_i = 0; env_i < parameters_rl::OFF_POLICY_RUNNER_SPEC::N_ENVIRONMENTS; env_i++){
                            auto& episode_stats = off_policy_runner.episode_stats[env_i];
                            if(episode_stats.next_episode_i > 0){
                                for(typename DEVICE::index_t episode_i = 0; episode_i < episode_stats.next_episode_i - 1; episode_i++){
                                    mean_return += get(episode_stats.returns, episode_i, 0);
                                    mean_steps  += get(episode_stats.steps  , episode_i, 0);
                                    num_episodes++;
                                }
                                episode_stats.next_episode_i = 1;
                            }
                        }
                        if(num_episodes > 0){
                            mean_return /= num_episodes;
                            mean_steps /= num_episodes;

                            bpt::add_scalar(device, device.logger, "episode/return", mean_return);
                            bpt::add_scalar(device, device.logger, "episode/length", mean_steps);
                            run_episode_step.push_back(step_i);
                            run_episode_returns.push_back(mean_return);
                            run_episode_steps.push_back(mean_steps);
                        }
                    }
                }
            }
            auto step_end = std::chrono::high_resolution_clock::now();
            bpt::add_scalar(device, device.logger, "performance/step_duration", std::chrono::duration_cast<std::chrono::microseconds>(step_end - step_start).count(), performance_logging_interval);
            if(step_i % DETERMINISTIC_EVALUATION_INTERVAL == 0){
                auto result = bpt::evaluate(device, evaluation_env, ui, actor_critic.actor, bpt::rl::utils::evaluation::Specification<10, parameters_rl::ENVIRONMENT_STEP_LIMIT>(), actor_buffers_deterministic_eval, evaluation_rng);
                bpt::add_scalar(device, device.logger, "evaluation/return/mean", result.mean);
                bpt::add_scalar(device, device.logger, "evaluation/return/std", result.std);
                bpt::add_histogram(device, device.logger, "evaluation/return", result.returns, decltype(result)::N_EPISODES);
                std::cout << "Evaluation return mean: " << result.mean << " (std: " << result.std << ")" << std::endl;
                run_eval_step.push_back(step_i);
                run_eval_return.push_back(result.mean);

//            if(step_i > 250000){
//                ASSERT_GT(mean_return, 1000);
//            }
            }
            if(ACTOR_ENABLE_CHECKPOINTS && step_i % ACTOR_CHECKPOINT_INTERVAL == 0){
                std::filesystem::path actor_output_dir = std::filesystem::path(ACTOR_CHECKPOINT_DIRECTORY) / run_name;
                try {
                    std::filesystem::create_directories(actor_output_dir);
                }
                catch (std::exception& e) {
                }
                std::string checkpoint_name = "latest.h5";
                if(!ACTOR_OVERWRITE_CHECKPOINTS){
                    std::stringstream checkpoint_name_ss;
                    checkpoint_name_ss << "actor_" << std::setw(15) << std::setfill('0') << (step_i / ACTOR_CHECKPOINT_INTERVAL) << "_" << std::setw(15) << std::setfill('0') << step_i << ".h5";
                    checkpoint_name = checkpoint_name_ss.str();
                }
                std::filesystem::path actor_output_path = actor_output_dir / checkpoint_name;
#if defined(BACKPROP_TOOLS_ENABLE_HDF5) && !defined(BACKPROP_TOOLS_DISABLE_HDF5)
                try{
                    auto actor_file = HighFive::File(actor_output_path.string(), HighFive::File::Overwrite);
                    bpt::save(device, actor_critic.actor, actor_file.createGroup("actor"));
                }
                catch(HighFive::Exception& e){
                    std::cout << "Error while saving actor: " << e.what() << std::endl;
                }
#endif
            }
//#if defined(BACKPROP_TOOLS_ENABLE_HDF5) && !defined(BACKPROP_TOOLS_DISABLE_HDF5)
//            if(step_i % ACTOR_CHECKPOINT_INTERVAL == 0){
//                std::filesystem::path actor_output_dir = std::filesystem::path(ACTOR_CHECKPOINT_DIRECTORY) / run_name;
//                try {
//                    std::filesystem::create_directories(actor_output_dir);
//                }
//                catch (std::exception& e) {
//                }
//                std::stringstream checkpoint_name;
//                checkpoint_name << "actor_" << std::setw(15) << std::setfill('0') << step_i << ".h5";
//                std::filesystem::path actor_output_path = actor_output_dir / checkpoint_name.str();
//                try{
//                    auto actor_file = HighFive::File(actor_output_path.string(), HighFive::File::Overwrite);
//                    bpt::save(device, actor_critic.actor, actor_file.createGroup("actor"));
//                }
//                catch(HighFive::Exception& e){
//                    std::cout << "Error while saving actor: " << e.what() << std::endl;
//                }
//            }
//#endif
        }
#if defined(BACKPROP_TOOLS_ENABLE_HDF5) && !defined(BACKPROP_TOOLS_DISABLE_HDF5)
        if constexpr(BACKPROP_TOOLS_SAVE_REPLAY_BUFFER){
            try{
                auto actor_file = HighFive::File(REPLAY_BUFFER_OUTPUT_PATH, HighFive::File::Overwrite);
                auto replay_buffer_group = actor_file.createGroup("replay_buffer");
                for(typename DEVICE::index_t env_i = 0; env_i < decltype(off_policy_runner)::N_ENVIRONMENTS; env_i++){
                    bpt::save(device, off_policy_runner.replay_buffers[env_i], replay_buffer_group.createGroup(std::to_string(env_i)));
                }
            }
            catch(HighFive::Exception& e){
                std::cout << "Error while saving actor: " << e.what() << std::endl;
            }
        }
#endif
        bpt::destruct(device, device.logger);

        bpt::free(device, actor_critic);
        bpt::free(device, off_policy_runner);

        bpt::free(device, critic_batches[0]);
        bpt::free(device, critic_batches[1]);
        bpt::free(device, critic_training_buffers[0]);
        bpt::free(device, critic_training_buffers[1]);
        bpt::free(device, critic_buffers[0]);
        bpt::free(device, critic_buffers[1]);

        bpt::free(device, actor_batch);
        bpt::free(device, actor_training_buffers);
        bpt::free(device, actor_buffers[0]);
        bpt::free(device, actor_buffers[1]);
        bpt::free(device, actor_buffers_eval);
    }


#if defined(BACKPROP_TOOLS_ENABLE_HDF5) && !defined(BACKPROP_TOOLS_DISABLE_HDF5)
    auto data_file = HighFive::File(DATA_FILE_PATH, HighFive::File::Overwrite);
    for(typename DEVICE::index_t run_i = 0; run_i < episode_step.size(); run_i++){
        auto group = data_file.createGroup(std::to_string(run_i));
        group.createDataSet("episode_step", episode_step[run_i]);
        group.createDataSet("episode_returns", episode_returns[run_i]);
        group.createDataSet("episode_steps", episode_steps[run_i]);
        group.createDataSet("eval_step", eval_step[run_i]);
        group.createDataSet("eval_return", eval_return[run_i]);
    }
#endif
}
