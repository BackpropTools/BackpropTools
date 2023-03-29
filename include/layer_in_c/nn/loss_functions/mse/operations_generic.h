#ifndef LAYER_IN_C_NN_LOSS_FUNCTIONS_MSE_OPERATIONS_GENERIC
#define LAYER_IN_C_NN_LOSS_FUNCTIONS_MSE_OPERATIONS_GENERIC

namespace layer_in_c::nn::loss_functions {
    template<typename DEVICE, typename SPEC_A, typename SPEC_B>
    typename SPEC_A::T mse(DEVICE& device, Matrix<SPEC_A> a, Matrix<SPEC_B> b, typename SPEC_A::T loss_weight = 1) {
        static_assert(containers::check_structure<SPEC_A, SPEC_B>);
        using T = typename SPEC_A::T;
        using TI = typename SPEC_A::TI;
        T acc = 0;
        for(TI row_i = 0; row_i < SPEC_A::ROWS; row_i++) {
            for(TI col_i = 0; col_i < SPEC_A::COLS; col_i++) {
//                TI index = row_i * SPEC_A::COLS + col_i;
                T diff = get(a, row_i, col_i) - get(b, row_i, col_i);
                acc += diff * diff;
            }
        }
        return acc * loss_weight / (SPEC_A::ROWS * SPEC_A::COLS);
    }

    template<typename DEVICE, typename SPEC_A, typename SPEC_B, typename SPEC_DA>
    void d_mse_d_x(DEVICE& device, Matrix<SPEC_A> a, Matrix<SPEC_B> b, Matrix<SPEC_DA> d_a, typename SPEC_A::T loss_weight = 1) {
        static_assert(containers::check_structure<SPEC_A, SPEC_B>);
        static_assert(containers::check_structure<SPEC_A, SPEC_DA>);
        using T = typename SPEC_A::T;
        using TI = typename SPEC_A::TI;
        for(TI row_i = 0; row_i < SPEC_A::ROWS; row_i++) {
            for(TI col_i = 0; col_i < SPEC_A::COLS; col_i++) {
//                TI index = row_i * SPEC_A::COLS + col_i;
                T diff = get(a, row_i, col_i) - get(b, row_i, col_i);
                set(d_a, row_i, col_i, 2*diff/(SPEC_A::ROWS * SPEC_A::COLS) * loss_weight);
            }
        }
    }
}

#endif