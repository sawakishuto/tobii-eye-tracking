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
    if (shm_fd == -1) { perror("shm_open"); return 1; }
    rb = static_cast<RingBuffer*>(mmap(nullptr, sizeof(RingBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
    if (rb == MAP_FAILED) { perror("mmap"); return 1; }

    // 2) connect to UDS
    int sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    sockaddr_un addr{}; addr.sun_family = AF_UNIX; std::strcpy(addr.sun_path, UDS_PATH);
    while (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) usleep(10'000);

    // 3) OpenGL window
    if (!glfwInit()) { fprintf(stderr, "GLFW init failed\n"); return 1; }
    GLFWwindow* win = glfwCreateWindow(800, 600, "Gaze Viewer", nullptr, nullptr);
    if (!win) { fprintf(stderr, "Window create failed\n"); return 1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(0);    // vsync off for latency measurement

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
