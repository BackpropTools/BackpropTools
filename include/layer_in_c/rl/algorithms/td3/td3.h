#ifndef LAYER_IN_C_RL_ALGORITHMS_TD3
#define LAYER_IN_C_RL_ALGORITHMS_TD3

#include "replay_buffer.h"
#include <random>
#include <layer_in_c/nn/nn.h>
#include <layer_in_c/nn_models/operations_generic.h>
#include <layer_in_c/utils/polyak.h>
namespace lic = layer_in_c;

template <typename T>
struct DefaultTD3Parameters{
    static constexpr T GAMMA = 0.99;
    static constexpr uint32_t ACTOR_BATCH_SIZE = 32;
    static constexpr uint32_t CRITIC_BATCH_SIZE = 32;
    static constexpr T ACTOR_POLYAK = 0.005;
    static constexpr T CRITIC_POLYAK = 0.005;
    static constexpr T TARGET_NEXT_ACTION_NOISE_STD = 0.2;
    static constexpr T TARGET_NEXT_ACTION_NOISE_CLIP = 0.5;
};

template <typename T, int T_LAYER_1_DIM, int T_LAYER_2_DIM, lic::nn::activation_functions::ActivationFunction FN>
struct ActorNetworkSpecification{
    static constexpr int LAYER_1_DIM = T_LAYER_1_DIM;
    static constexpr int LAYER_2_DIM = T_LAYER_2_DIM;
    static constexpr lic::nn::activation_functions::ActivationFunction LAYER_1_FN = FN;
    static constexpr lic::nn::activation_functions::ActivationFunction LAYER_2_FN = FN;
    typedef lic::nn::optimizers::adam::DefaultParameters<T> ADAM_PARAMETERS;
};

template <typename T, int T_LAYER_1_DIM, int T_LAYER_2_DIM, lic::nn::activation_functions::ActivationFunction FN>
struct CriticNetworkSpecification{
    static constexpr int LAYER_1_DIM = T_LAYER_1_DIM;
    static constexpr int LAYER_2_DIM = T_LAYER_2_DIM;
    static constexpr lic::nn::activation_functions::ActivationFunction LAYER_1_FN = FN;
    static constexpr lic::nn::activation_functions::ActivationFunction LAYER_2_FN = FN;
    typedef lic::nn::optimizers::adam::DefaultParameters<T> ADAM_PARAMETERS;
};


template <
    typename T_T,
    typename T_ENVIRONMENT,
    typename T_ACTOR_SPEC,
    typename T_CRITIC_SPEC,
    typename T_PARAMETERS
>
struct ActorCriticSpecification{
    typedef T_T T;
    typedef T_ENVIRONMENT ENVIRONMENT;
    typedef T_ACTOR_SPEC ACTOR_SPEC;
    typedef T_CRITIC_SPEC CRITIC_SPEC;
    typedef T_PARAMETERS PARAMETERS;
};

template <typename DEVICE, typename T_SPEC>
struct ActorCritic{
    typedef T_SPEC SPEC;
    typedef typename SPEC::T T;

    typedef lic::nn_models::three_layer_fc::StructureSpecification<
        typename SPEC::T,
        SPEC::ENVIRONMENT::OBSERVATION_DIM,
        SPEC::ACTOR_SPEC::LAYER_1_DIM, SPEC::ACTOR_SPEC::LAYER_1_FN,
        SPEC::ACTOR_SPEC::LAYER_2_DIM, SPEC::ACTOR_SPEC::LAYER_2_FN,
        SPEC::ENVIRONMENT::ACTION_DIM, lic::nn::activation_functions::TANH> ACTOR_NETWORK_STRUCTURE_SPEC;

    typedef lic::nn_models::three_layer_fc::AdamSpecification<DEVICE, ACTOR_NETWORK_STRUCTURE_SPEC, typename SPEC::ACTOR_SPEC::ADAM_PARAMETERS> ACTOR_NETWORK_SPEC;
    typedef layer_in_c::nn_models::three_layer_fc::NeuralNetworkAdam<DEVICE, ACTOR_NETWORK_SPEC> ACTOR_NETWORK_TYPE;

    typedef lic::nn_models::three_layer_fc::InferenceSpecification<DEVICE, ACTOR_NETWORK_STRUCTURE_SPEC> ACTOR_TARGET_NETWORK_SPEC;
    typedef layer_in_c::nn_models::three_layer_fc::NeuralNetwork<DEVICE, ACTOR_TARGET_NETWORK_SPEC> ACTOR_TARGET_NETWORK_TYPE;

