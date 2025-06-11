#!/bin/bash

echo "Building Gaze IPC Demo..."

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# Clean previous build
rm -rf *

# Configure with CMake
echo "Configuring with CMake..."
cmake ..

if [ $? -eq 0 ]; then
    echo "Building project..."
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
    
    if [ $? -eq 0 ]; then
        echo "Build successful! Executables are in build/bin/"
        echo ""
        echo "To run the demo:"
        echo "1. Start the producer: ./bin/producer"
        echo "2. Start the consumer: ./bin/consumer"
        echo ""
        echo "Note: Make sure your Tobii eye tracker is connected and the SDK is properly installed."
    else
        echo "Build failed!"
        exit 1
    fi
else
    echo "CMake configuration failed!"
    exit 1
fi 