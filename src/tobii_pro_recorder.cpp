#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <conio.h>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#include "tobii_research.h"
#include "tobii_research_streams.h"
#include "tobii_research_eyetracker.h"

// データ記録用の構造体
struct RecordedGazeData {
    // タイムスタンプ
    int64_t device_time_stamp;
    int64_t system_time_stamp;
    
    // 左目データ
    float left_gaze_point_x;
    float left_gaze_point_y;
    bool left_gaze_point_valid;
    
    float left_gaze_origin_x;
    float left_gaze_origin_y;
    float left_gaze_origin_z;
    bool left_gaze_origin_valid;
    
    float left_pupil_diameter;
    bool left_pupil_valid;
    
    // 右目データ
    float right_gaze_point_x;
    float right_gaze_point_y;
    bool right_gaze_point_valid;
    
    float right_gaze_origin_x;
    float right_gaze_origin_y;
    float right_gaze_origin_z;
    bool right_gaze_origin_valid;
    
    float right_pupil_diameter;
    bool right_pupil_valid;
};

// データキュー（低レイテンシのためのダブルバッファリング）
class DataQueue {
private:
    std::queue<RecordedGazeData> queue1;
    std::queue<RecordedGazeData> queue2;
    std::queue<RecordedGazeData>* active_queue;
    std::queue<RecordedGazeData>* processing_queue;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> should_stop{false};
    
public:
    DataQueue() : active_queue(&queue1), processing_queue(&queue2) {}
    
    // コピー禁止
    DataQueue(const DataQueue&) = delete;
    DataQueue& operator=(const DataQueue&) = delete;
    // move も禁止で良い

    void push(const RecordedGazeData& data) {
        std::lock_guard<std::mutex> lock(mutex);
        active_queue->push(data);
    }
    
    std::queue<RecordedGazeData>* swap_and_get() {
        std::unique_lock<std::mutex> lock(mutex);
        std::swap(active_queue, processing_queue);
        cv.notify_one();
        return processing_queue;
    }
    
    void stop() {
        should_stop = true;
        cv.notify_all();
    }
    
    bool is_stopped() const {
        return should_stop;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        queue1 = {};
        queue2 = {};
        active_queue = &queue1;
        processing_queue = &queue2;
        should_stop = false;
    }
};

// グローバル変数
DataQueue g_data_queue;
std::atomic<int64_t> g_sample_count(0);
std::atomic<bool> g_is_recording(false);
std::string g_participant_id;
std::string g_session_info;

// 視線データのコールバック関数（最小限の処理）
void gaze_data_callback(TobiiResearchGazeData* gaze_data, void* user_data)
{
    if (!g_is_recording) return;
    
    RecordedGazeData data;
    
    // タイムスタンプ
    data.device_time_stamp = gaze_data->device_time_stamp;
    data.system_time_stamp = gaze_data->system_time_stamp;
    
    // 左目データ
    data.left_gaze_point_x = gaze_data->left_eye.gaze_point.position_on_display_area.x;
    data.left_gaze_point_y = gaze_data->left_eye.gaze_point.position_on_display_area.y;
    data.left_gaze_point_valid = (gaze_data->left_eye.gaze_point.validity == TOBII_RESEARCH_VALIDITY_VALID);
    
    data.left_gaze_origin_x = gaze_data->left_eye.gaze_origin.position_in_user_coordinates.x;
    data.left_gaze_origin_y = gaze_data->left_eye.gaze_origin.position_in_user_coordinates.y;
    data.left_gaze_origin_z = gaze_data->left_eye.gaze_origin.position_in_user_coordinates.z;
    data.left_gaze_origin_valid = (gaze_data->left_eye.gaze_origin.validity == TOBII_RESEARCH_VALIDITY_VALID);
    
    data.left_pupil_diameter = gaze_data->left_eye.pupil_data.diameter;
    data.left_pupil_valid = (gaze_data->left_eye.pupil_data.validity == TOBII_RESEARCH_VALIDITY_VALID);
    
    // 右目データ
    data.right_gaze_point_x = gaze_data->right_eye.gaze_point.position_on_display_area.x;
    data.right_gaze_point_y = gaze_data->right_eye.gaze_point.position_on_display_area.y;
    data.right_gaze_point_valid = (gaze_data->right_eye.gaze_point.validity == TOBII_RESEARCH_VALIDITY_VALID);
    
    data.right_gaze_origin_x = gaze_data->right_eye.gaze_origin.position_in_user_coordinates.x;
    data.right_gaze_origin_y = gaze_data->right_eye.gaze_origin.position_in_user_coordinates.y;
    data.right_gaze_origin_z = gaze_data->right_eye.gaze_origin.position_in_user_coordinates.z;
    data.right_gaze_origin_valid = (gaze_data->right_eye.gaze_origin.validity == TOBII_RESEARCH_VALIDITY_VALID);
    
    data.right_pupil_diameter = gaze_data->right_eye.pupil_data.diameter;
    data.right_pupil_valid = (gaze_data->right_eye.pupil_data.validity == TOBII_RESEARCH_VALIDITY_VALID);
    
    // キューに追加（低レイテンシ）
    g_data_queue.push(data);
    g_sample_count++;
}