    static constexpr int CRITIC_INPUT_DIM = SPEC::ENVIRONMENT::OBSERVATION_DIM + SPEC::ENVIRONMENT::ACTION_DIM;
    typedef layer_in_c::nn_models::three_layer_fc::StructureSpecification<T, CRITIC_INPUT_DIM,
            SPEC::CRITIC_SPEC::LAYER_1_DIM, SPEC::CRITIC_SPEC::LAYER_1_FN,
            SPEC::CRITIC_SPEC::LAYER_2_DIM, SPEC::CRITIC_SPEC::LAYER_2_FN,
            1, layer_in_c::nn::activation_functions::LINEAR> CRITIC_NETWORK_STRUCTURE_SPEC;

    typedef lic::nn_models::three_layer_fc::AdamSpecification<DEVICE, CRITIC_NETWORK_STRUCTURE_SPEC, typename SPEC::CRITIC_SPEC::ADAM_PARAMETERS> CRITIC_NETWORK_SPEC;
    typedef layer_in_c::nn_models::three_layer_fc::NeuralNetworkAdam<DEVICE, CRITIC_NETWORK_SPEC> CRITIC_NETWORK_TYPE;

    typedef layer_in_c::nn_models::three_layer_fc::InferenceBackwardSpecification<DEVICE, CRITIC_NETWORK_STRUCTURE_SPEC> CRITIC_TARGET_INFERENCE_BACKWARD_NETWORK_SPEC;
    typedef layer_in_c::nn_models::three_layer_fc::NeuralNetworkBackward<DEVICE, CRITIC_TARGET_INFERENCE_BACKWARD_NETWORK_SPEC> CRITIC_TARGET_INFERENCE_BACKWARD_NETWORK_TYPE;

    typedef layer_in_c::nn_models::three_layer_fc::InferenceSpecification<DEVICE, CRITIC_NETWORK_STRUCTURE_SPEC> CRITIC_TARGET_INFERENCE_NETWORK_SPEC;
    typedef layer_in_c::nn_models::three_layer_fc::NeuralNetwork<DEVICE, CRITIC_TARGET_INFERENCE_NETWORK_SPEC> CRITIC_TARGET_INFERENCE_NETWORK_TYPE;

    ACTOR_NETWORK_TYPE actor;
    ACTOR_TARGET_NETWORK_TYPE actor_target;

    CRITIC_NETWORK_TYPE critic_1;
    CRITIC_NETWORK_TYPE critic_2;
    CRITIC_TARGET_INFERENCE_BACKWARD_NETWORK_TYPE critic_target_1;
    CRITIC_TARGET_INFERENCE_NETWORK_TYPE critic_target_2;
};

template<typename SPEC>
void update_target_layer(lic::nn::layers::dense::Layer<lic::devices::Generic, SPEC>& target, const lic::nn::layers::dense::Layer<lic::devices::Generic, SPEC>& source, typename SPEC::T polyak) {
    lic::utils::polyak::update_matrix<typename SPEC::T, SPEC::OUTPUT_DIM, SPEC::INPUT_DIM>(target.weights, source.weights, polyak);
    lic::utils::polyak::update       <typename SPEC::T, SPEC::OUTPUT_DIM                 >(target.biases , source.biases , polyak);
}
template<typename T, typename TARGET_NETWORK_TYPE, typename SOURCE_NETWORK_TYPE>
void update_target_network(TARGET_NETWORK_TYPE& target, const SOURCE_NETWORK_TYPE& source, T polyak) {
    update_target_layer(target.layer_1, source.layer_1, polyak);
    update_target_layer(target.layer_2, source.layer_2, polyak);
    update_target_layer(target.output_layer, source.output_layer, polyak);
}

template <typename DEVICE, typename SPEC>
void update_targets(ActorCritic<DEVICE, SPEC>& actor_critic) {
    update_target_network(actor_critic.actor_target   , actor_critic.   actor, SPEC::PARAMETERS::ACTOR_POLYAK);
    update_target_network(actor_critic.critic_target_1, actor_critic.critic_1, SPEC::PARAMETERS::CRITIC_POLYAK);
    update_target_network(actor_critic.critic_target_2, actor_critic.critic_2, SPEC::PARAMETERS::CRITIC_POLYAK);

}


