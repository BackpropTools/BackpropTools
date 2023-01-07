#include "multirotor.h"

namespace layer_in_c::rl::environments::multirotor {
    template<typename T, int ACTION_DIM>
    typename Parameters <T, ACTION_DIM>::Dynamics default_dynamics_parameters = {
            // Rotor positions
//            {
//                    {1, -1, 0},
//                    {1, 1, 0},
//                    {-1, 1, 0},
//                    {-1, -1, 0}
//            }, // => normalized
//        {
//                { 0.12020815, -0.12020815,  0.        },
//                { 0.12020815,  0.12020815,  0.        },
//                {-0.12020815,  0.12020815,  0.        },
//                {-0.12020815, -0.12020815,  0.        }
//        },
            {
                    {
                            0.1202081528017130834795622718047525268048,
                            -0.1202081528017130834795622718047525268048,
                            0.0000000000000000000000000000000000000000
                    },
                    {
                            0.1202081528017130834795622718047525268048,
                            0.1202081528017130834795622718047525268048,
                            0.0000000000000000000000000000000000000000
                    },
                    {

                            -0.1202081528017130834795622718047525268048,
                            0.1202081528017130834795622718047525268048,
                            0.0000000000000000000000000000000000000000
                    },
                    {
                            -0.1202081528017130834795622718047525268048,
                            -0.1202081528017130834795622718047525268048,
                            0.0000000000000000000000000000000000000000
                    }
            },
            // Rotor thrust directions
            {
                    {0, 0, -1},
                    {0, 0, -1},
                    {0, 0, -1},
                    {0, 0, -1},
            },
            // Rotor torque directions
            {
                    {0, 0, -1},
                    {0, 0, 1},
                    {0, 0, -1},
                    {0, 0, 1},
            },
            // thrust constants
//        {1.3298253500372892e-06, 0.0038360810526746033, -1.768998684812532},
            {
                    0.0000013298253500372891536393180067499031,
                    0.0038360810526746032603218061751704226481,
                    -1.7689986848125325291647413905593566596508
            },
            // torque constant
            0.016,
            // mass vehicle
            0.73,
            // gravity
            {0, 0, 9.81},
            // J
//        {
//                {0.00791138, 0.0       , 0.0       },
//                {0.0       , 0.00791138, 0.0       },
//                {0.0       , 0.0       , 0.01230658}
//        },
            {
                    {
                            0.0079113750000000000045519144009631418157,
                            0.0000000000000000000000000000000000000000,
                            0.0000000000000000000000000000000000000000
                    },
                    {
                            0.0000000000000000000000000000000000000000,
                            0.0079113750000000000045519144009631418157,
                            0.0000000000000000000000000000000000000000
                    },
                    {
                            0.0000000000000000000000000000000000000000,
                            0.0000000000000000000000000000000000000000,
                            0.0123065833333333343041493534997243841644
                    }
            },
            // J_inv
//        {
//                {126.40027808,   0.        ,   0.        },
//                {  0.        , 126.40027808,   0.        },
//                {  0.        ,   0.        ,  81.25732162}
//        },
            {
                    {
                            126.4002780806117840484148473478853702545166,
                            0.0000000000000000000000000000000000000000,
                            0.0000000000000000000000000000000000000000
                    },
                    {
                            0.0000000000000000000000000000000000000000,
                            126.4002780806117840484148473478853702545166,
                            0.0000000000000000000000000000000000000000
                    },
                    {
                            0.0000000000000000000000000000000000000000,
                            0.0000000000000000000000000000000000000000,
                            81.2573216232504194067587377503514289855957
                    }
            }
    };
    template<typename T, int ACTION_DIM>
    typename Parameters<T, ACTION_DIM>::Reward default_reward_parameters = {
            10,
            10,
            1,
            0,
            1
    };
    template<typename T, int ACTION_DIM>
    typename Parameters<T, ACTION_DIM>::ActionLimit default_action_limit = {0, 2000};
    template<typename T, int ACTION_DIM>
    typename Parameters<T, ACTION_DIM>::Initialization default_init_parameters = {
            2,
            1,
            0.5 * M_PI * 2
    };
    template<typename T, int ACTION_DIM>
    typename Parameters<T, ACTION_DIM>::Initialization simple_init_parameters = {
            0,
            0,
            0
    };
    template<typename T>
    Parameters<T, 4> default_parameters = {
            default_dynamics_parameters<T, 4>,
            default_action_limit<T, 4>,
            simple_init_parameters<T, 4>,
            default_reward_parameters<T, 4>,
            // dt
            0.02
    };



}