// ファイル書き込みスレッド
void file_writer_thread(const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "エラー: ファイルを開けませんでした: " << filename << std::endl;
        return;
    }
    
    // メタデータを書き込み
    file << "# Tobii Pro SDK Recording" << std::endl;
    file << "# Date: " << std::chrono::system_clock::now().time_since_epoch().count() << std::endl;
    file << "# Participant: " << g_participant_id << std::endl;
    file << "# Session: " << g_session_info << std::endl;
    file << "#" << std::endl;
    
    // ヘッダー行
    file << "device_timestamp_us,system_timestamp_us,"
         << "left_gaze_x,left_gaze_y,left_gaze_valid,"
         << "left_origin_x_mm,left_origin_y_mm,left_origin_z_mm,left_origin_valid,"
         << "left_pupil_diameter_mm,left_pupil_valid,"
         << "right_gaze_x,right_gaze_y,right_gaze_valid,"
         << "right_origin_x_mm,right_origin_y_mm,right_origin_z_mm,right_origin_valid,"
         << "right_pupil_diameter_mm,right_pupil_valid"
         << std::endl;
    
    // 高精度出力設定
    file << std::fixed << std::setprecision(6);
    
    while (!g_data_queue.is_stopped() || g_is_recording) {
        // データを取得
        auto* data_queue = g_data_queue.swap_and_get();
        
        // バッチ書き込み（効率化）
        while (!data_queue->empty()) {
            const auto& data = data_queue->front();
            
            file << data.device_time_stamp << ","
                 << data.system_time_stamp << ","
                 << data.left_gaze_point_x << ","
                 << data.left_gaze_point_y << ","
                 << data.left_gaze_point_valid << ","
                 << data.left_gaze_origin_x << ","
                 << data.left_gaze_origin_y << ","
                 << data.left_gaze_origin_z << ","
                 << data.left_gaze_origin_valid << ","
                 << data.left_pupil_diameter << ","
                 << data.left_pupil_valid << ","
                 << data.right_gaze_point_x << ","
                 << data.right_gaze_point_y << ","
                 << data.right_gaze_point_valid << ","
                 << data.right_gaze_origin_x << ","
                 << data.right_gaze_origin_y << ","
                 << data.right_gaze_origin_z << ","
                 << data.right_gaze_origin_valid << ","
                 << data.right_pupil_diameter << ","
                 << data.right_pupil_valid
                 << std::endl;
            
            data_queue->pop();
        }
        
        // 定期的にフラッシュ
        file.flush();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 残りのデータを書き込み
    auto* final_queue = g_data_queue.swap_and_get();
    while (!final_queue->empty()) {
        const auto& data = final_queue->front();
        file << data.device_time_stamp << "," << data.system_time_stamp << ","
             << data.left_gaze_point_x << "," << data.left_gaze_point_y << ","
             << data.left_gaze_point_valid << ","
             << data.right_gaze_point_x << "," << data.right_gaze_point_y << ","
             << data.right_gaze_point_valid << std::endl;
        final_queue->pop();
    }
    
    file.close();
}

// 現在時刻の文字列取得
std::string get_timestamp_string() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time_t);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return ss.str();
}

// ステータス表示
void display_status(float frequency) {
    static auto start_time = std::chrono::steady_clock::now();
    
    COORD coord = {0, 10};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
    
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(current_time - start_time).count();
    
    std::cout << "\n=== Recording Status ===" << std::endl;
    std::cout << "Recording State: " << (g_is_recording ? "Recording" : "Waiting") << "    " << std::endl;
    std::cout << "Sample Count: " << g_sample_count << std::endl;
    std::cout << "Elapsed Time: " << std::fixed << std::setprecision(1) << elapsed << " seconds" << std::endl;
    std::cout << "Effective Frequency: " << (g_sample_count / elapsed) << " Hz (Set: " << frequency << " Hz)" << std::endl;
    std::cout << "\n[SPACE] Start/Stop Recording  [ESC] Exit" << std::endl;
}