template <typename DEVICE, typename SPEC, auto RANDOM_UNIFORM, typename RNG>
void init(ActorCritic<DEVICE, SPEC>& actor_critic, RNG& rng){
    layer_in_c::init_weights<typename ActorCritic<DEVICE, SPEC>:: ACTOR_NETWORK_SPEC, RANDOM_UNIFORM, RNG>(actor_critic.actor, rng);
    layer_in_c::init_weights<typename ActorCritic<DEVICE, SPEC>::CRITIC_NETWORK_SPEC, RANDOM_UNIFORM, RNG>(actor_critic.critic_1, rng);
    layer_in_c::init_weights<typename ActorCritic<DEVICE, SPEC>::CRITIC_NETWORK_SPEC, RANDOM_UNIFORM, RNG>(actor_critic.critic_2, rng);
    layer_in_c::reset_optimizer_state(actor_critic.actor);
    layer_in_c::reset_optimizer_state(actor_critic.critic_1);
    layer_in_c::reset_optimizer_state(actor_critic.critic_2);
    // Target networks still need to be initialised because they could be none which could destroy the use of the polyak update for assignment
    layer_in_c::init_weights<typename ActorCritic<DEVICE, SPEC>::ACTOR_TARGET_NETWORK_SPEC                    , RANDOM_UNIFORM, RNG>(actor_critic.actor_target, rng);
    layer_in_c::init_weights<typename ActorCritic<DEVICE, SPEC>::CRITIC_TARGET_INFERENCE_BACKWARD_NETWORK_SPEC, RANDOM_UNIFORM, RNG>(actor_critic.critic_target_1, rng);
    layer_in_c::init_weights<typename ActorCritic<DEVICE, SPEC>::CRITIC_TARGET_INFERENCE_NETWORK_SPEC         , RANDOM_UNIFORM, RNG>(actor_critic.critic_target_2, rng);
    update_target_network(actor_critic.actor_target, actor_critic.actor, (typename SPEC::T)0);
    update_target_network(actor_critic.critic_target_1, actor_critic.critic_1, (typename SPEC::T)0);
    update_target_network(actor_critic.critic_target_2, actor_critic.critic_2, (typename SPEC::T)0);
}


template <typename DEVICE, typename SPEC, typename CRITIC_TYPE, int CAPACITY, typename RNG>
typename SPEC::T train_critic(ActorCritic<DEVICE, SPEC>& actor_critic, CRITIC_TYPE& critic, ReplayBuffer<typename SPEC::T, SPEC::ENVIRONMENT::OBSERVATION_DIM, SPEC::ENVIRONMENT::ACTION_DIM, CAPACITY>& replay_buffer, RNG& rng, bool deterministic = false) {
    typedef typename SPEC::T T;
    assert(replay_buffer.full || replay_buffer.position >= SPEC::PARAMETERS::CRITIC_BATCH_SIZE);
    T loss = 0;
    lic::zero_gradient(critic);
    std::uniform_int_distribution<uint32_t> sample_distribution(0, (replay_buffer.full ? CAPACITY : replay_buffer.position) - 1);
    for (int sample_i=0; sample_i < SPEC::PARAMETERS::CRITIC_BATCH_SIZE; sample_i++){
        uint32_t sample_index = sample_distribution(rng);
        T next_state_action_value_input[SPEC::ENVIRONMENT::OBSERVATION_DIM + SPEC::ENVIRONMENT::ACTION_DIM];
        memcpy(next_state_action_value_input, replay_buffer.next_observations[sample_index], sizeof(T) * SPEC::ENVIRONMENT::OBSERVATION_DIM); // setting the first part with next observations
        lic::evaluate(actor_critic.actor_target, next_state_action_value_input, &next_state_action_value_input[SPEC::ENVIRONMENT::OBSERVATION_DIM]); // setting the second part with next actions
        std::normal_distribution<T> target_next_action_noise_distribution(0, SPEC::PARAMETERS::TARGET_NEXT_ACTION_NOISE_STD);
        for(int action_i=0; action_i < SPEC::ENVIRONMENT::ACTION_DIM; action_i++){
            T action_noise = deterministic ? 0 : std::clamp(
                    target_next_action_noise_distribution(rng),
                    -SPEC::PARAMETERS::TARGET_NEXT_ACTION_NOISE_CLIP,
                    SPEC::PARAMETERS::TARGET_NEXT_ACTION_NOISE_CLIP
            );
            T noisy_next_action = next_state_action_value_input[SPEC::ENVIRONMENT::OBSERVATION_DIM + action_i] + action_noise;
            noisy_next_action = std::clamp<T>(noisy_next_action, -1, 1);
            next_state_action_value_input[SPEC::ENVIRONMENT::OBSERVATION_DIM + action_i] = noisy_next_action;
        }
        T next_state_action_value_critic_1 = lic::evaluate(actor_critic.critic_target_1, next_state_action_value_input);
        T next_state_action_value_critic_2 = lic::evaluate(actor_critic.critic_target_2, next_state_action_value_input);

        T min_next_state_action_value = std::min(
            next_state_action_value_critic_1,
            next_state_action_value_critic_2
        );
        T state_action_value_input[SPEC::ENVIRONMENT::OBSERVATION_DIM + SPEC::ENVIRONMENT::ACTION_DIM];
        memcpy(state_action_value_input, replay_buffer.observations[sample_index], sizeof(T) * SPEC::ENVIRONMENT::OBSERVATION_DIM); // setting the first part with the current observation
        memcpy(&state_action_value_input[SPEC::ENVIRONMENT::OBSERVATION_DIM], replay_buffer.actions[sample_index], sizeof(T) * SPEC::ENVIRONMENT::ACTION_DIM); // setting the first part with the current action
//        standardise<T,  OBSERVATION_DIM>(X_train[batch_i * batch_size + sample_i].data(), X_mean.data(), X_std.data(), input);
//        standardise<T, ACTION_DIM>(Y_train[batch_i * batch_size + sample_i].data(), Y_mean.data(), Y_std.data(), output);
        T target_action_value[1] = {replay_buffer.rewards[sample_index] + SPEC::PARAMETERS::GAMMA * min_next_state_action_value * (!replay_buffer.terminated[sample_index])};

        lic::forward_backward_mse(critic, state_action_value_input, target_action_value);
        loss += lic::nn::loss_functions::mse<T, 1>(critic.output_layer.output, target_action_value);
    }
    loss /= SPEC::PARAMETERS::CRITIC_BATCH_SIZE;
    lic::update(critic);
    return loss;
}

