#include <execution>

#include <gtest/gtest.h>

#include <highfive/H5File.hpp>
#pragma push_macro("slots")
#undef slots
#include "matplotlib/matplotlibcpp.h"
#pragma pop_macro("slots")
namespace plt = matplotlibcpp;

#include <layer_in_c/nn_models/models.h>
#include <layer_in_c/rl/algorithms/td3/td3.h>
#include <layer_in_c/rl/components/off_policy_runner/off_policy_runner.h>
#include <layer_in_c/rl/environments/environments.h>
#include <layer_in_c/utils/rng_std.h>
#include <layer_in_c/rl/utils/evaluation.h>

namespace lic = layer_in_c;
#define DTYPE float

typedef lic::rl::environments::pendulum::Spec<DTYPE, lic::rl::environments::pendulum::DefaultParameters<DTYPE>> PENDULUM_SPEC;
typedef lic::rl::environments::Pendulum<lic::devices::Generic, PENDULUM_SPEC> ENVIRONMENT;
ENVIRONMENT env;

template <typename T>
using TestActorNetworkDefinition = lic::rl::algorithms::td3::ActorNetworkSpecification<T, 64, 64, lic::nn::activation_functions::ActivationFunction::RELU, lic::nn::optimizers::adam::DefaultParametersTorch<DTYPE>>;

template <typename T>
using TestCriticNetworkDefinition = lic::rl::algorithms::td3::CriticNetworkSpecification<T, 64, 64, lic::nn::activation_functions::ActivationFunction::RELU, lic::nn::optimizers::adam::DefaultParametersTorch<DTYPE>>;

template <typename T>
struct TD3PendulumParameters: lic::rl::algorithms::td3::DefaultTD3Parameters<T>{
    constexpr static uint32_t CRITIC_BATCH_SIZE = 100;
    constexpr static uint32_t ACTOR_BATCH_SIZE = 100;
};

//constexpr int N_STEPS = 150000;
//constexpr int N_CORES = 4;
//constexpr int N_BLOCKS = 125;
constexpr int N_STEPS = 50000;
constexpr int N_CORES = 4;
constexpr int N_BLOCKS = 50;
constexpr int N_TRAINING_RUNS = N_CORES * N_BLOCKS;

constexpr size_t REPLAY_BUFFER_CAP = N_STEPS;
constexpr size_t ENVIRONMENT_STEP_LIMIT = 200;
typedef lic::rl::algorithms::td3::ActorCritic<lic::devices::Generic, lic::rl::algorithms::td3::ActorCriticSpecification<DTYPE, ENVIRONMENT, TestActorNetworkDefinition<DTYPE>, TestCriticNetworkDefinition<DTYPE>, TD3PendulumParameters<DTYPE>>> ActorCriticType;
typedef lic::rl::algorithms::td3::OffPolicyRunner<DTYPE, ENVIRONMENT, lic::rl::algorithms::td3::DefaultOffPolicyRunnerParameters<DTYPE, REPLAY_BUFFER_CAP, ENVIRONMENT_STEP_LIMIT>> OFF_POLICY_RUNNER_TYPE;
const DTYPE STATE_TOLERANCE = 0.00001;
constexpr int N_WARMUP_STEPS = ActorCriticType::SPEC::PARAMETERS::ACTOR_BATCH_SIZE;
static_assert(ActorCriticType::SPEC::PARAMETERS::ACTOR_BATCH_SIZE == ActorCriticType::SPEC::PARAMETERS::CRITIC_BATCH_SIZE);


OFF_POLICY_RUNNER_TYPE off_policy_runners[N_CORES];
ActorCriticType actor_critics[N_CORES];

