#include <gtest/gtest.h>
#include <highfive/H5File.hpp>

#include <layer_in_c/rl/environments/operations_cpu.h>
#include <layer_in_c/rl/algorithms/td3/operations_cpu.h>

#include <layer_in_c/nn_models/persist.h>
#include <layer_in_c/rl/utils/evaluation.h>

#include "../../../utils/utils.h"
#include "../../../utils/nn_comparison_mlp.h"

#ifdef LAYER_IN_C_TEST_RL_ALGORITHMS_TD3_SECOND_STAGE_EVALUATE_VISUALLY
#include <layer_in_c/rl/environments/pendulum/ui.h>
#include <layer_in_c/rl/utils/evaluation_visual.h>
#endif

#ifdef LAYER_IN_C_TEST_RL_ALGORITHMS_TD3_SECOND_STAGE_OUTPUT_PLOTS
#include "plot_policy_and_value_function.h"
#endif

namespace lic = layer_in_c;
std::string get_data_file_path(){
    std::string DATA_FILE_PATH = "../multirotor-torch/model_second_stage.hdf5";
    const char* data_file_path = std::getenv("LAYER_IN_C_TEST_RL_ALGORITHMS_TD3_SECOND_STAGE_DATA_FILE");
    if (data_file_path != NULL){
        DATA_FILE_PATH = std::string(data_file_path);
//            std::runtime_error("Environment variable LAYER_IN_C_TEST_DATA_DIR not set. Skipping test.");
    }
    return DATA_FILE_PATH;
}
#define DTYPE double
typedef lic::rl::environments::pendulum::Specification<DTYPE, lic::rl::environments::pendulum::DefaultParameters<DTYPE>> PENDULUM_SPEC;
typedef lic::rl::environments::Pendulum<lic::devices::CPU, PENDULUM_SPEC> ENVIRONMENT;
#ifdef LAYER_IN_C_TEST_RL_ALGORITHMS_TD3_SECOND_STAGE_EVALUATE_VISUALLY
typedef lic::rl::environments::pendulum::UI<DTYPE> UI;
#endif
ENVIRONMENT env;

template <typename T>
struct TD3Parameters: public lic::rl::algorithms::td3::DefaultParameters<T>{
    constexpr static int CRITIC_BATCH_SIZE = 32;
    constexpr static int ACTOR_BATCH_SIZE = 32;
};
struct ActorStructureSpec{
    using T = DTYPE;
    static constexpr size_t INPUT_DIM = ENVIRONMENT::OBSERVATION_DIM;
    static constexpr size_t OUTPUT_DIM = ENVIRONMENT::ACTION_DIM;
    static constexpr int NUM_LAYERS = 3;
    static constexpr int HIDDEN_DIM = 64;
    static constexpr lic::nn::activation_functions::ActivationFunction HIDDEN_ACTIVATION_FUNCTION = lic::nn::activation_functions::RELU;
    static constexpr lic::nn::activation_functions::ActivationFunction OUTPUT_ACTIVATION_FUNCTION = lic::nn::activation_functions::TANH;
};

struct CriticStructureSpec{
    using T = DTYPE;
    static constexpr size_t INPUT_DIM = ENVIRONMENT::OBSERVATION_DIM + ENVIRONMENT::ACTION_DIM;
    static constexpr size_t OUTPUT_DIM = 1;
    static constexpr int NUM_LAYERS = 3;
    static constexpr int HIDDEN_DIM = 64;
    static constexpr lic::nn::activation_functions::ActivationFunction HIDDEN_ACTIVATION_FUNCTION = lic::nn::activation_functions::RELU;
    static constexpr lic::nn::activation_functions::ActivationFunction OUTPUT_ACTIVATION_FUNCTION = lic::nn::activation_functions::IDENTITY;
};

template <typename T>
struct TD3ParametersCopyTraining: public lic::rl::algorithms::td3::DefaultParameters<T>{
    constexpr static int CRITIC_BATCH_SIZE = 100;
    constexpr static int ACTOR_BATCH_SIZE = 100;
};

using NN_DEVICE = lic::devices::CPU;
using ACTOR_NETWORK_SPEC = lic::nn_models::mlp::AdamSpecification<NN_DEVICE, ActorStructureSpec, typename lic::nn::optimizers::adam::DefaultParametersTorch<DTYPE>>;
using ACTOR_NETWORK_TYPE = lic::nn_models::mlp::NeuralNetworkAdam<NN_DEVICE, ACTOR_NETWORK_SPEC>;