template <typename DEVICE, typename SPEC, int CAPACITY, typename RNG>
typename SPEC::T train_actor(ActorCritic<DEVICE, SPEC>& actor_critic, ReplayBuffer<typename SPEC::T, SPEC::ENVIRONMENT::OBSERVATION_DIM, SPEC::ENVIRONMENT::ACTION_DIM, CAPACITY>& replay_buffer, RNG& rng) {
    typedef typename SPEC::T T;
    typedef typename SPEC::PARAMETERS PARAMETERS;
    typedef typename SPEC::ENVIRONMENT ENVIRONMENT;
    T actor_value = 0;
    zero_gradient(actor_critic.actor);
    std::uniform_int_distribution<uint32_t> sample_distribution(0, (replay_buffer.full ? CAPACITY : replay_buffer.position) - 1);
    for (int sample_i=0; sample_i < PARAMETERS::ACTOR_BATCH_SIZE; sample_i++){
        uint32_t sample_index = sample_distribution(rng);
        T state_action_value_input[ENVIRONMENT::OBSERVATION_DIM + ENVIRONMENT::ACTION_DIM];
        memcpy(state_action_value_input, replay_buffer.observations[sample_index], sizeof(T) * ENVIRONMENT::OBSERVATION_DIM); // setting the first part with next observations
        evaluate(actor_critic.actor_target, state_action_value_input, &state_action_value_input[ENVIRONMENT::OBSERVATION_DIM]); // setting the second part with next actions

        forward(actor_critic.critic_target_1, state_action_value_input);
        actor_value += actor_critic.critic_target_1.output_layer.output[0];
        T d_output[1] = {-1}; // we want to maximise the critic output using gradient descent
        T d_critic_input[ENVIRONMENT::OBSERVATION_DIM + ENVIRONMENT::ACTION_DIM];
        backward(actor_critic.critic_target_1, state_action_value_input, d_output, d_critic_input);
        T d_actor_input[ENVIRONMENT::OBSERVATION_DIM];
        backward(actor_critic.actor, state_action_value_input, &d_critic_input[ENVIRONMENT::OBSERVATION_DIM], d_actor_input);
    }
    actor_value /= PARAMETERS::ACTOR_BATCH_SIZE;
    update(actor_critic.actor);
    return actor_value;
}


#endif