#include <layer_in_c/nn/nn.h>
namespace lic = layer_in_c;

using DTYPE = double;


template <typename DEVICE, typename T_T>
struct StructureSpecification{
    typedef T_T T;
    static constexpr typename DEVICE::index_t INPUT_DIM = 17;
    static constexpr typename DEVICE::index_t OUTPUT_DIM = 13;
    static constexpr int NUM_LAYERS = 3;
    static constexpr int HIDDEN_DIM = 50;
    static constexpr lic::nn::activation_functions::ActivationFunction HIDDEN_ACTIVATION_FUNCTION = lic::nn::activation_functions::RELU;
    static constexpr lic::nn::activation_functions::ActivationFunction OUTPUT_ACTIVATION_FUNCTION = lic::nn::activation_functions::IDENTITY;
};

using NN_DEVICE = lic::devices::DefaultCPU;

using STRUCTURE_SPEC = StructureSpecification<NN_DEVICE, DTYPE>;
using NETWORK_SPEC = lic::nn_models::mlp::AdamSpecification<NN_DEVICE, STRUCTURE_SPEC, lic::nn::optimizers::adam::DefaultParametersTF<DTYPE>>;
using NetworkType = lic::nn_models::mlp::NeuralNetworkAdam<NN_DEVICE, NETWORK_SPEC>;

using NETWORK_SPEC_BACKWARD_ONLY = lic::nn_models::mlp::InferenceBackwardSpecification<NN_DEVICE, StructureSpecification<NN_DEVICE, DTYPE>>;
using NetworkTypeBackwardOnly = lic::nn_models::mlp::NeuralNetworkBackward<NN_DEVICE, NETWORK_SPEC_BACKWARD_ONLY>;

constexpr typename NN_DEVICE::index_t INPUT_DIM = STRUCTURE_SPEC::INPUT_DIM;
constexpr typename NN_DEVICE::index_t LAYER_1_DIM = STRUCTURE_SPEC::HIDDEN_DIM;
constexpr typename NN_DEVICE::index_t LAYER_2_DIM = STRUCTURE_SPEC::HIDDEN_DIM;
constexpr typename NN_DEVICE::index_t OUTPUT_DIM = STRUCTURE_SPEC::OUTPUT_DIM;

class NeuralNetworkTest : public ::testing::Test {
protected:
    std::string DATA_FILE_PATH = "../model-learning/data.hdf5";
    std::string model_name = "model_1";
    void SetUp() override {
        const char* data_file_path = std::getenv("LAYER_IN_C_TEST_NN_DATA_FILE");
        if (data_file_path != NULL){
            DATA_FILE_PATH = std::string(data_file_path);
//            std::runtime_error("Environment variable LAYER_IN_C_TEST_DATA_DIR not set. Skipping test.");
        }

        auto data_file = HighFive::File(DATA_FILE_PATH, HighFive::File::ReadOnly);
        data_file.getDataSet("data/X_train").read(X_train);
        data_file.getDataSet("data/Y_train").read(Y_train);
        data_file.getDataSet("data/X_val").read(X_val);
        data_file.getDataSet("data/Y_val").read(Y_val);
        data_file.getDataSet("data/X_mean").read(X_mean);
        data_file.getDataSet("data/X_std").read(X_std);
        data_file.getDataSet("data/Y_mean").read(Y_mean);
        data_file.getDataSet("data/Y_std").read(Y_std);
    }

    std::vector<std::vector<DTYPE>> X_train;
    std::vector<std::vector<DTYPE>> Y_train;
    std::vector<std::vector<DTYPE>> X_val;
    std::vector<std::vector<DTYPE>> Y_val;
    std::vector<DTYPE> X_mean;
    std::vector<DTYPE> X_std;
    std::vector<DTYPE> Y_mean;
    std::vector<DTYPE> Y_std;
};

template <typename DEVICE, typename T_T>
struct StructureSpecification_3{
    typedef T_T T;
    static constexpr typename DEVICE::index_t INPUT_DIM = 17;
    static constexpr typename DEVICE::index_t OUTPUT_DIM = 13;
    static constexpr int NUM_LAYERS = 3;
    static constexpr int HIDDEN_DIM = 50;
    static constexpr lic::nn::activation_functions::ActivationFunction HIDDEN_ACTIVATION_FUNCTION = lic::nn::activation_functions::GELU;
    static constexpr lic::nn::activation_functions::ActivationFunction OUTPUT_ACTIVATION_FUNCTION = lic::nn::activation_functions::IDENTITY;
};
using NETWORK_SPEC_3 = lic::nn_models::mlp::AdamSpecification<NN_DEVICE, StructureSpecification_3<NN_DEVICE, DTYPE>, lic::nn::optimizers::adam::DefaultParametersTF<DTYPE>>;
using NetworkType_3 = lic::nn_models::mlp::NeuralNetworkAdam<NN_DEVICE, NETWORK_SPEC_3>;
