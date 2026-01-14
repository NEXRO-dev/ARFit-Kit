# CMake for Web Assembly Build (Emscripten)
cmake_minimum_required(VERSION 3.10)
project(ARFitKitWeb)

# Emscripten toolchain file should be passed via command line
# -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake

set(CMAKE_CXX_STANDARD 17)

# Include core headers
include_directories(../../core/include)

# Source files
file(GLOB_RECURSE CORE_SOURCES "../../core/src/*.cpp")

# Define wasm target
add_executable(arfitkit_wasm ${CORE_SOURCES} "src/arfit_wasm.cpp")

# Compile options for Emscripten
target_compile_options(arfitkit_wasm PRIVATE -O3 -flto)

# Link options
target_link_options(arfitkit_wasm PRIVATE
    -O3
    -flto
    -s WASM=1
    -s MODULARIZE=1
    -s EXPORT_NAME="ARFitKitModule"
    -s "EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap']"
    -s ALLOW_MEMORY_GROWTH=1
    --bind 
)

# Output files
set_target_properties(arfitkit_wasm PROPERTIES 
    SUFFIX ".js"
    OUTPUT_NAME "arfit-kit-wasm"
)
