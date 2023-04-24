SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
BACKPROP_TOOLS_INCLUDE_DIR=${BACKPROP_TOOLS_INCLUDE_DIR:-$SCRIPT_DIR/../../../../../../include}
BACKPROP_TOOLS_BUILD_DIR=${BACKPROP_TOOLS_BUILD_DIR:-./static/build}

echo BACKPROP_TOOLS_INCLUDE_DIR: $BACKPROP_TOOLS_INCLUDE_DIR
echo BACKPROP_TOOLS_BUILD_DIR: $BACKPROP_TOOLS_BUILD_DIR
cd $SCRIPT_DIR
mkdir -p $BACKPROP_TOOLS_BUILD_DIR
EXPORTED_FUNCTIONS="['_proxy_create_training_state', '_proxy_training_step', '_proxy_get_step', '_proxy_get_evaluation_count', '_proxy_get_evaluation_return', '_proxy_destroy_training_state', '_proxy_get_state_dim', '_proxy_get_state_value']"
em++ -DBACKPROP_TOOLS_STEP_LIMIT=10000 -DBACKPROP_TOOLS_BENCHMARK --std=c++17 -O3 -s WASM=1 -I$BACKPROP_TOOLS_INCLUDE_DIR -s EXPORTED_FUNCTIONS="$EXPORTED_FUNCTIONS" -s MODULARIZE=1 -s EXPORT_ES6=1 -s USE_ES6_IMPORT_META=0 -s ENVIRONMENT='web' --pre-js prejs.js -o $BACKPROP_TOOLS_BUILD_DIR/wasm_interface_benchmark.js wasm_interface.cpp
em++ --std=c++17 -O3 -s WASM=1 -I$BACKPROP_TOOLS_INCLUDE_DIR -s EXPORTED_FUNCTIONS="$EXPORTED_FUNCTIONS" -s MODULARIZE=1 -s EXPORT_ES6=1 -s USE_ES6_IMPORT_META=0 -s ENVIRONMENT='web' --pre-js prejs.js -o $BACKPROP_TOOLS_BUILD_DIR/wasm_interface.js wasm_interface.cpp
