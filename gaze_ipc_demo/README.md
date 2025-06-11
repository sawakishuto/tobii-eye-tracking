# Gaze IPC Demo

This project demonstrates real-time gaze data communication between a Tobii eye tracker and a visualization application using shared memory and Unix domain sockets for high-performance IPC.

## Architecture

- **Producer** (`producer.cpp`): Connects to Tobii eye tracker, receives gaze data, and stores it in a shared memory ring buffer
- **Consumer** (`consumer.cpp`): Reads gaze data from shared memory and visualizes it in real-time using OpenGL
- **IPC Mechanism**:
  - Shared memory for high-throughput data transfer
  - Unix domain sockets for wake-up notifications

## Prerequisites

### 1. Tobii Research SDK

Download and install the Tobii Research SDK from:
https://developer.tobii.com/product-integration/stream-engine/

### 2. System Dependencies (macOS)

```bash
# Install dependencies
./setup_tobii.sh
```

Or manually:

```bash
# Install GLFW
brew install glfw

# Create SDK directory
sudo mkdir -p /opt/tobii/sdk/{lib,include}
sudo chown -R $(whoami) /opt/tobii

# Extract Tobii SDK files to:
# - Headers: /opt/tobii/sdk/include/
# - Library: /opt/tobii/sdk/lib/libtobii_research.dylib
```

## Building

```bash
# Build the project
./build.sh
```

Or manually:

```bash
mkdir -p build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

## Usage

### 1. Connect Eye Tracker

Make sure your Tobii eye tracker is connected and properly calibrated.

### 2. Run the Demo

```bash
# Terminal 1: Start the producer
cd build
./bin/producer

# Terminal 2: Start the consumer
cd build
./bin/consumer
```

## Configuration

### Custom SDK Path

If your Tobii SDK is installed in a different location:

```bash
cmake -DTOBII_SDK_PATH=/path/to/your/tobii/sdk ..
```

### Performance Tuning

- The ring buffer size can be adjusted in `include/ring.hpp` (LEN constant)
- Disable VSync in consumer for minimum latency
- Use high-frequency eye tracker modes if available

## Technical Details

### Ring Buffer

- Lock-free circular buffer for producer-consumer communication
- Atomic write index for thread safety
- Configurable size (default: 1024 samples)

### Data Flow

1. Tobii SDK callback receives gaze data
2. Producer stores data in shared memory ring buffer
3. Producer sends wake-up signal via Unix domain socket
4. Consumer reads data and renders gaze point

### Coordinate System

- Gaze coordinates are normalized (0.0-1.0)
- Origin adjustment for OpenGL rendering
- Timestamp in milliseconds for latency analysis

## Troubleshooting

### CMake Configuration Issues

- Verify Tobii SDK path: `/opt/tobii/sdk`
- Check that `libtobii_research.dylib` exists
- Ensure GLFW is installed via Homebrew

### Runtime Issues

- Eye tracker not found: Check USB connection and drivers
- Shared memory errors: Restart both applications
- OpenGL errors: Update graphics drivers

### Permission Issues

```bash
# Fix shared memory permissions
sudo rm /dev/shm/gaze_shm
sudo rm /tmp/gaze_wakeup.sock
```

## Performance Metrics

The system is designed for:

- Sub-millisecond IPC latency
- 120Hz+ gaze data throughput
- Minimal CPU overhead
- Real-time visualization

## License

This project is for demonstration purposes. Please check Tobii Research SDK license terms for commercial use.