TEST(LAYER_IN_C_RL_ALGORITHMS_TD3_PENDULUM, TRAINING_STATS) {

    std::vector<size_t> mean_returns_steps;
    std::vector<std::vector<DTYPE>> mean_returns(N_TRAINING_RUNS);

    for(int block_i=0; block_i < N_BLOCKS; block_i++){
        std::vector<size_t> training_run_indices;
        for(int training_run_i=0; training_run_i < N_CORES; training_run_i++){
            training_run_indices.push_back(training_run_i);
        }
        std::for_each(
                std::execution::par,
                training_run_indices.begin(),
                training_run_indices.end(),
                [&](auto training_run_sub_i)
                {
                    int training_run_i = block_i * N_CORES + training_run_sub_i;
                    std::mt19937 rng(training_run_i);
                    OFF_POLICY_RUNNER_TYPE* temp_runner = new OFF_POLICY_RUNNER_TYPE();
                    off_policy_runners[training_run_sub_i] = *temp_runner;
                    delete temp_runner;
                    auto& off_policy_runner = off_policy_runners[training_run_sub_i];
                    ActorCriticType* temp_actor_critic = new ActorCriticType();
                    actor_critics[training_run_sub_i] = *temp_actor_critic;
                    delete temp_actor_critic;
                    auto& actor_critic = actor_critics[training_run_sub_i];
                    lic::init<lic::devices::Generic, ActorCriticType::SPEC, layer_in_c::utils::random::stdlib::uniform<DTYPE, typeof(rng)>, typeof(rng)>(actor_critic, rng);
                    for(int step_i = 0; step_i < N_STEPS; step_i++){
                        lic::step(off_policy_runner, actor_critic.actor, rng);
                        if(off_policy_runner.replay_buffer.full || off_policy_runner.replay_buffer.position > N_WARMUP_STEPS){
                            DTYPE critic_1_loss = lic::train_critic(actor_critic, actor_critic.critic_1, off_policy_runner.replay_buffer, rng);
                            lic::train_critic(actor_critic, actor_critic.critic_2, off_policy_runner.replay_buffer, rng);
                            if(step_i % 2 == 0){
                                lic::train_actor(actor_critic, off_policy_runner.replay_buffer, rng);
                                lic::update_targets(actor_critic);
                            }
                        }
                        if(step_i % 1000 == 0){
                            std::cout << "step_i: " << step_i << std::endl;
                            DTYPE mean_return = lic::evaluate<ENVIRONMENT, ActorCriticType::ACTOR_NETWORK_TYPE, typeof(rng), ENVIRONMENT_STEP_LIMIT, false>(ENVIRONMENT(), actor_critic.actor, 500, rng);
                            std::cout << "Mean return: " << mean_return << std::endl;
                            if(block_i == 0 && training_run_i == 0){
                                mean_returns_steps.push_back(step_i);
                            }
                            mean_returns[training_run_i].push_back(mean_return);
                        }
                    }

                }
        );
    }
    std::string results_dir = "results";
    mkdir(results_dir.c_str(), 0777);

    std::string training_stats_dir = results_dir + "/training_stats";
    mkdir(training_stats_dir.c_str(), 0777);

    plt::figure();
    plt::title(std::string("TD3 Pendulum Training Curves (" + std::to_string(N_TRAINING_RUNS) + " runs)"));
    plt::xlabel("step");
    plt::ylabel("mean return (over 500 episodes)");
    for(int training_run_i=0; training_run_i < N_TRAINING_RUNS; training_run_i++){
        plt::plot(mean_returns_steps, mean_returns[training_run_i]);
    }
    std::string learning_curve_filename = training_stats_dir + "/training_curve.png";
    plt::save(learning_curve_filename);
    plt::close();

    std::string training_stats_file = training_stats_dir + "/td3_pendulum_training_stats.h5";
    HighFive::File file(training_stats_file, HighFive::File::Overwrite);
    HighFive::DataSet dataset = file.createDataSet<size_t>("mean_returns_steps", HighFive::DataSpace::From(mean_returns_steps));
    dataset.write(mean_returns_steps);
    dataset = file.createDataSet<DTYPE>("mean_returns", HighFive::DataSpace::From(mean_returns));
    dataset.write(mean_returns);
}