using ACTOR_TARGET_NETWORK_SPEC = lic::nn_models::mlp::InferenceSpecification<NN_DEVICE, ActorStructureSpec>;
using ACTOR_TARGET_NETWORK_TYPE = layer_in_c::nn_models::mlp::NeuralNetwork<NN_DEVICE , ACTOR_TARGET_NETWORK_SPEC>;

using CRITIC_NETWORK_SPEC = lic::nn_models::mlp::AdamSpecification<NN_DEVICE, CriticStructureSpec, typename lic::nn::optimizers::adam::DefaultParametersTorch<DTYPE>>;
using CRITIC_NETWORK_TYPE = layer_in_c::nn_models::mlp::NeuralNetworkAdam<NN_DEVICE, CRITIC_NETWORK_SPEC>;

using CRITIC_TARGET_NETWORK_SPEC = layer_in_c::nn_models::mlp::InferenceSpecification<NN_DEVICE, CriticStructureSpec>;
using CRITIC_TARGET_NETWORK_TYPE = layer_in_c::nn_models::mlp::NeuralNetwork<NN_DEVICE, CRITIC_TARGET_NETWORK_SPEC>;

using TD3_SPEC = lic::rl::algorithms::td3::Specification<DTYPE, ENVIRONMENT, ACTOR_NETWORK_TYPE, ACTOR_TARGET_NETWORK_TYPE, CRITIC_NETWORK_TYPE, CRITIC_TARGET_NETWORK_TYPE, TD3ParametersCopyTraining<DTYPE>>;
using ActorCriticType = lic::rl::algorithms::td3::ActorCritic<lic::devices::CPU, TD3_SPEC>;


TEST(LAYER_IN_C_RL_ALGORITHMS_TD3_MLP_SECOND_STAGE, TEST_LOADING_TRAINED_ACTOR) {
    constexpr bool verbose = false;
    ActorCriticType actor_critic;

    std::mt19937 rng(0);

    auto data_file = HighFive::File(get_data_file_path(), HighFive::File::ReadOnly);
    int step = data_file.getGroup("full_training").getGroup("steps").getNumberObjects()-1;
    assert(step >= 0);
    auto step_group = data_file.getGroup("full_training").getGroup("steps").getGroup(std::to_string(step));
    lic::load(actor_critic.actor, step_group.getGroup("actor"));
    DTYPE mean_return = lic::evaluate<ENVIRONMENT, decltype(actor_critic.actor), typeof(rng), 200, true>(ENVIRONMENT(), actor_critic.actor, 100, rng);
    std::cout << "mean return: " << mean_return << std::endl;
}

