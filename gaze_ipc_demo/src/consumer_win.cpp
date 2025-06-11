#include "ring.hpp"

#ifdef _WIN32
#include <windows.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <atomic>

constexpr char SHM_NAME[] = "GazeSharedMemory";        // Windows shared memory name
constexpr char PIPE_NAME[] = "\\\\.\\pipe\\GazeWakeup"; // Windows named pipe

static RingBuffer* rb = nullptr;
static HANDLE hMapFile = nullptr;
static HANDLE hPipe = INVALID_HANDLE_VALUE;

int main() {
    std::cout << "Gaze IPC Consumer (Windows Version)\n";
    std::cout << "Connecting to producer...\n\n";
    
    // 1) Attach to existing shared memory
    hMapFile = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,   // read/write access
        FALSE,                 // do not inherit the name
        SHM_NAME);             // name of mapping object

    if (hMapFile == nullptr) {
        std::cerr << "Could not open file mapping object (" << GetLastError() << ").\n";
        std::cerr << "Make sure producer is running first.\n";
        return 1;
    }

    rb = (RingBuffer*)MapViewOfFile(
        hMapFile,              // handle to map object
        FILE_MAP_ALL_ACCESS,   // read/write permission
        0,
        0,
        sizeof(RingBuffer));

    if (rb == nullptr) {
        std::cerr << "Could not map view of file (" << GetLastError() << ").\n";
        CloseHandle(hMapFile);
        return 1;
    }

    // 2) Connect to named pipe
    std::cout << "Connecting to producer...\n";
    
    // Wait for named pipe to become available
    while (true) {
        hPipe = CreateFile(
            PIPE_NAME,            // pipe name
            GENERIC_READ,         // read access
            0,                    // no sharing
            nullptr,              // default security attributes
            OPEN_EXISTING,        // opens existing pipe
            0,                    // default attributes
            nullptr);             // no template file

        if (hPipe != INVALID_HANDLE_VALUE)
            break;

        if (GetLastError() != ERROR_PIPE_BUSY) {
            std::cerr << "Could not open pipe (" << GetLastError() << ").\n";
            std::cerr << "Make sure producer is running.\n";
            UnmapViewOfFile(rb);
            CloseHandle(hMapFile);
            return 1;
        }

        // All pipe instances are busy, so wait
        if (!WaitNamedPipe(PIPE_NAME, 1000)) {
            std::cerr << "Could not open pipe: 1 second wait timed out.\n";
            UnmapViewOfFile(rb);
            CloseHandle(hMapFile);
            return 1;
        }
    }

    std::cout << "Connected to producer.\n";

    // 3) Initialize OpenGL window
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        CloseHandle(hPipe);
        UnmapViewOfFile(rb);
        CloseHandle(hMapFile);
        return 1;
    }

    GLFWwindow* win = glfwCreateWindow(800, 600, "Gaze Viewer (Windows)", nullptr, nullptr);
    if (!win) {
        std::cerr << "Window create failed\n";
        glfwTerminate();
        CloseHandle(hPipe);
        UnmapViewOfFile(rb);
        CloseHandle(hMapFile);
        return 1;
    }

    glfwMakeContextCurrent(win);
    glfwSwapInterval(0);    // vsync off for latency measurement

    std::cout << "Gaze visualization started. Close window to quit.\n";

    // 4) Main rendering loop
    while (!glfwWindowShouldClose(win)) {
        // Wait for notification from producer
        DWORD bytesRead;
        char buffer[16];
        BOOL success = ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, nullptr);
        
        if (!success && GetLastError() != ERROR_MORE_DATA) {
            std::cerr << "ReadFile failed (" << GetLastError() << ").\n";
            break;
        }

        // Read latest gaze data
        uint32_t idx = rb->w.load(std::memory_order_acquire) % LEN;
        auto s = rb->data[idx];
        
        if (!s.valid) {
            glfwPollEvents();
            continue;
        }

        // Clear and render gaze point
        glClear(GL_COLOR_BUFFER_BIT);
        
        float gx = s.x * 2.0f - 1.0f;               // Convert to [-1, 1]
        float gy = (1.0f - s.y) * 2.0f - 1.0f;     // Flip Y and convert to [-1, 1]
        
        // Set point color (red for valid gaze)
        glColor3f(1.0f, 0.0f, 0.0f);
        glPointSize(20);
        glBegin(GL_POINTS);
        glVertex2f(gx, gy);
        glEnd();

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    // Cleanup
    glfwTerminate();
    CloseHandle(hPipe);
    UnmapViewOfFile(rb);
    CloseHandle(hMapFile);

    return 0;
}

#else
#error "This file is for Windows only. Use consumer.cpp for Unix/Linux."
#endif 