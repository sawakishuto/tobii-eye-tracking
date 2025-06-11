#include "ring.hpp"
#include <GLFW/glfw3.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>

constexpr char SHM_NAME[] = "/gaze_shm";
constexpr char UDS_PATH[] = "/tmp/gaze_wakeup.sock";
static RingBuffer* rb = nullptr;

int main() {
    // 1) attach shared memory
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0600);
    if (shm_fd == -1) { 
        fprintf(stderr, "Failed to open shared memory. Make sure producer is running first.\n");
        perror("shm_open"); 
        return 1; 
    }
    
    rb = static_cast<RingBuffer*>(mmap(nullptr, sizeof(RingBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
    if (rb == MAP_FAILED) { 
        perror("mmap"); 
        close(shm_fd);
        return 1; 
    }
    close(shm_fd);  // Can close fd after mmap

    // 2) connect to UDS
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) { perror("socket"); return 1; }
    
    sockaddr_un addr{}; 
    addr.sun_family = AF_UNIX; 
    std::strcpy(addr.sun_path, UDS_PATH);
    
    printf("Connecting to producer...\n");
    int retry_count = 0;
    while (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        if (++retry_count > 100) {  // Timeout after ~1 second
            fprintf(stderr, "Failed to connect to producer. Make sure producer is running.\n");
            close(sock);
            return 1;
        }
        usleep(10'000);
    }
    printf("Connected to producer.\n");

    // 3) OpenGL window
    if (!glfwInit()) { fprintf(stderr, "GLFW init failed\n"); return 1; }
    GLFWwindow* win = glfwCreateWindow(800, 600, "Gaze Viewer", nullptr, nullptr);
    if (!win) { fprintf(stderr, "Window create failed\n"); return 1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(0);    // vsync off for latency measurement

    printf("Gaze visualization started. Close window or press Ctrl+C to quit.\n");
    
    struct pollfd pfd { sock, POLLIN, 0 };
    while (!glfwWindowShouldClose(win)) {
        poll(&pfd, 1, -1);                        // wait for wake‑up
        char trash[8]; recv(sock, trash, sizeof(trash), 0); // clear queue

        uint32_t idx = rb->w.load(std::memory_order_acquire) % LEN;
        auto s = rb->data[idx];
        if (!s.valid) continue;

        glClear(GL_COLOR_BUFFER_BIT);
        float gx = s.x * 2.f - 1.f;
        float gy = (1.f - s.y) * 2.f - 1.f;      // origin adjust
        glPointSize(20);
        glBegin(GL_POINTS);
        glVertex2f(gx, gy);
        glEnd();

        glfwSwapBuffers(win);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
