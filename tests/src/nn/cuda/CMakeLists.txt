cmake_minimum_required(VERSION 3.16)

project(test_backprop_tools_nn_cuda CUDA)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CXX_STANDARD  17)

function(BACKPROP_TOOLS_TESTS_NN_CUDA_ADD_DEBUG_DEFINITIONS TARGET)
    #    if(BACKPROP_TOOLS_DEBUG)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#        target_compile_definitions(${TARGET} PRIVATE -DBACKPROP_TOOLS_DEBUG_CONTAINER_CHECK_BOUNDS)
#        target_compile_definitions(${TARGET} PRIVATE -DBACKPROP_TOOLS_DEBUG_CONTAINER_CHECK_MALLOC)
        target_compile_definitions(${TARGET} PRIVATE -DBACKPROP_TOOLS_DEBUG_CONTAINER_MALLOC_INIT_NAN)
        target_compile_definitions(${TARGET} PRIVATE -DBACKPROP_TOOLS_DEBUG_RL_COMPONENTS_OFF_POLICY_RUNNER_CHECK_INIT)
    endif()
endfunction()

add_executable(
        test_nn_cuda_basics
        cuda_basics.cu
)
target_link_libraries(
        test_nn_cuda_basics
        PRIVATE
        backprop_tools
        GTest::gtest_main
        cublas
)
set_target_properties(test_nn_cuda_basics PROPERTIES CUDA_ARCHITECTURES ${BACKPROP_TOOLS_BACKEND_CUDA_ARCHITECTURES})
BACKPROP_TOOLS_TESTS_NN_CUDA_ADD_DEBUG_DEFINITIONS(test_nn_cuda_basics)
gtest_discover_tests(test_nn_cuda_basics)

#add_executable(
#        test_nn_cuda_cutlass
#        cutlass_test.cu
#)
#target_link_libraries(
#        test_nn_cuda_cutlass
#        PRIVATE
#        CUTLASS
#)
