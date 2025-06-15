#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <atomic>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#endif

#include "tobii_research.h"
#include "tobii_research_streams.h"
#include "tobii_research_eyetracker.h"

// データ収集用の構造体
struct GazeDataSample {
    int64_t device_time_stamp;     // デバイスのタイムスタンプ（マイクロ秒）
    int64_t system_time_stamp;     // システムのタイムスタンプ（マイクロ秒）
    
    // 左目データ
    float left_gaze_point_x;
    float left_gaze_point_y;
    TobiiResearchValidity left_gaze_point_validity;
    
    float left_gaze_origin_x;
    float left_gaze_origin_y;
    float left_gaze_origin_z;
    TobiiResearchValidity left_gaze_origin_validity;
    
    float left_pupil_diameter;
    TobiiResearchValidity left_pupil_validity;
    
    // 右目データ
    float right_gaze_point_x;
    float right_gaze_point_y;
    TobiiResearchValidity right_gaze_point_validity;
    
    float right_gaze_origin_x;
    float right_gaze_origin_y;
    float right_gaze_origin_z;
    TobiiResearchValidity right_gaze_origin_validity;
    
    float right_pupil_diameter;
    TobiiResearchValidity right_pupil_validity;
};

// グローバル変数（デモ用）
std::mutex g_data_mutex;
GazeDataSample g_latest_sample;
std::atomic<bool> g_has_new_data(false);
std::atomic<int64_t> g_sample_count(0);
std::atomic<int64_t> g_latency_us(0);

// 視線データのコールバック関数
void gaze_data_callback(TobiiResearchGazeData* gaze_data, void* user_data)
{
    // 現在のシステム時刻を取得（レイテンシ計測用）
    auto now = std::chrono::high_resolution_clock::now();
    auto system_now_us = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();
    
    // レイテンシを計算
    int64_t latency = system_now_us - gaze_data->system_time_stamp;
    g_latency_us = latency;
    
    // データをコピー
    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        
        g_latest_sample.device_time_stamp = gaze_data->device_time_stamp;
        g_latest_sample.system_time_stamp = gaze_data->system_time_stamp;
        
        // 左目データ
        g_latest_sample.left_gaze_point_x = gaze_data->left_eye.gaze_point.position_on_display_area.x;
        g_latest_sample.left_gaze_point_y = gaze_data->left_eye.gaze_point.position_on_display_area.y;
        g_latest_sample.left_gaze_point_validity = gaze_data->left_eye.gaze_point.validity;
        
        g_latest_sample.left_gaze_origin_x = gaze_data->left_eye.gaze_origin.position_in_user_coordinates.x;
        g_latest_sample.left_gaze_origin_y = gaze_data->left_eye.gaze_origin.position_in_user_coordinates.y;
        g_latest_sample.left_gaze_origin_z = gaze_data->left_eye.gaze_origin.position_in_user_coordinates.z;
        g_latest_sample.left_gaze_origin_validity = gaze_data->left_eye.gaze_origin.validity;
        
        g_latest_sample.left_pupil_diameter = gaze_data->left_eye.pupil_data.diameter;
        g_latest_sample.left_pupil_validity = gaze_data->left_eye.pupil_data.validity;
        
        // 右目データ
        g_latest_sample.right_gaze_point_x = gaze_data->right_eye.gaze_point.position_on_display_area.x;
        g_latest_sample.right_gaze_point_y = gaze_data->right_eye.gaze_point.position_on_display_area.y;
        g_latest_sample.right_gaze_point_validity = gaze_data->right_eye.gaze_point.validity;
        
        g_latest_sample.right_gaze_origin_x = gaze_data->right_eye.gaze_origin.position_in_user_coordinates.x;
        g_latest_sample.right_gaze_origin_y = gaze_data->right_eye.gaze_origin.position_in_user_coordinates.y;
        g_latest_sample.right_gaze_origin_z = gaze_data->right_eye.gaze_origin.position_in_user_coordinates.z;
        g_latest_sample.right_gaze_origin_validity = gaze_data->right_eye.gaze_origin.validity;
        
        g_latest_sample.right_pupil_diameter = gaze_data->right_eye.pupil_data.diameter;
        g_latest_sample.right_pupil_validity = gaze_data->right_eye.pupil_data.validity;
    }
    
    g_has_new_data = true;
    g_sample_count++;
}

