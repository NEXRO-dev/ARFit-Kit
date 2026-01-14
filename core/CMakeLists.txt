cmake_minimum_required(VERSION 3.20)
project(ARFitKit VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(ARFIT_BUILD_TESTS "Build unit tests" ON)
option(ARFIT_BUILD_EXAMPLES "Build examples" ON)
option(ARFIT_USE_GPU "Enable GPU acceleration" ON)
option(ARFIT_BUILD_IOS "Build iOS framework" OFF)
option(ARFIT_BUILD_ANDROID "Build Android library" OFF)

# Find dependencies
find_package(OpenCV REQUIRED)

# MediaPipe (custom find module or submodule)
# find_package(MediaPipe REQUIRED)

# Core library sources
set(ARFIT_CORE_SOURCES
    src/arfit_kit.cpp
    src/body_tracker.cpp
    src/garment_converter.cpp
    src/physics_engine.cpp
    src/ar_renderer.cpp
    src/mesh.cpp
    src/texture.cpp
)

set(ARFIT_CORE_HEADERS
    include/arfit_kit.h
    include/body_tracker.h
    include/garment_converter.h
    include/physics_engine.h
    include/ar_renderer.h
    include/types.h
    include/mesh.h
    include/texture.h
)

# Create core library
add_library(arfit_core STATIC ${ARFIT_CORE_SOURCES} ${ARFIT_CORE_HEADERS})
target_include_directories(arfit_core PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(arfit_core PUBLIC ${OpenCV_LIBS})

# GPU acceleration
if(ARFIT_USE_GPU)
    if(APPLE)
        find_library(METAL_FRAMEWORK Metal)
        find_library(METALKIT_FRAMEWORK MetalKit)
        target_link_libraries(arfit_core PUBLIC ${METAL_FRAMEWORK} ${METALKIT_FRAMEWORK})
        target_compile_definitions(arfit_core PUBLIC ARFIT_USE_METAL)
    elseif(ANDROID)
        find_package(Vulkan REQUIRED)
        target_link_libraries(arfit_core PUBLIC Vulkan::Vulkan)
        target_compile_definitions(arfit_core PUBLIC ARFIT_USE_VULKAN)
    else()
        find_package(Vulkan)
        if(Vulkan_FOUND)
            target_link_libraries(arfit_core PUBLIC Vulkan::Vulkan)
            target_compile_definitions(arfit_core PUBLIC ARFIT_USE_VULKAN)
        endif()
    endif()
endif()

# Tests
if(ARFIT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(ARFIT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation
include(GNUInstallDirs)
install(TARGETS arfit_core
    EXPORT ARFitKitTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
