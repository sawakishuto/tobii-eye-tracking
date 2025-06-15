#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <conio.h>  // Windows用（_kbhit, _getch）

#include "tobii_gameintegration.h"
#include "windows.h"

using namespace TobiiGameIntegration;

void DisplayGazeInfo(const GazePoint& gazePoint, const HeadPose& headPose, bool isPresent)
{
    // カーソルを先頭に戻す（画面をクリアせずに上書き）
    COORD coord = {0, 5};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
    
    // 視線情報の表示
    std::cout << "=== 視線情報 ===" << std::endl;
    std::cout << "視線位置: (" << std::fixed << std::setprecision(2) 
              << std::setw(7) << gazePoint.X << ", " 
              << std::setw(7) << gazePoint.Y << ")" << std::endl;
    
    // 頭部姿勢の表示
    std::cout << "\n=== 頭部姿勢 ===" << std::endl;
    std::cout << "ヨー  : " << std::setw(7) << headPose.Rotation.YawDegrees << " 度" << std::endl;
    std::cout << "ピッチ: " << std::setw(7) << headPose.Rotation.PitchDegrees << " 度" << std::endl;
    std::cout << "ロール: " << std::setw(7) << headPose.Rotation.RollDegrees << " 度" << std::endl;
    
    // 頭部位置の表示
    std::cout << "\n=== 頭部位置 ===" << std::endl;
    std::cout << "X: " << std::setw(7) << headPose.Position.X << " mm" << std::endl;
    std::cout << "Y: " << std::setw(7) << headPose.Position.Y << " mm" << std::endl;
    std::cout << "Z: " << std::setw(7) << headPose.Position.Z << " mm" << std::endl;
    
    // ユーザー存在状態
    std::cout << "\n=== ユーザー状態 ===" << std::endl;
    std::cout << "ユーザー検出: " << (isPresent ? "あり" : "なし") << "    " << std::endl;
}

int main()
{
    std::cout << "=====================================" << std::endl;
    std::cout << "Tobii Game Integration API デモ" << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << "[ESC]キーで終了" << std::endl;
    std::cout << std::endl;

    // Tobii Game Integration APIの初期化
    ITobiiGameIntegrationApi* api = GetApi("Tobii Eye Tracking Demo");
    if (!api)
    {
        std::cerr << "エラー: Tobii Game Integration APIの初期化に失敗しました" << std::endl;
        return 1;
    }

    // ストリームプロバイダーの取得
    IStreamsProvider* streamsProvider = api->GetStreamsProvider();
    if (!streamsProvider)
    {
        std::cerr << "エラー: ストリームプロバイダーの取得に失敗しました" << std::endl;
        api->Shutdown();
        return 1;
    }

    // トラッカーコントローラーの取得
    ITrackerController* trackerController = api->GetTrackerController();
    if (!trackerController)
    {
        std::cerr << "エラー: トラッカーコントローラーの取得に失敗しました" << std::endl;
        api->Shutdown();
        return 1;
    }

    // コンソールウィンドウをトラッキング対象に設定
    trackerController->TrackWindow(GetConsoleWindow());

    std::cout << "アイトラッカーの初期化中..." << std::endl;
    
    // トラッカーが準備できるまで待機
    bool trackerFound = false;
    for (int i = 0; i < 50; i++)  // 最大5秒待機
    {
        api->Update();
        if (trackerController->GetTrackerInfo() != nullptr)
        {
            trackerFound = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!trackerFound)
    {
        std::cerr << "エラー: アイトラッカーが見つかりません" << std::endl;
        std::cerr << "アイトラッカーが接続されていることを確認してください" << std::endl;
        api->Shutdown();
        return 1;
    }

    // トラッカー情報の表示
    auto trackerInfo = trackerController->GetTrackerInfo();
    if (trackerInfo)
    {
        std::cout << "\nアイトラッカー情報:" << std::endl;
        std::cout << "モデル: " << trackerInfo->GetTrackerModel() << std::endl;
        std::cout << "シリアル番号: " << trackerInfo->GetSerialNumber() << std::endl;
        std::cout << "ファームウェア: " << trackerInfo->GetFirmwareVersion() << std::endl;
    }

    std::cout << "\n視線データの取得を開始しました..." << std::endl;
    std::cout << std::endl;

    // メインループ
    while (true)
    {
        // ESCキーが押されたか確認（Windows固有）
        if (_kbhit() && _getch() == 27)  // ESCキーのASCIIコードは27
        {
            break;
        }

        // APIの更新
        api->Update();

        // 最新の視線データを取得
        GazePoint gazePoint;
        bool hasGazeData = streamsProvider->GetLatestGazePoint(gazePoint);

        // 最新の頭部姿勢データを取得
        HeadPose headPose;
        bool hasHeadData = streamsProvider->GetLatestHeadPose(headPose);

        // ユーザーの存在状態を取得
        bool userIsPresent = streamsProvider->IsPresent();

        // データが取得できた場合のみ表示を更新
        if (hasGazeData || hasHeadData)
        {
            DisplayGazeInfo(gazePoint, headPose, userIsPresent);
        }

        // フレームレート制御（約60fps）
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // クリーンアップ
    std::cout << "\n\nクリーンアップ中..." << std::endl;
    api->Shutdown();
    std::cout << "終了しました" << std::endl;

    return 0;
} 