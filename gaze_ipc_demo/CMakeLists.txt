cmake_minimum_required(VERSION 3.20)
project(gaze_ipc_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Silence OpenGL deprecation warnings on macOS
add_definitions(-DGL_SILENCE_DEPRECATION)

# Find required packages
find_package(glfw3 REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

# Tobii Research SDK configuration (optional)
set(TOBII_SDK_PATH "/opt/tobii/sdk" CACHE PATH "Path to Tobii Research SDK")
set(USE_TOBII_SDK OFF)

if(EXISTS ${TOBII_SDK_PATH}/include/tobii_research.h AND EXISTS ${TOBII_SDK_PATH}/lib/libtobii_research.dylib)
    set(USE_TOBII_SDK ON)
    message(STATUS "Tobii SDK found - using real eye tracker")
else()
    message(STATUS "Tobii SDK not found - using simulation mode")
endif()

# Producer executable
add_executable(producer src/producer.cpp)
target_include_directories(producer PRIVATE include)

if(USE_TOBII_SDK)
    target_include_directories(producer PRIVATE ${TOBII_SDK_PATH}/include)
    target_link_libraries(producer PRIVATE 
        Threads::Threads
        ${TOBII_SDK_PATH}/lib/libtobii_research.dylib
    )
    target_compile_definitions(producer PRIVATE USE_TOBII_SDK)
else()
    target_link_libraries(producer PRIVATE Threads::Threads)
    target_compile_definitions(producer PRIVATE USE_SIMULATION)
endif()

# Consumer executable  
add_executable(consumer src/consumer.cpp)
target_include_directories(consumer PRIVATE include)
target_link_libraries(consumer PRIVATE 
    glfw
    OpenGL::GL
    Threads::Threads
)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