// 有効性の文字列表現
const char* validity_to_string(TobiiResearchValidity validity)
{
    return (validity == TOBII_RESEARCH_VALIDITY_VALID) ? "Valid" : "Invalid";
}

// アイトラッカー情報を表示
void print_eyetracker_info(TobiiResearchEyeTracker* eyetracker)
{
    char* serial_number = nullptr;
    char* model = nullptr;
    char* firmware_version = nullptr;
    char* device_name = nullptr;
    TobiiResearchCapabilities capabilities;
    
    tobii_research_get_serial_number(eyetracker, &serial_number);
    tobii_research_get_model(eyetracker, &model);
    tobii_research_get_firmware_version(eyetracker, &firmware_version);
    tobii_research_get_device_name(eyetracker, &device_name);
    tobii_research_get_capabilities(eyetracker, &capabilities);
    
    std::cout << "\n=== Tobii Pro SDK - Low-Latency Gaze Viewer ===" << std::endl;
    std::cout << "Name: " << (device_name ? device_name : "Unknown") << std::endl;
    std::cout << "Model: " << (model ? model : "Unknown") << std::endl;
    std::cout << "Serial Number: " << (serial_number ? serial_number : "Unknown") << std::endl;
    std::cout << "Firmware Version: " << (firmware_version ? firmware_version : "Unknown") << std::endl;
    
    // 利用可能な周波数を表示
    TobiiResearchGazeOutputFrequencies* frequencies = nullptr;
    if (tobii_research_get_all_gaze_output_frequencies(eyetracker, &frequencies) == TOBII_RESEARCH_STATUS_OK)
    {
        std::cout << "Available Sampling Frequencies: ";
        for (size_t i = 0; i < frequencies->frequency_count; i++)
        {
            std::cout << frequencies->frequencies[i] << "Hz ";
        }
        std::cout << std::endl;
        tobii_research_free_gaze_output_frequencies(frequencies);
    }
    
    // 現在の周波数を取得
    float current_frequency;
    if (tobii_research_get_gaze_output_frequency(eyetracker, &current_frequency) == TOBII_RESEARCH_STATUS_OK)
    {
        std::cout << "Current Sampling Frequency: " << current_frequency << "Hz" << std::endl;
    }
    
    // メモリ解放
    tobii_research_free_string(serial_number);
    tobii_research_free_string(model);
    tobii_research_free_string(firmware_version);
    tobii_research_free_string(device_name);
    if (tobii_research_get_capabilities(eyetracker, &capabilities) == TOBII_RESEARCH_STATUS_OK)
    {
        // capabilities can be used if needed; currently not displayed
    }
}

// リアルタイムデータ表示
void display_realtime_data()
{
    static auto start_time = std::chrono::steady_clock::now();
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed_seconds = std::chrono::duration<double>(current_time - start_time).count();
    
    // カーソル位置をリセット
    std::cout << "\033[H\033[J"; // Clear screen (Linux/Mac)
    
    std::cout << "=== Tobii Pro SDK - Low-Latency Gaze Viewer ===" << std::endl;
    std::cout << "Elapsed Time: " << std::fixed << std::setprecision(1) << elapsed_seconds << " seconds" << std::endl;
    std::cout << "Sample Count: " << g_sample_count << std::endl;
    std::cout << "Average Frequency: " << std::setprecision(1) << (g_sample_count / elapsed_seconds) << " Hz" << std::endl;
    std::cout << "Latency: " << g_latency_us / 1000.0 << " ms" << std::endl;
    
    std::lock_guard<std::mutex> lock(g_data_mutex);
    
    std::cout << "\n=== Left Eye ===" << std::endl;
    std::cout << "Gaze Position: (" << std::setprecision(3) 
              << g_latest_sample.left_gaze_point_x << ", " 
              << g_latest_sample.left_gaze_point_y << ") " 
              << validity_to_string(g_latest_sample.left_gaze_point_validity) << std::endl;
    std::cout << "Eyeball Position: (" 
              << g_latest_sample.left_gaze_origin_x << ", " 
              << g_latest_sample.left_gaze_origin_y << ", " 
              << g_latest_sample.left_gaze_origin_z << ") mm " 
              << validity_to_string(g_latest_sample.left_gaze_origin_validity) << std::endl;
    std::cout << "Pupil Diameter: " << g_latest_sample.left_pupil_diameter << " mm " 
              << validity_to_string(g_latest_sample.left_pupil_validity) << std::endl;
    
    std::cout << "\n=== Right Eye ===" << std::endl;
    std::cout << "Gaze Position: (" 
              << g_latest_sample.right_gaze_point_x << ", " 
              << g_latest_sample.right_gaze_point_y << ") " 
              << validity_to_string(g_latest_sample.right_gaze_point_validity) << std::endl;
    std::cout << "Eyeball Position: (" 
              << g_latest_sample.right_gaze_origin_x << ", " 
              << g_latest_sample.right_gaze_origin_y << ", " 
              << g_latest_sample.right_gaze_origin_z << ") mm " 
              << validity_to_string(g_latest_sample.right_gaze_origin_validity) << std::endl;
    std::cout << "Pupil Diameter: " << g_latest_sample.right_pupil_diameter << " mm " 
              << validity_to_string(g_latest_sample.right_pupil_validity) << std::endl;
    
    std::cout << "\n[Ctrl+C] to exit" << std::endl;
}

