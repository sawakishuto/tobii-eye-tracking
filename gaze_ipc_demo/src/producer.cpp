#include "ring.hpp"
extern "C" {
#include <tobii_research.h>
#include <tobii_research_streams.h>
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
    s.ts    = gd->system_time_stamp / 1000.0;   // µs → ms
    
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
    
    send(uds_conn, "1", 1, MSG_DONTWAIT);   // wake consumer (non‑block)
}

int main() {
    // 1) shared memory create & map
    // Clean up any existing shared memory first
    shm_unlink(SHM_NAME);
    
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600);
    if (shm_fd == -1) { perror("shm_open"); return 1; }
    
    if (ftruncate(shm_fd, sizeof(RingBuffer)) == -1) { 
        perror("ftruncate");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return 1; 
    }
    
    rb = static_cast<RingBuffer*>(mmap(nullptr, sizeof(RingBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));
    if (rb == MAP_FAILED) { 
        perror("mmap"); 
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return 1; 
    }
    memset(rb, 0, sizeof(RingBuffer));
    close(shm_fd);  // Can close fd after mmap

    // 2) UDS server set‑up
    ::unlink(UDS_PATH);
    int srv = socket(AF_UNIX, SOCK_STREAM, 0);
    if (srv == -1) { perror("socket"); return 1; }
    
    sockaddr_un addr{}; 
    addr.sun_family = AF_UNIX; 
    std::strcpy(addr.sun_path, UDS_PATH);
    
    if (bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) { 
        perror("bind"); 
        close(srv);
        return 1; 
    }
    
    if (listen(srv, 1) == -1) { 
        perror("listen"); 
        close(srv);
        return 1; 
    }
    
    printf("Waiting for consumer to connect...\n");
    uds_conn = accept(srv, nullptr, nullptr);
    if (uds_conn == -1) { perror("accept"); close(srv); return 1; }
    printf("Consumer connected.\n");
    close(srv);  // Close the server socket, keep the connection

    // 3) Tobii eye‑tracker subscribe
    TobiiResearchEyeTrackers* eyetrackers = nullptr;
    TobiiResearchStatus status = tobii_research_find_all_eyetrackers(&eyetrackers);
    if (status != TOBII_RESEARCH_STATUS_OK) {
        fprintf(stderr, "Failed to find eye trackers. Status: %d\n", status);
        return 1;
    }
    
    if (eyetrackers->count == 0) {
        fprintf(stderr, "No eye tracker found.\n");
        tobii_research_free_eyetrackers(eyetrackers);
        return 1;
    }
    
    TobiiResearchEyeTracker* eyetracker = eyetrackers->eyetrackers[0];
    
    printf("Starting gaze data subscription...\n");
    status = tobii_research_subscribe_to_gaze_data(eyetracker, gaze_callback, nullptr);
    if (status != TOBII_RESEARCH_STATUS_OK) {
        fprintf(stderr, "Subscribe failed. Status: %d\n", status);
        tobii_research_free_eyetrackers(eyetrackers);
        return 1;
    }

    printf("Producer running with real eye tracker — press Ctrl+C to quit.\n");
    pause();     // sleep forever
    
    // Cleanup (though we never reach here due to pause())
    tobii_research_unsubscribe_from_gaze_data(eyetracker, gaze_callback);
    tobii_research_free_eyetrackers(eyetrackers);
    return 0;
}
