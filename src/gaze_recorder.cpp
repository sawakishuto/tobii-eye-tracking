#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <sstream>
#include <thread>
#include <conio.h>
#include <vector>
#include <string>

#include "tobii_gameintegration.h"
#include "windows.h"

using namespace TobiiGameIntegration;
using namespace std::chrono;

struct GazeData {
    double timestamp;
    double gazeX;
    double gazeY;
    double headYaw;
    double headPitch;
    double headRoll;
    double headX;
    double headY;
    double headZ;
    bool userPresent;
};

std::string GetCurrentTimeString() {
    auto now = system_clock::now();
    auto time_t = system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time_t);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return ss.str();
}

void SaveToCSV(const std::string& filename, const std::vector<GazeData>& data) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "エラー: ファイルを開けませんでした: " << filename << std::endl;
        return;
    }
    
    // ヘッダー行
    file << "Timestamp(ms),GazeX,GazeY,HeadYaw(deg),HeadPitch(deg),HeadRoll(deg),"
         << "HeadX(mm),HeadY(mm),HeadZ(mm),UserPresent\n";
    
    // データ行
    for (const auto& d : data) {
        file << std::fixed << std::setprecision(3)
             << d.timestamp << ","
             << d.gazeX << ","
             << d.gazeY << ","
             << d.headYaw << ","
             << d.headPitch << ","
             << d.headRoll << ","
             << d.headX << ","
             << d.headY << ","
             << d.headZ << ","
             << (d.userPresent ? "1" : "0") << "\n";
    }
    
    file.close();
    std::cout << "\nデータを保存しました: " << filename << std::endl;
    std::cout << "記録数: " << data.size() << " サンプル" << std::endl;
}

void DisplayStatus(int sampleCount, double elapsedSeconds, const GazeData& latestData) {
    COORD coord = {0, 5};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
    
    std::cout << "=== 記録状態 ===" << std::endl;
    std::cout << "経過時間: " << std::fixed << std::setprecision(1) 
              << elapsedSeconds << " 秒" << std::endl;
    std::cout << "サンプル数: " << sampleCount << std::endl;
    std::cout << "サンプリングレート: " << std::setprecision(1) 
              << (sampleCount / elapsedSeconds) << " Hz" << std::endl;
    
    std::cout << "\n=== 最新データ ===" << std::endl;
    std::cout << "視線位置: (" << std::setprecision(2) 
              << latestData.gazeX << ", " << latestData.gazeY << ")" << std::endl;
    std::cout << "頭部角度: Y:" << latestData.headYaw 
              << " P:" << latestData.headPitch 
              << " R:" << latestData.headRoll << std::endl;
    std::cout << "ユーザー: " << (latestData.userPresent ? "検出" : "未検出") << "    " << std::endl;
}

int main() {
    std::cout << "=====================================" << std::endl;
    std::cout << "Tobii 視線データレコーダー" << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << "[SPACE]キーで記録開始/停止" << std::endl;
    std::cout << "[ESC]キーで終了" << std::endl;
    std::cout << std::endl;

    // Tobii Game Integration APIの初期化
    ITobiiGameIntegrationApi* api = GetApi("Gaze Recorder");
    if (!api) {
        std::cerr << "エラー: APIの初期化に失敗しました" << std::endl;
        return 1;
    }

    auto* streamsProvider = api->GetStreamsProvider();
    auto* trackerController = api->GetTrackerController();
    
    // コンソールウィンドウをトラッキング対象に設定
    trackerController->TrackWindow(GetConsoleWindow());

    // トラッカーの検出待機
    std::cout << "アイトラッカーを検出中..." << std::endl;
    bool trackerFound = false;
    for (int i = 0; i < 50; i++) {
        api->Update();
        if (trackerController->GetTrackerInfo() != nullptr) {
            trackerFound = true;
            break;
        }
        std::this_thread::sleep_for(milliseconds(100));
    }

    if (!trackerFound) {
        std::cerr << "エラー: アイトラッカーが見つかりません" << std::endl;
        api->Shutdown();
        return 1;
    }

    std::cout << "アイトラッカーを検出しました" << std::endl;
    std::cout << "\n準備完了。SPACEキーで記録を開始します。" << std::endl;

    std::vector<GazeData> recordedData;
    bool isRecording = false;
    auto startTime = steady_clock::now();
    auto recordingStartTime = startTime;

    // メインループ
    while (true) {
        // キー入力チェック
        if (_kbhit()) {
            int key = _getch();
            if (key == 27) {  // ESC
                break;
            } else if (key == 32) {  // SPACE
                if (!isRecording) {
                    // 記録開始
                    isRecording = true;
                    recordedData.clear();
                    recordingStartTime = steady_clock::now();
                    std::cout << "\n\n*** 記録を開始しました ***\n" << std::endl;
                } else {
                    // 記録停止
                    isRecording = false;
                    std::cout << "\n\n*** 記録を停止しました ***" << std::endl;
                    
                    // データを保存
                    if (!recordedData.empty()) {
                        std::string filename = "gaze_data_" + GetCurrentTimeString() + ".csv";
                        SaveToCSV(filename, recordedData);
                    }
                }
            }
        }

        // APIの更新
        api->Update();

        // データの取得
        GazePoint gazePoint;
        HeadPose headPose;
        bool hasGaze = streamsProvider->GetLatestGazePoint(gazePoint);
        bool hasHead = streamsProvider->GetLatestHeadPose(headPose);
        bool userPresent = streamsProvider->IsPresent();

        if (hasGaze || hasHead) {
            auto currentTime = steady_clock::now();
            double timestamp = duration<double, std::milli>(currentTime - recordingStartTime).count();
            
            GazeData data = {
                timestamp,
                gazePoint.X,
                gazePoint.Y,
                headPose.Rotation.YawDegrees,
                headPose.Rotation.PitchDegrees,
                headPose.Rotation.RollDegrees,
                headPose.Position.X,
                headPose.Position.Y,
                headPose.Position.Z,
                userPresent
            };

            if (isRecording) {
                recordedData.push_back(data);
            }

            // ステータス表示
            double elapsedSeconds = duration<double>(currentTime - recordingStartTime).count();
            DisplayStatus(
                isRecording ? recordedData.size() : 0,
                isRecording ? elapsedSeconds : 0.0,
                data
            );
        }

        // フレームレート制御
        std::this_thread::sleep_for(milliseconds(8));  // 約120Hz
    }

    // 記録中のデータがあれば保存
    if (isRecording && !recordedData.empty()) {
        std::string filename = "gaze_data_" + GetCurrentTimeString() + ".csv";
        SaveToCSV(filename, recordedData);
    }

    // クリーンアップ
    api->Shutdown();
    std::cout << "\n終了しました" << std::endl;

    return 0;
} 