int main()
{
    TobiiResearchStatus status;
    
    std::cout << "Tobii Pro SDK - Low-Latency Gaze Viewer" << std::endl;
    std::cout << "======================================" << std::endl;
    
    // アイトラッカーを検索
    TobiiResearchEyeTrackers* eyetrackers = nullptr;
    status = tobii_research_find_all_eyetrackers(&eyetrackers);
    
    if (status != TOBII_RESEARCH_STATUS_OK)
    {
        std::cerr << "Error: Failed to find eye trackers" << std::endl;
        return 1;
    }
    
    if (eyetrackers->count == 0)
    {
        std::cerr << "Error: Eye trackers not found" << std::endl;
        std::cerr << "Please ensure a Tobii Pro eye tracker is connected" << std::endl;
        tobii_research_free_eyetrackers(eyetrackers);
        return 1;
    }
    
    // 最初のアイトラッカーを使用
    TobiiResearchEyeTracker* eyetracker = eyetrackers->eyetrackers[0];
    
    // アイトラッカー情報を表示
    print_eyetracker_info(eyetracker);
    
    // 最高周波数に設定（低レイテンシのため）
    TobiiResearchGazeOutputFrequencies* frequencies = nullptr;
    status = tobii_research_get_all_gaze_output_frequencies(eyetracker, &frequencies);
    if (status == TOBII_RESEARCH_STATUS_OK && frequencies->frequency_count > 0)
    {
        // 最高周波数を選択
        float max_frequency = frequencies->frequencies[0];
        for (size_t i = 1; i < frequencies->frequency_count; i++)
        {
            if (frequencies->frequencies[i] > max_frequency)
            {
                max_frequency = frequencies->frequencies[i];
            }
        }
        
        std::cout << "\nSetting highest frequency " << max_frequency << "Hz..." << std::endl;
        status = tobii_research_set_gaze_output_frequency(eyetracker, max_frequency);
        if (status == TOBII_RESEARCH_STATUS_OK)
        {
            std::cout << "Frequency set successfully" << std::endl;
        }
        
        tobii_research_free_gaze_output_frequencies(frequencies);
    }
    
    // 視線データの購読を開始
    std::cout << "\nStarting to subscribe to gaze data..." << std::endl;
    status = tobii_research_subscribe_to_gaze_data(eyetracker, gaze_data_callback, nullptr);
    if (status != TOBII_RESEARCH_STATUS_OK)
    {
        std::cerr << "Error: Failed to subscribe to gaze data" << std::endl;
        tobii_research_free_eyetrackers(eyetrackers);
        return 1;
    }
    
    std::cout << "Data acquisition started\n" << std::endl;
    
    // メインループ（リアルタイム表示）
    while (true)
    {
        display_realtime_data();
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 20Hz更新
    }
    
    // クリーンアップ（実際には到達しない）
    tobii_research_unsubscribe_from_gaze_data(eyetracker, gaze_data_callback);
    tobii_research_free_eyetrackers(eyetrackers);
    
    return 0;
} 