using ReplayBufferSpecCopyTraining = lic::rl::components::replay_buffer::Spec<DTYPE, 3, 1, 1000>;
typedef lic::rl::components::ReplayBuffer<lic::devices::CPU, ReplayBufferSpecCopyTraining> ReplayBufferTypeCopyTraining;
constexpr int BATCH_DIM = ENVIRONMENT::OBSERVATION_DIM * 2 + ENVIRONMENT::ACTION_DIM + 2;
template <typename T, typename REPLAY_BUFFER_TYPE>
void load(ReplayBufferTypeCopyTraining& rb, std::vector<std::vector<T>> batch){
    for(int i = 0; i < batch.size(); i++){
        memcpy( rb.     observations[i], &batch[i][0], sizeof(T) * ENVIRONMENT::OBSERVATION_DIM);
        memcpy( rb.          actions[i], &batch[i][ENVIRONMENT::OBSERVATION_DIM], sizeof(T) * ENVIRONMENT::ACTION_DIM);
        memcpy( rb.next_observations[i], &batch[i][ENVIRONMENT::OBSERVATION_DIM + ENVIRONMENT::ACTION_DIM], sizeof(T) * ENVIRONMENT::OBSERVATION_DIM);
        rb.   rewards[i] = batch[i][ENVIRONMENT::OBSERVATION_DIM + ENVIRONMENT::ACTION_DIM + ENVIRONMENT::OBSERVATION_DIM    ];
        rb.terminated[i] = batch[i][ENVIRONMENT::OBSERVATION_DIM + ENVIRONMENT::ACTION_DIM + ENVIRONMENT::OBSERVATION_DIM + 1] == 1;
        rb. truncated[i] = batch[i][ENVIRONMENT::OBSERVATION_DIM + ENVIRONMENT::ACTION_DIM + ENVIRONMENT::OBSERVATION_DIM + 2] == 1;
    }
    rb.position = batch.size();
}
TEST(LAYER_IN_C_RL_ALGORITHMS_TD3_MLP_SECOND_STAGE, FP_ACC) {
    for(int i = 0; i < 1000; i++){
        std::normal_distribution<float> dist;
        auto rng = std::mt19937(0);
        float a = dist(rng) * 5e-3;
        float b = dist(rng) / 10;
        float aa = dist(rng);

        float c = a * b;
        c += aa;
        float d = c / b;
        d -= aa / b;
        float e = a - d;

//        std::cout << e << std::endl;
    }
    for(int i = 0; i < 1000; i++){
        std::normal_distribution<double> dist;
        auto rng = std::mt19937(0);
        double a = dist(rng) * 5e-3;
        double b = dist(rng) / 10;
        double aa = dist(rng);

        double c = a * b;
        c += aa;
        double d = c / b;
        d -= aa / b;
        double e = a - d;

//        std::cout << e << std::endl;
    }
}
TEST(LAYER_IN_C_RL_ALGORITHMS_TD3_MLP_SECOND_STAGE, TEST_COPY_TRAINING) {
#ifdef LAYER_IN_C_TEST_RL_ALGORITHMS_TD3_SECOND_STAGE_EVALUATE_VISUALLY
    UI ui;
#endif
    constexpr bool verbose = true;
    ActorCriticType actor_critic;

    std::mt19937 rng(0);
    lic::init(actor_critic, rng);



    auto data_file = HighFive::File(get_data_file_path(), HighFive::File::ReadOnly);
    lic::load(actor_critic.actor, data_file.getGroup("actor"));
    lic::load(actor_critic.actor_target, data_file.getGroup("actor_target"));
    lic::load(actor_critic.critic_1, data_file.getGroup("critic_1"));
    lic::load(actor_critic.critic_target_1, data_file.getGroup("critic_target_1"));
    lic::load(actor_critic.critic_2, data_file.getGroup("critic_2"));
    lic::load(actor_critic.critic_target_2, data_file.getGroup("critic_target_2"));

    ReplayBufferTypeCopyTraining replay_buffer;

    lic::reset_optimizer_state(actor_critic.actor);
    lic::reset_optimizer_state(actor_critic.critic_1);
    lic::reset_optimizer_state(actor_critic.critic_2);
    DTYPE mean_ratio_critic = 0;
    DTYPE mean_ratio_critic_grad = 0;
    DTYPE mean_ratio_critic_adam = 0;
    DTYPE mean_ratio_actor = 0;
    DTYPE mean_ratio_actor_grad = 0;
    DTYPE mean_ratio_actor_adam = 0;
    DTYPE mean_ratio_critic_target = 0;
    auto full_training_group = data_file.getGroup("full_training");
    auto steps_group = full_training_group.getGroup("steps");
    int num_steps = std::min(steps_group.getNumberObjects(), (size_t)1000);
    auto pre_critic_1 = actor_critic.critic_1;
    auto pre_actor = actor_critic.actor;
    auto pre_critic_1_target = actor_critic.critic_target_1;
    for(int step_i = 0; step_i < num_steps; step_i++){
        if(verbose){
            std::cout << "step_i: " << step_i << std::endl;
        }
        auto step_group = steps_group.getGroup(std::to_string(step_i));
        if(step_group.exist("critics_batch")){
            std::vector<std::vector<DTYPE>> batch;
            step_group.getDataSet("critics_batch").read(batch);
            assert(batch.size() == ActorCriticType::SPEC::PARAMETERS::CRITIC_BATCH_SIZE);

            DTYPE target_next_action_noise[ActorCriticType::SPEC::PARAMETERS::CRITIC_BATCH_SIZE][ActorCriticType::SPEC::ENVIRONMENT::ACTION_DIM];
            step_group.getDataSet("target_next_action_noise").read(target_next_action_noise);

            load<DTYPE, ReplayBufferTypeCopyTraining>(replay_buffer, batch);
            if (step_i == 0 && step_group.exist("pre_critic1")){
                decltype(actor_critic.critic_1) pre_critic_1_step;
                lic::load(pre_critic_1_step, step_group.getGroup("pre_critic1"));
                DTYPE pre_current_diff = abs_diff(pre_critic_1_step, actor_critic.critic_1);
                ASSERT_EQ(pre_current_diff, 0);
            }

            decltype(actor_critic.critic_1) post_critic_1;// = actor_critic.critic_1;
            lic::load(post_critic_1, step_group.getGroup("critic1"));

            DTYPE critic_1_loss = lic::train_critic<
                    decltype(actor_critic)::SPEC,
                    decltype(actor_critic.critic_1),
                    decltype(replay_buffer)::DEVICE,
                    decltype(replay_buffer)::CAPACITY,
                    decltype(rng),
                    true
            >(actor_critic, actor_critic.critic_1, replay_buffer, target_next_action_noise, rng);


            DTYPE pre_post_diff_per_weight = abs_diff(pre_critic_1, post_critic_1)/ActorCriticType::SPEC::CRITIC_NETWORK_TYPE::NUM_WEIGHTS;
            DTYPE diff_target_per_weight = abs_diff(post_critic_1, actor_critic.critic_1)/ActorCriticType::SPEC::CRITIC_NETWORK_TYPE::NUM_WEIGHTS;
            DTYPE diff_ratio = pre_post_diff_per_weight/diff_target_per_weight;

            DTYPE pre_post_diff_grad_per_weight = abs_diff_grad(pre_critic_1, post_critic_1)/ActorCriticType::SPEC::CRITIC_NETWORK_TYPE::NUM_WEIGHTS;
            DTYPE diff_target_grad_per_weight = abs_diff_grad(post_critic_1, actor_critic.critic_1)/ActorCriticType::SPEC::CRITIC_NETWORK_TYPE::NUM_WEIGHTS;
            DTYPE diff_ratio_grad = pre_post_diff_grad_per_weight/diff_target_grad_per_weight;

            DTYPE pre_post_diff_adam_per_weight = abs_diff_adam(pre_critic_1, post_critic_1)/ActorCriticType::SPEC::CRITIC_NETWORK_TYPE::NUM_WEIGHTS;
            DTYPE diff_target_adam_per_weight = abs_diff_adam(post_critic_1, actor_critic.critic_1)/ActorCriticType::SPEC::CRITIC_NETWORK_TYPE::NUM_WEIGHTS;
            DTYPE diff_ratio_adam = pre_post_diff_adam_per_weight/diff_target_adam_per_weight;

            if(verbose){
                std:: cout << "    critic update" << std::endl;
//                std::cout << "pre_post_diff_per_weight: " << pre_post_diff_per_weight << std::endl;
//                std::cout << "diff_target_per_weight: " << diff_target_per_weight << std::endl;
                std::cout << "        update ratio     : " << diff_ratio << std::endl;
//                std::cout << "pre_post_diff_grad_per_weight: " << pre_post_diff_grad_per_weight << std::endl;
//                std::cout << "diff_target_grad_per_weight: " << diff_target_grad_per_weight << std::endl;
                std::cout << "        update ratio grad: " << diff_ratio_grad << std::endl;
//                std::cout << "pre_post_diff_adam_per_weight: " << pre_post_diff_adam_per_weight << std::endl;
//                std::cout << "diff_target_adam_per_weight: " << diff_target_adam_per_weight << std::endl;
                std::cout << "        update ratio adam: " << diff_ratio_adam << std::endl;
            }
            if(diff_ratio < 1e10){
//                std::cout << "something wrong here" << std::endl;
            }

            switch(step_i){
                case 0: {
                    ASSERT_GT(diff_ratio, 1e6);
                    ASSERT_GT(diff_ratio_grad, 1e6);
                    ASSERT_GT(diff_ratio_adam, 1e6);
                }
                    break;
            }

            mean_ratio_critic += diff_ratio;
            mean_ratio_critic_grad += diff_ratio_grad;
            mean_ratio_critic_adam += diff_ratio_adam;

            DTYPE critic_2_loss = lic::train_critic<
                decltype(actor_critic)::SPEC,
                decltype(actor_critic.critic_2),
                decltype(replay_buffer)::DEVICE,
                decltype(replay_buffer)::CAPACITY,
                decltype(rng),
                true
            >(actor_critic, actor_critic.critic_2, replay_buffer, target_next_action_noise, rng);
            pre_critic_1 = actor_critic.critic_1;

            if(true){//(step_i % 100 == 0){
                DTYPE diff = 0;
                for(int batch_sample_i = 0; batch_sample_i < ActorCriticType::SPEC::PARAMETERS::CRITIC_BATCH_SIZE; batch_sample_i++){
                    DTYPE input[ActorCriticType::SPEC::ENVIRONMENT::OBSERVATION_DIM + ActorCriticType::SPEC::ENVIRONMENT::ACTION_DIM];
                    memcpy(input, replay_buffer.observations[batch_sample_i], sizeof(DTYPE) * ActorCriticType::SPEC::ENVIRONMENT::OBSERVATION_DIM);
                    memcpy(&input[ActorCriticType::SPEC::ENVIRONMENT::OBSERVATION_DIM], replay_buffer.actions[batch_sample_i], sizeof(DTYPE) * ActorCriticType::SPEC::ENVIRONMENT::ACTION_DIM);
                    DTYPE current_value = lic::evaluate(actor_critic.critic_1, input);
                    DTYPE desired_value = lic::evaluate(post_critic_1, input);
                    diff += (current_value - desired_value) * (current_value - desired_value) / ActorCriticType::SPEC::PARAMETERS::CRITIC_BATCH_SIZE;
                }
//                std::cout << "value mse: " << diff << std::endl;
            }
        }

        if(step_group.exist("actor_batch")){
            std::vector<std::vector<DTYPE>> batch;
            step_group.getDataSet("actor_batch").read(batch);
            assert(batch.size() == ActorCriticType::SPEC::PARAMETERS::ACTOR_BATCH_SIZE);
            load<DTYPE, ReplayBufferTypeCopyTraining>(replay_buffer, batch);

            decltype(actor_critic.actor) post_actor;
            lic::load(post_actor, step_group.getGroup("actor"));

            decltype(actor_critic.actor) pre_actor_loaded;
            lic::load(pre_actor_loaded, step_group.getGroup("pre_actor"));
            DTYPE pre_current_diff = abs_diff(pre_actor_loaded, actor_critic.actor);
            if(step_i == 0){
                ASSERT_EQ(pre_current_diff, 0);
            }


            DTYPE actor_loss = lic::train_actor<ActorCriticType::SPEC, decltype(replay_buffer)::DEVICE, decltype(replay_buffer)::CAPACITY, typeof(rng), true>(actor_critic, replay_buffer, rng);

            if(true){//(step_i % 100 == 1){
                DTYPE diff = 0;
                for(int batch_sample_i = 0; batch_sample_i < ActorCriticType::SPEC::PARAMETERS::ACTOR_BATCH_SIZE; batch_sample_i++){
                    DTYPE current_action[ActorCriticType::SPEC::ENVIRONMENT::ACTION_DIM];
                    lic::evaluate(actor_critic.actor, replay_buffer.observations[batch_sample_i], current_action);
                    DTYPE desired_action[ActorCriticType::SPEC::ENVIRONMENT::ACTION_DIM];
                    lic::evaluate(post_actor, replay_buffer.observations[batch_sample_i], desired_action);
                    diff += lic::nn::loss_functions::mse<DTYPE, ActorCriticType::SPEC::ENVIRONMENT::ACTION_DIM, ActorCriticType::SPEC::PARAMETERS::ACTOR_BATCH_SIZE>(current_action, desired_action);
                }
//                std::cout << "action mse: " << diff << std::endl;
            }

            DTYPE pre_post_diff_per_weight = abs_diff(pre_actor, post_actor)/ActorCriticType::SPEC::ACTOR_NETWORK_TYPE::NUM_WEIGHTS;
            DTYPE diff_target_per_weight = abs_diff(post_actor, actor_critic.actor)/ActorCriticType::SPEC::ACTOR_NETWORK_TYPE::NUM_WEIGHTS;
            DTYPE diff_ratio = pre_post_diff_per_weight/diff_target_per_weight;

            DTYPE pre_post_diff_grad_per_weight = abs_diff_grad(pre_actor, post_actor)/ActorCriticType::SPEC::ACTOR_NETWORK_TYPE::NUM_WEIGHTS;
            DTYPE diff_target_grad_per_weight = abs_diff_grad(post_actor, actor_critic.actor)/ActorCriticType::SPEC::ACTOR_NETWORK_TYPE::NUM_WEIGHTS;
            DTYPE diff_ratio_grad = pre_post_diff_grad_per_weight/diff_target_grad_per_weight;

            DTYPE pre_post_diff_adam_per_weight = abs_diff_adam(pre_actor, post_actor)/ActorCriticType::SPEC::ACTOR_NETWORK_TYPE::NUM_WEIGHTS;
            DTYPE diff_target_adam_per_weight = abs_diff_adam(post_actor, actor_critic.actor)/ActorCriticType::SPEC::ACTOR_NETWORK_TYPE::NUM_WEIGHTS;
            DTYPE diff_ratio_adam = pre_post_diff_adam_per_weight/diff_target_adam_per_weight;

            if(verbose){
                std:: cout << "    actor update" << std::endl;
//                std::cout << "        pre_post_diff_per_weight: " << pre_post_diff_per_weight << std::endl;
//                std::cout << "        diff_target_per_weight: " << diff_target_per_weight << std::endl;
                std::cout << "        update ratio     : " << diff_ratio << std::endl;

//                std::cout << "        pre_post_diff_grad_per_weight: " << pre_post_diff_grad_per_weight << std::endl;
//                std::cout << "        diff_target_grad_per_weight: " << diff_target_grad_per_weight << std::endl;
                std::cout << "        update ratio grad: " << diff_ratio_grad << std::endl;

//                std::cout << "        pre_post_diff_adam_per_weight: " << pre_post_diff_adam_per_weight << std::endl;
//                std::cout << "        diff_target_adam_per_weight: " << diff_target_adam_per_weight << std::endl;
                std::cout << "        update ratio adam: " << diff_ratio_adam << std::endl;
            }

            switch(step_i){
                case 0: {
                    ASSERT_GT(diff_ratio, 1e6);
                    ASSERT_GT(diff_ratio_grad, 1e6);
                    ASSERT_GT(diff_ratio_adam, 1e6);
                }
                break;
            }

            mean_ratio_actor += diff_ratio;
            mean_ratio_actor_grad += diff_ratio_grad;
            mean_ratio_actor_adam += diff_ratio_adam;

            pre_actor = actor_critic.actor;
        }
        if(step_group.exist("critic1_target")){
            if(verbose){
                std:: cout << "    target update" << std::endl;
            }
            if (step_i == 0){
                decltype(actor_critic.critic_target_1) pre_critic_1_target_step;
                lic::load(pre_critic_1_target_step, step_group.getGroup("pre_critic1_target"));
                DTYPE pre_current_diff = abs_diff(pre_critic_1_target_step, actor_critic.critic_target_1);
                ASSERT_EQ(pre_current_diff, 0);
            }
            else{
                if (step_i >= ActorCriticType::SPEC::PARAMETERS::CRITIC_BATCH_SIZE){

                    decltype(actor_critic.critic_target_1) post_critic_1_target;
                    lic::load(post_critic_1_target, step_group.getGroup("critic1_target"));

                    lic::update_targets(actor_critic);

                    DTYPE pre_post_diff_per_weight = abs_diff(pre_critic_1_target, post_critic_1_target)/ActorCriticType::SPEC::CRITIC_NETWORK_TYPE::NUM_WEIGHTS;
                    DTYPE diff_target_per_weight = abs_diff(post_critic_1_target, actor_critic.critic_target_1)/ActorCriticType::SPEC::CRITIC_NETWORK_TYPE::NUM_WEIGHTS;
                    DTYPE diff_ratio = pre_post_diff_per_weight/diff_target_per_weight;

                    if(verbose){
                        std::cout << "    critic target update" << std::endl;
//                        std::cout << "        pre_post_diff_per_weight: " << pre_post_diff_per_weight << std::endl;
//                        std::cout << "        diff_target_per_weight: " << diff_target_per_weight << std::endl;
                        std::cout << "        update ratio     : " << diff_ratio << std::endl;
                    }

                    switch(step_i){
                        case 0: {
                            ASSERT_GT(diff_ratio, 1e6);
                        }
                            break;
                    }

                    mean_ratio_critic_target += diff_ratio;

                    pre_critic_1_target = actor_critic.critic_target_1;

                    if(true){//(step_i % 100 == 0){
                        DTYPE diff = 0;
                        for(int batch_sample_i = 0; batch_sample_i < ActorCriticType::SPEC::PARAMETERS::CRITIC_BATCH_SIZE; batch_sample_i++){
                            DTYPE input[ActorCriticType::SPEC::ENVIRONMENT::OBSERVATION_DIM + ActorCriticType::SPEC::ENVIRONMENT::ACTION_DIM];
                            memcpy(input, replay_buffer.observations[batch_sample_i], sizeof(DTYPE) * ActorCriticType::SPEC::ENVIRONMENT::OBSERVATION_DIM);
                            memcpy(&input[ActorCriticType::SPEC::ENVIRONMENT::OBSERVATION_DIM], replay_buffer.actions[batch_sample_i], sizeof(DTYPE) * ActorCriticType::SPEC::ENVIRONMENT::ACTION_DIM);
                            DTYPE current_value = lic::evaluate(actor_critic.critic_target_1, input);
                            DTYPE desired_value = lic::evaluate(post_critic_1_target, input);
                            diff += (current_value - desired_value) * (current_value - desired_value) / ActorCriticType::SPEC::PARAMETERS::CRITIC_BATCH_SIZE;
                        }
//                        std::cout << "value mse: " << diff << std::endl;
                    }
                }
            }

        }
        if(step_i % 100 == 0){
            if(!verbose){
                std::cout << "step_i: " << step_i << std::endl;
            }
            DTYPE mean_return = lic::evaluate<ENVIRONMENT, decltype(actor_critic.actor), typeof(rng), 200, true>(ENVIRONMENT(), actor_critic.actor, 100, rng);
#ifdef LAYER_IN_C_TEST_RL_ALGORITHMS_TD3_SECOND_STAGE_OUTPUT_PLOTS
            plot_policy_and_value_function<DTYPE, ENVIRONMENT, ActorCriticType::ACTOR_NETWORK_TYPE, ActorCriticType::CRITIC_NETWORK_TYPE>(actor_critic.actor, actor_critic.critic_1, std::string("second_stage"), step_i);
#endif
#ifdef LAYER_IN_C_TEST_RL_ALGORITHMS_TD3_SECOND_STAGE_EVALUATE_VISUALLY
            if(mean_return > -400){
                while(true){
                    ENVIRONMENT::State initial_state;
                    lic::sample_initial_state(env, initial_state, rng);
                    lic::evaluate_visual<ENVIRONMENT, UI, decltype(actor_critic.actor), 100, 3>(ENVIRONMENT(), ui, actor_critic.actor, initial_state);
                }
            }
#endif
        }
    }
    mean_ratio_critic /= num_steps;
    mean_ratio_critic_grad /= num_steps;
    mean_ratio_critic_adam /= num_steps;
    mean_ratio_actor /= num_steps;
    mean_ratio_actor_grad /= num_steps;
    mean_ratio_actor_adam /= num_steps;
    mean_ratio_critic_target /= num_steps;
    std::cout << "mean_ratio_critic: " << mean_ratio_critic << std::endl;
    std::cout << "mean_ratio_critic_grad: " << mean_ratio_critic_grad << std::endl;
    std::cout << "mean_ratio_critic_adam: " << mean_ratio_critic_adam << std::endl;
    std::cout << "mean_ratio_actor: " << mean_ratio_actor << std::endl;
    std::cout << "mean_ratio_actor_grad: " << mean_ratio_actor_grad << std::endl;
    std::cout << "mean_ratio_actor_adam: " << mean_ratio_actor_adam << std::endl;
    std::cout << "mean_ratio_critic_target: " << mean_ratio_critic_target << std::endl;
    ASSERT_GT(mean_ratio_critic, 1e12);
    ASSERT_GT(mean_ratio_critic_grad, 1e13);
    ASSERT_GT(mean_ratio_critic_adam, 1e12);
    ASSERT_GT(mean_ratio_actor, 1e12);
    ASSERT_GT(mean_ratio_actor_grad, 1e12);
    ASSERT_GT(mean_ratio_actor_adam, 1e12);
    ASSERT_GT(mean_ratio_critic_target, 1e11);
}