int main()
{
    std::cout << "=== Tobii Pro SDK - High Performance Data Recorder ===" << std::endl;
    std::cout << "Research Low Latency Eye Tracking Data Recording System" << std::endl;
    std::cout << "======================================================" << std::endl;
    
    // 参加者情報の入力
    std::cout << "\nParticipant ID: ";
    std::getline(std::cin, g_participant_id);
    std::cout << "Session Information: ";
    std::getline(std::cin, g_session_info);
    
    // アイトラッカーを検索
    TobiiResearchEyeTrackers* eyetrackers = nullptr;
    TobiiResearchStatus status = tobii_research_find_all_eyetrackers(&eyetrackers);
    
    if (status != TOBII_RESEARCH_STATUS_OK || eyetrackers->count == 0) {
        std::cerr << "エラー: Tobii Pro Eye Tracker not found" << std::endl;
        if (eyetrackers) tobii_research_free_eyetrackers(eyetrackers);
        return 1;
    }
    
    TobiiResearchEyeTracker* eyetracker = eyetrackers->eyetrackers[0];
    
    // デバイス情報を取得
    char* model = nullptr;
    tobii_research_get_model(eyetracker, &model);
    std::cout << "\nDetected Eye Tracker: " << (model ? model : "Unknown") << std::endl;
    tobii_research_free_string(model);
    
    // 最高周波数に設定
    TobiiResearchGazeOutputFrequencies* frequencies = nullptr;
    float max_frequency = 120.0f; // Default
    
    if (tobii_research_get_all_gaze_output_frequencies(eyetracker, &frequencies) == TOBII_RESEARCH_STATUS_OK) {
        max_frequency = frequencies->frequencies[0];
        for (size_t i = 1; i < frequencies->frequency_count; i++) {
            if (frequencies->frequencies[i] > max_frequency) {
                max_frequency = frequencies->frequencies[i];
            }
        }
        
        std::cout << "Setting sampling frequency to " << max_frequency << " Hz" << std::endl;
        tobii_research_set_gaze_output_frequency(eyetracker, max_frequency);
        tobii_research_free_gaze_output_frequencies(frequencies);
    }
    
    // 視線データの購読を開始
    status = tobii_research_subscribe_to_gaze_data(eyetracker, gaze_data_callback, nullptr);
    if (status != TOBII_RESEARCH_STATUS_OK) {
        std::cerr << "エラー: Failed to subscribe to gaze data" << std::endl;
        tobii_research_free_eyetrackers(eyetrackers);
        return 1;
    }
    
    std::cout << "\nReady. Press [SPACE] key to start recording." << std::endl;
    
    // メインループ
    std::thread* writer_thread = nullptr;
    std::string current_filename;
    
    while (true) {
        if (_kbhit()) {
            int key = _getch();
            if (key == 27) { // ESC
                if (g_is_recording) {
                    g_is_recording = false;
                    g_data_queue.stop();
                    if (writer_thread) {
                        writer_thread->join();
                        delete writer_thread;
                    }
                }
                break;
            } else if (key == 32) { // SPACE
                if (!g_is_recording) {
                    // 記録開始
                    g_sample_count = 0;
                    current_filename = "tobii_pro_data_" + g_participant_id + "_" + get_timestamp_string() + ".csv";
                    
                    g_is_recording = true;
                    writer_thread = new std::thread(file_writer_thread, current_filename);
                    
                    std::cout << "\n*** Recording started: " << current_filename << " ***" << std::endl;
                } else {
                    // 記録停止
                    g_is_recording = false;
                    g_data_queue.stop();
                    
                    if (writer_thread) {
                        writer_thread->join();
                        delete writer_thread;
                        writer_thread = nullptr;
                    }
                    
                    std::cout << "\n*** Recording stopped ***" << std::endl;
                    std::cout << "File: " << current_filename << std::endl;
                    std::cout << "Total Sample Count: " << g_sample_count << std::endl;
                    
                    // キューをリセット
                    g_data_queue.clear();
                }
            }
        }
        
        display_status(max_frequency);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // クリーンアップ
    tobii_research_unsubscribe_from_gaze_data(eyetracker, gaze_data_callback);
    tobii_research_free_eyetrackers(eyetrackers);
    
    std::cout << "\nExiting..." << std::endl;
    return 0;
} 