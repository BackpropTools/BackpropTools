#ifndef LAYER_IN_C_NN_UTILS_POLYAK
#define LAYER_IN_C_NN_UTILS_POLYAK


namespace layer_in_c::utils::polyak {
    // todo: polyak factor as template parameter (reciprocal INT e.g.)
    template<typename DEVICE, typename T, auto N_ROWS, auto N_COLS>
    void update_matrix(DEVICE dev, T target[N_ROWS][N_COLS], const T source[N_ROWS][N_COLS], const T polyak) {
        for(typename DEVICE::index_t i = 0; i < N_ROWS; i++) {
            for(typename DEVICE::index_t j = 0; j < N_COLS; j++) {
                target[i][j] = polyak * target[i][j] + (1 - polyak) * source[i][j];
            }
        }
    }
    template<typename DEVICE, typename T, auto DIM>
    void update(DEVICE dev, T target[DIM], const T source[DIM], const T polyak) {
        for(typename DEVICE::index_t i = 0; i < DIM; i++) {
            target[i] = polyak * target[i] + (1 - polyak) * source[i];
        }
    }

    template<typename DEVICE, typename T, auto N_ROWS, auto N_COLS>
    void update_squared_matrix(DEVICE dev, T target[N_ROWS][N_COLS], const T source[N_ROWS][N_COLS], const T polyak) {
        for(typename DEVICE::index_t i = 0; i < N_ROWS; i++) {
            for(typename DEVICE::index_t j = 0; j < N_COLS; j++) {
                target[i][j] = polyak * target[i][j] + (1 - polyak) * source[i][j] * source[i][j];
            }
        }
    }
    template<typename DEVICE, typename T, auto DIM>
    void update_squared(DEVICE dev, T target[DIM], const T source[DIM], const T polyak) {
        for(typename DEVICE::index_t i = 0; i < DIM; i++) {
            target[i] = polyak * target[i] + (1 - polyak) * source[i] * source[i];
        }
    }
}


#endif