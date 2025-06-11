#include "ring.hpp"
extern "C" {
#include <tobii_research.h>
}
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <atomic>

constexpr char SHM_NAME[] = "/gaze_shm";               // POSIX SHM path
constexpr char UDS_PATH[] = "/tmp/gaze_wakeup.sock";   // UDS file path

static RingBuffer* rb = nullptr;
static int uds_conn   = -1;

void gaze_callback(TobiiResearchGazeData* gd, void*) {
    uint32_t idx = rb->w.fetch_add(1, std::memory_order_relaxed) % LEN;
    auto& s = rb->data[idx];
    s.ts    = gd->system_time_stamp / 1e6;   // µs → ms
    s.x     = gd->left_eye.gaze_point.position_in_display_normalized.x;
    s.y     = gd->left_eye.gaze_point.position_in_display_normalized.y;
    s.valid = 1;
    send(uds_conn, "1", 1, MSG_DONTWAIT);   // wake consumer (non‑block)
}

int main() {
    // 1) shared memory create & map
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600);
    if (shm_fd == -1) { perror("shm_open"); return 1; }
    if (ftruncate(shm_fd, sizeof(RingBuffer)) == -1) { perror("ftruncate"); return 1; }
    rb = static_cast<RingBuffer*>(mmap(nullptr, sizeof(RingBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
    if (rb == MAP_FAILED) { perror("mmap"); return 1; }
    memset(rb, 0, sizeof(RingBuffer));

    // 2) UDS server set‑up
    ::unlink(UDS_PATH);
    int srv = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    sockaddr_un addr{}; addr.sun_family = AF_UNIX; std::strcpy(addr.sun_path, UDS_PATH);
    if (bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) { perror("bind"); return 1; }
    listen(srv, 1);
    uds_conn = accept(srv, nullptr, nullptr);
    if (uds_conn == -1) { perror("accept"); return 1; }

    // 3) Tobii eye‑tracker subscribe
    TobiiResearchEyeTracker** list = nullptr; size_t n = tobii_research_find_all_eye_trackers(&list);
    if (n == 0) { fprintf(stderr, "No eye tracker found.\n"); return 1; }
    if (tobii_research_subscribe_to_gaze_data(list[0], gaze_callback, nullptr) != TOBII_RESEARCH_STATUS_OK) {
        fprintf(stderr, "Subscribe failed.\n"); return 1;
    }

    printf("Producer running — press Ctrl+C to quit.\n");
    pause();     // sleep forever
    return 0;
}
