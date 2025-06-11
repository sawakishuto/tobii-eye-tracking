#include "ring.hpp"

#ifdef _WIN32
#include <windows.h>
#include <iostream>
#include <atomic>
#include <cstring>

extern "C" {
#include <tobii_research.h>
#include <tobii_research_streams.h>
}

constexpr char SHM_NAME[] = "GazeSharedMemory";        // Windows shared memory name
constexpr char PIPE_NAME[] = "\\\\.\\pipe\\GazeWakeup"; // Windows named pipe

static RingBuffer* rb = nullptr;
static HANDLE hMapFile = nullptr;
static HANDLE hPipe = INVALID_HANDLE_VALUE;

void gaze_callback(TobiiResearchGazeData* gd, void*) {
    if (!rb) return;
    
    uint32_t idx = rb->w.fetch_add(1, std::memory_order_relaxed) % LEN;
    auto& s = rb->data[idx];
    s.ts = gd->system_time_stamp / 1000.0;   // µs → ms
    
    // Check if left eye data is valid, fallback to right eye if needed
    if (gd->left_eye.gaze_point.validity == TOBII_RESEARCH_VALIDITY_VALID) {
        s.x = gd->left_eye.gaze_point.position_on_display_area.x;
        s.y = gd->left_eye.gaze_point.position_on_display_area.y;
        s.valid = 1;
    } else if (gd->right_eye.gaze_point.validity == TOBII_RESEARCH_VALIDITY_VALID) {
        s.x = gd->right_eye.gaze_point.position_on_display_area.x;
        s.y = gd->right_eye.gaze_point.position_on_display_area.y;
        s.valid = 1;
    } else {
        s.valid = 0;  // Invalid data
    }
    
    // Wake up consumer via named pipe
    if (hPipe != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        char signal = '1';
        WriteFile(hPipe, &signal, 1, &bytesWritten, nullptr);
    }
}

int main() {
    std::cout << "Gaze IPC Producer (Windows Version)\n";
    std::cout << "Connecting to Tobii eye tracker...\n\n";
    
    // 1) Create shared memory using Windows API
    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,    // use paging file
        nullptr,                 // default security
        PAGE_READWRITE,          // read/write access
        0,                       // maximum object size (high-order DWORD)
        sizeof(RingBuffer),      // maximum object size (low-order DWORD)
        SHM_NAME);               // name of mapping object

    if (hMapFile == nullptr) {
        std::cerr << "Could not create file mapping object (" << GetLastError() << ").\n";
        return 1;
    }

    rb = (RingBuffer*)MapViewOfFile(
        hMapFile,               // handle to map object
        FILE_MAP_ALL_ACCESS,    // read/write permission
        0,
        0,
        sizeof(RingBuffer));

    if (rb == nullptr) {
        std::cerr << "Could not map view of file (" << GetLastError() << ").\n";
        CloseHandle(hMapFile);
        return 1;
    }

    // Initialize the ring buffer
    memset(rb, 0, sizeof(RingBuffer));

    // 2) Create named pipe for wake-up notifications
    hPipe = CreateNamedPipe(
        PIPE_NAME,               // pipe name
        PIPE_ACCESS_OUTBOUND,    // write access
        PIPE_TYPE_MESSAGE |      // message type pipe
        PIPE_READMODE_MESSAGE |  // message-read mode
        PIPE_WAIT,               // blocking mode
        1,                       // max. instances
        1024,                    // output buffer size
        1024,                    // input buffer size
        0,                       // client time-out
        nullptr);                // default security attribute

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateNamedPipe failed (" << GetLastError() << ").\n";
        UnmapViewOfFile(rb);
        CloseHandle(hMapFile);
        return 1;
    }

    std::cout << "Waiting for consumer to connect...\n";
    
    // Wait for the consumer to connect
    BOOL fConnected = ConnectNamedPipe(hPipe, nullptr) ? 
        TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

    if (!fConnected) {
        std::cerr << "ConnectNamedPipe failed (" << GetLastError() << ").\n";
        CloseHandle(hPipe);
        UnmapViewOfFile(rb);
        CloseHandle(hMapFile);
        return 1;
    }

    std::cout << "Consumer connected.\n";

    // 3) Tobii eye-tracker subscribe
    TobiiResearchEyeTrackers* eyetrackers = nullptr;
    TobiiResearchStatus status = tobii_research_find_all_eyetrackers(&eyetrackers);
    if (status != TOBII_RESEARCH_STATUS_OK) {
        std::cerr << "Failed to find eye trackers. Status: " << status << "\n";
        CloseHandle(hPipe);
        UnmapViewOfFile(rb);
        CloseHandle(hMapFile);
        return 1;
    }
    
    if (eyetrackers->count == 0) {
        std::cerr << "No eye tracker found.\n";
        tobii_research_free_eyetrackers(eyetrackers);
        CloseHandle(hPipe);
        UnmapViewOfFile(rb);
        CloseHandle(hMapFile);
        return 1;
    }
    
    TobiiResearchEyeTracker* eyetracker = eyetrackers->eyetrackers[0];
    
    std::cout << "Starting gaze data subscription...\n";
    status = tobii_research_subscribe_to_gaze_data(eyetracker, gaze_callback, nullptr);
    if (status != TOBII_RESEARCH_STATUS_OK) {
        std::cerr << "Subscribe failed. Status: " << status << "\n";
        tobii_research_free_eyetrackers(eyetrackers);
        CloseHandle(hPipe);
        UnmapViewOfFile(rb);
        CloseHandle(hMapFile);
        return 1;
    }

    std::cout << "Producer running with real eye tracker — press Enter to quit.\n";
    std::cin.get();  // Wait for user input (Windows equivalent of pause())
    
    // Cleanup
    tobii_research_unsubscribe_from_gaze_data(eyetracker, gaze_callback);
    tobii_research_free_eyetrackers(eyetrackers);
    
    CloseHandle(hPipe);
    UnmapViewOfFile(rb);
    CloseHandle(hMapFile);
    
    return 0;
}

#else
#error "This file is for Windows only. Use producer.cpp for Unix/Linux."
#endif 