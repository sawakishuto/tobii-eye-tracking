#pragma once
#include <atomic>
#include <cstdint>

constexpr size_t LEN = 1024;  // Ring buffer size (power of 2 for efficiency)

struct Sample {
    double ts;      // timestamp in milliseconds
    float x, y;     // normalized gaze coordinates (0.0-1.0)
    int valid;      // validity flag
};

struct RingBuffer {
    std::atomic<uint32_t> w{0};  // write index (producer increments)
    Sample data[LEN];            // circular buffer
};