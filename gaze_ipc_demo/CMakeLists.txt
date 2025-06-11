cmake_minimum_required(VERSION 3.20)
project(gaze_ipc_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform-specific configurations
if(APPLE)
    # Silence OpenGL deprecation warnings on macOS
    add_definitions(-DGL_SILENCE_DEPRECATION)
    set(TOBII_LIBRARY_NAME "libtobii_research.dylib")
    set(DEFAULT_SDK_PATH "/opt/tobii/sdk")
elseif(WIN32)
    set(TOBII_LIBRARY_NAME "tobii_research.dll")
    set(DEFAULT_SDK_PATH "C:/Program Files/Tobii/Tobii Pro SDK")
else()
    # Linux
    set(TOBII_LIBRARY_NAME "libtobii_research.so")
    set(DEFAULT_SDK_PATH "/opt/tobii/sdk")
endif()

# Find required packages
find_package(glfw3 REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

# Tobii Research SDK configuration (optional)
set(TOBII_SDK_PATH "${DEFAULT_SDK_PATH}" CACHE PATH "Path to Tobii Research SDK")
set(USE_TOBII_SDK OFF)

if(EXISTS ${TOBII_SDK_PATH}/include/tobii_research.h AND EXISTS ${TOBII_SDK_PATH}/lib/${TOBII_LIBRARY_NAME})
    set(USE_TOBII_SDK ON)
    message(STATUS "Tobii SDK found - using real eye tracker")
else()
    message(STATUS "Tobii SDK not found - using simulation mode")
endif()

# Platform-specific executables
if(WIN32)
    # Windows versions using Win32 API
    add_executable(producer src/producer_win.cpp)
    target_include_directories(producer PRIVATE include)
    
    if(USE_TOBII_SDK)
        target_include_directories(producer PRIVATE ${TOBII_SDK_PATH}/include)
        target_link_libraries(producer PRIVATE 
            Threads::Threads
            ${TOBII_SDK_PATH}/lib/${TOBII_LIBRARY_NAME}
        )
        target_compile_definitions(producer PRIVATE USE_TOBII_SDK)
    else()
        target_link_libraries(producer PRIVATE Threads::Threads)
        target_compile_definitions(producer PRIVATE USE_SIMULATION)
    endif()
    
    add_executable(consumer src/consumer_win.cpp)
    target_include_directories(consumer PRIVATE include)
    target_link_libraries(consumer PRIVATE 
        glfw
        OpenGL::GL
        Threads::Threads
    )
    
else()
    # Unix/Linux versions using POSIX API
    add_executable(producer src/producer.cpp)
    target_include_directories(producer PRIVATE include)
    
    if(USE_TOBII_SDK)
        target_include_directories(producer PRIVATE ${TOBII_SDK_PATH}/include)
        target_link_libraries(producer PRIVATE 
            Threads::Threads
            ${TOBII_SDK_PATH}/lib/${TOBII_LIBRARY_NAME}
        )
        target_compile_definitions(producer PRIVATE USE_TOBII_SDK)
    else()
        target_link_libraries(producer PRIVATE Threads::Threads)
        target_compile_definitions(producer PRIVATE USE_SIMULATION)
    endif()
    
    add_executable(consumer src/consumer.cpp)
    target_include_directories(consumer PRIVATE include)
    target_link_libraries(consumer PRIVATE 
        glfw
        OpenGL::GL
        Threads::Threads
    )
endif()

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
