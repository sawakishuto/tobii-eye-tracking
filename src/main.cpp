#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <cassert>

// グローバル変数（デモ用）
double g_gaze_x = 0.0;
double g_gaze_y = 0.0;
bool g_gaze_valid = false;

// 視線データのコールバック関数
void gaze_point_callback(tobii_gaze_point_t const* gaze_point, void* user_data)
{
    if (gaze_point->validity == TOBII_VALIDITY_VALID)
    {
        g_gaze_x = gaze_point->position_xy[0];
        g_gaze_y = gaze_point->position_xy[1];
        g_gaze_valid = true;
        
        std::cout << "視線位置: (" 
                  << g_gaze_x << ", " 
                  << g_gaze_y << ")" 
                  << std::endl;
    }
    else
    {
        g_gaze_valid = false;
        std::cout << "視線データが無効です" << std::endl;
    }
}

// デバイスURLを受け取るコールバック関数
void url_receiver(char const* url, void* user_data)
{
    char* buffer = (char*)user_data;
    if (*buffer != '\0') return; // 最初のデバイスのみ使用

    if (strlen(url) < 256)
        strcpy(buffer, url);
}

int main()
{
    std::cout << "====================================" << std::endl;
    std::cout << "Tobii アイトラッカー 視線取得デモ" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << std::endl;

    tobii_error_t result;
    
    // Tobii APIの作成
    tobii_api_t* api = nullptr;
    result = tobii_api_create(&api, nullptr, nullptr);
    if (result != TOBII_ERROR_NO_ERROR)
    {
        std::cerr << "エラー: Tobii APIの作成に失敗しました" << std::endl;
        return 1;
    }
    
    std::cout << "Tobii APIを初期化しました" << std::endl;

    // 接続されているアイトラッカーを検索
    char url[256] = { 0 };
    result = tobii_enumerate_local_device_urls(api, url_receiver, url);
    if (result != TOBII_ERROR_NO_ERROR)
    {
        std::cerr << "エラー: デバイスの列挙に失敗しました" << std::endl;
        tobii_api_destroy(api);
        return 1;
    }
    
    if (*url == '\0')
    {
        std::cerr << "エラー: Tobiiアイトラッカーが見つかりません" << std::endl;
        std::cerr << "アイトラッカーが接続されていることを確認してください" << std::endl;
        tobii_api_destroy(api);
        return 1;
    }
    
    std::cout << "アイトラッカーを検出しました: " << url << std::endl;

    // アイトラッカーに接続
    tobii_device_t* device = nullptr;
    result = tobii_device_create(api, url, TOBII_FIELD_OF_USE_STORE_OR_TRANSFER_FALSE, &device);
    if (result != TOBII_ERROR_NO_ERROR)
    {
        std::cerr << "エラー: デバイスへの接続に失敗しました" << std::endl;
        tobii_api_destroy(api);
        return 1;
    }
    
    std::cout << "アイトラッカーに接続しました" << std::endl;

    // 視線データの購読を開始
    result = tobii_gaze_point_subscribe(device, gaze_point_callback, nullptr);
    if (result != TOBII_ERROR_NO_ERROR)
    {
        std::cerr << "エラー: 視線データの購読に失敗しました" << std::endl;
        tobii_device_destroy(device);
        tobii_api_destroy(api);
        return 1;
    }
    
    std::cout << std::endl;
    std::cout << "視線データの取得を開始しました" << std::endl;
    std::cout << "終了するには Ctrl+C を押してください" << std::endl;
    std::cout << std::endl;

    // メインループ（視線データを継続的に取得）
    while (true)
    {
        // コールバックを待つ（最大1000ミリ秒）
        result = tobii_wait_for_callbacks(1, &device);
        if (result == TOBII_ERROR_TIMED_OUT)
        {
            continue;
        }
        else if (result != TOBII_ERROR_NO_ERROR)
        {
            std::cerr << "エラー: コールバック待機中にエラーが発生しました" << std::endl;
            break;
        }

        // コールバックを処理
        result = tobii_device_process_callbacks(device);
        if (result != TOBII_ERROR_NO_ERROR)
        {
            std::cerr << "エラー: コールバック処理中にエラーが発生しました" << std::endl;
            break;
        }

        // 少し待機（CPU使用率を下げるため）
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // クリーンアップ
    std::cout << std::endl;
    std::cout << "クリーンアップ中..." << std::endl;
    
    tobii_gaze_point_unsubscribe(device);
    tobii_device_destroy(device);
    tobii_api_destroy(api);
    
    std::cout << "終了しました" << std::endl;
    
    return 0;
} 