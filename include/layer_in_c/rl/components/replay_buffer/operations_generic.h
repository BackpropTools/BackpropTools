#ifndef LAYER_IN_C_RL_COMPONENTS_REPLAY_BUFFER_OPERATIONS_GENERIC_H
#define LAYER_IN_C_RL_COMPONENTS_REPLAY_BUFFER_OPERATIONS_GENERIC_H

#include "replay_buffer.h"
#include <layer_in_c/utils/generic/memcpy.h>

namespace layer_in_c {
    template <typename DEVICE, typename SPEC>
    void malloc(DEVICE& device, rl::components::ReplayBuffer<SPEC>& rb) {
        malloc(device, rb.observations);
        malloc(device, rb.actions);
        malloc(device, rb.rewards);
        malloc(device, rb.next_observations);
        malloc(device, rb.terminated);
        malloc(device, rb.truncated);
    }
    template <typename DEVICE, typename SPEC>
    void free(DEVICE& device, rl::components::ReplayBuffer<SPEC>& rb) {
        free(device, rb.observations);
        free(device, rb.actions);
        free(device, rb.rewards);
        free(device, rb.next_observations);
        free(device, rb.terminated);
        free(device, rb.truncated);
    }
    template <typename DEVICE, typename SPEC, typename SPEC::TI BATCH_SIZE>
    void malloc(DEVICE& device, rl::components::replay_buffer::Batch<SPEC, BATCH_SIZE>& batch) {
        malloc(device, batch.observations);
        malloc(device, batch.actions);
        malloc(device, batch.rewards);
        malloc(device, batch.next_observations);
        malloc(device, batch.terminated);
        malloc(device, batch.truncated);
    }
    template <typename DEVICE, typename SPEC, typename SPEC::TI BATCH_SIZE>
    void free(DEVICE& device, rl::components::replay_buffer::Batch<SPEC, BATCH_SIZE>& batch) {
        free(device, batch.observations);
        free(device, batch.actions);
        free(device, batch.rewards);
        free(device, batch.next_observations);
        free(device, batch.terminated);
        free(device, batch.truncated);
    }
    template <typename DEVICE, typename SPEC>
    void add(DEVICE& device, rl::components::ReplayBuffer<SPEC>& buffer, const typename SPEC::T observation[SPEC::OBSERVATION_DIM], const typename SPEC::T action[SPEC::ACTION_DIM], const typename SPEC::T reward, const typename SPEC::T next_observation[SPEC::OBSERVATION_DIM], const bool terminated, const bool truncated) {
        // todo: change to memcpy?
        for(typename DEVICE::index_t i = 0; i < SPEC::OBSERVATION_DIM; i++) {
            buffer.observations.data[buffer.position * SPEC::OBSERVATION_DIM + i] = observation[i];
            buffer.next_observations.data[buffer.position * SPEC::OBSERVATION_DIM + i] = next_observation[i];
        }
        for(typename DEVICE::index_t i = 0; i < SPEC::ACTION_DIM; i++) {
            buffer.actions.data[buffer.position * SPEC::ACTION_DIM + i] = action[i];
        }
        buffer.rewards.data[buffer.position] = reward;
        buffer.terminated.data[buffer.position] = terminated;
        buffer.truncated.data[buffer.position] = truncated;
        buffer.position = (buffer.position + 1) % SPEC::CAPACITY;
        if(buffer.position == 0 && !buffer.full) {
            buffer.full = true;
        }
    }
    template <typename DEVICE, typename SPEC, typename SPEC::TI BATCH_SIZE, typename RNG, bool DETERMINISTIC=false>
    void gather_batch(DEVICE& device, rl::components::ReplayBuffer<SPEC>& replay_buffer, rl::components::replay_buffer::Batch<SPEC, BATCH_SIZE>& batch, RNG& rng) {
        using T = typename SPEC::T;
        for(typename DEVICE::index_t batch_step_i=0; batch_step_i < BATCH_SIZE; batch_step_i++) {
            typename DEVICE::index_t sample_index_max = (replay_buffer.full ? SPEC::CAPACITY : replay_buffer.position) - 1;
            typename DEVICE::index_t sample_index = DETERMINISTIC ? batch_step_i : random::uniform_int_distribution( typename DEVICE::SPEC::RANDOM(), (typename DEVICE::index_t) 0, sample_index_max, rng);
            utils::memcpy(&batch.observations.data[batch_step_i * SPEC::OBSERVATION_DIM], &replay_buffer.observations.data[sample_index * SPEC::OBSERVATION_DIM], SPEC::OBSERVATION_DIM);
            utils::memcpy(&batch.actions.data[batch_step_i * SPEC::ACTION_DIM], &replay_buffer.actions.data[sample_index * SPEC::ACTION_DIM], SPEC::ACTION_DIM);
            batch.rewards.data[batch_step_i] = replay_buffer.rewards.data[sample_index];
            utils::memcpy(&batch.next_observations.data[batch_step_i * SPEC::OBSERVATION_DIM], &replay_buffer.next_observations.data[sample_index * SPEC::OBSERVATION_DIM], SPEC::OBSERVATION_DIM);
            batch.terminated.data[batch_step_i] = replay_buffer.terminated.data[sample_index];
            batch.truncated.data[batch_step_i] = replay_buffer.truncated.data[sample_index];
        }
    }
}
#endif
