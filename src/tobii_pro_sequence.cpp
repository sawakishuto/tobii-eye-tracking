#include <windows.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include "tobii_research.h"
#include "tobii_research_streams.h"

static std::atomic<float> g_gazeX{ -1.f };
static std::atomic<float> g_gazeY{ -1.f };
static std::atomic<bool> g_gazeUpdated{ false };

// セット1：四半期売上レビュー
static const std::vector<std::wstring> g_set1 = {
    L"来週火曜日までに四半期売上報告書の最終版を共有ドライブへアップロードしてください。",
    L"当日は午前10時から経営陣レビュー会議があり、佐藤さんが必ず参加しますので準備を万全にお願いします。",
    L"さらに、今月30日18時までに請求書処理をERPシステムへ入力・確定してください。",
    L"本日中に公開された新しい顧客管理システムのオンライン研修を受講し、完了証明を提出することが必須です。",
    L"また、明後日9時には倉庫で販促資料を受け取り、会議室Bへ搬入してください。",
    L"最後に、ネットワークアップグレードの影響で本日21時から3時間社内Wi-Fiが停止します。",
};

// セット2：製品ローンチ準備
static const std::vector<std::wstring> g_set2 = {
    L"今週金曜日までに製品紹介スライドの最新版をマーケティング共有フォルダへ保存してください。",
    L"同じ日の午後2時にはメディア向けデモがあり、鈴木さんが登壇しますので、リハーサルを含む準備をお願いいたします。",
    L"併せて、今月28日16時までに予算申請を経理システムへ提出してください。",
    L"本日中に新しいロゴガイドラインのオンライン講座を視聴し、完了報告を提出することが必須です。",
    L"明朝8時には倉庫でデモ機を受け取り、ホールCへ搬入してください。",
    L"加えて、電源工事のため本日20時から1時間ショールームの照明が消灯します。",
    L"作業計画に留意しつつ、顧客登録フォームのデータをJSON形式で木曜日18時までに私へ共有してください。"
};

// セット3：年次カンファレンス準備
static const std::vector<std::wstring> g_set3 = {
    L"来週水曜日までに年次会議の議題案を共有ドライブの\"Conference25\"フォルダへアップロードしてください。",
    L"同日の午前11時には基調講演リハーサルがあり、高橋さんが必ず参加しますので対応をお願いします。",
    L"また、今月27日15時までに旅費精算を旅費システムへ登録してください。",
    L"本日中に参加者管理アプリのチュートリアル動画を視聴し、完了スクリーンショットを提出してください。",
    L"さらに、明後日7時に印刷会社でパンフレットを受け取り、ホールDへ搬入する手配をお願いします。",
    L"なお、クラウドメンテナンスのため本日23時から2時間オンライン登録サイトが停止します。",
    L"この時間帯を避けて作業を進めつつ、参加者アンケートの初期集計をGoogleスプレッドシート形式で木曜日20時までに私へ送付してください。"
};

// 全セットを結合
static std::vector<std::wstring> g_sections;
static int g_currentSet = 0; // 現在のセット番号 (0, 1, 2)
static int g_currentSection = 0;

constexpr int kGazeMarginPx = 40; // tolerance around last character
constexpr int kDwellTimeMs = 500; //5 gaze dwell time before advancing (in milliseconds)

static std::chrono::steady_clock::time_point g_gazeStart;
static bool g_inTarget = false;
static bool g_showGaze = false; // toggle visualization - disabled by default to reduce flickering
static bool g_seenFirst = false; // whether the first char has been gazed at in current section

// Initialize sections with the first set
void initializeSections() {
    g_sections.clear();
    switch (g_currentSet) {
    case 0: g_sections = g_set1; break;
    case 1: g_sections = g_set2; break;
    case 2: g_sections = g_set3; break;
    }
    g_currentSection = 0;
    g_seenFirst = false;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));

        // Draw current section centered
        std::wstring text = g_sections[g_currentSection];
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));

        HFONT hFont = CreateFontW(32, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                  DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        HFONT old = (HFONT)SelectObject(hdc, hFont);

        SIZE sz;
        GetTextExtentPoint32W(hdc, text.c_str(), (int)text.size(), &sz);
        int x = (rc.right - sz.cx) / 2;
        int y = (rc.bottom - sz.cy) / 2;
        TextOutW(hdc, x, y, text.c_str(), (int)text.size());

        // Display set and section info in top-right corner
        HFONT hSmallFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                       DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        SelectObject(hdc, hSmallFont);
        std::wstring info = L"セット " + std::to_wstring(g_currentSet + 1) + L"/3 - " +
                           std::to_wstring(g_currentSection + 1) + L"/" + std::to_wstring(g_sections.size());
        SIZE infoSz;
        GetTextExtentPoint32W(hdc, info.c_str(), (int)info.size(), &infoSz);
        TextOutW(hdc, rc.right - infoSz.cx - 10, 10, info.c_str(), (int)info.size());
        DeleteObject(hSmallFont);

        SelectObject(hdc, old);
        DeleteObject(hFont);

        // compute last char rect for debug if needed
        SIZE szLast;
        GetTextExtentPoint32W(hdc, text.c_str() + text.size() - 1, 1, &szLast);
        RECT lastRect{ x + sz.cx - szLast.cx, y, x + sz.cx, y + sz.cy };
        FrameRect(hdc, &lastRect, (HBRUSH)GetStockObject(GRAY_BRUSH)); // visualize target

        // Draw gaze point if visualization enabled
        if (g_showGaze)
        {
            float gx = g_gazeX.load();
            float gy = g_gazeY.load();
            if (gx >= 0 && gy >= 0)
            {
                int px = (int)(gx * (rc.right - rc.left));
                int py = (int)(gy * (rc.bottom - rc.top));

                // red filled circle
                HBRUSH hDot = CreateSolidBrush(RGB(255, 0, 0));
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hDot);
                HPEN oldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
                int r = 6;
                Ellipse(hdc, px - r, py - r, px + r, py + r);
                SelectObject(hdc, oldBrush);
                SelectObject(hdc, oldPen);
                DeleteObject(hDot);

                // show numeric coordinates top-left
                std::wstring coord = L"(" + std::to_wstring(px) + L", " + std::to_wstring(py) + L")";
                TextOutW(hdc, 10, 10, coord.c_str(), (int)coord.size());
            }
        }

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_KEYDOWN:
        if (wParam == 'G')
        {
            g_showGaze = !g_showGaze;
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        return 0;
    case WM_TIMER:
        // Reduce flickering by limiting redraws - only update gaze visualization when needed
        if (g_showGaze && g_gazeUpdated.exchange(false)) {
            // Only invalidate the gaze area instead of the whole window
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void gaze_callback(TobiiResearchGazeData* gaze, void*)
{
    const TobiiResearchGazePoint& lgp = gaze->left_eye.gaze_point;
    const TobiiResearchGazePoint& rgp = gaze->right_eye.gaze_point;
    float x = -1, y = -1;
    if (lgp.validity == TOBII_RESEARCH_VALIDITY_VALID) { x = lgp.position_on_display_area.x; y = lgp.position_on_display_area.y; }
    else if (rgp.validity == TOBII_RESEARCH_VALIDITY_VALID) { x = rgp.position_on_display_area.x; y = rgp.position_on_display_area.y; }
    if (x >= 0 && y >= 0) {
        g_gazeX.store(x);
        g_gazeY.store(y);
        g_gazeUpdated.store(true);
    }
}

bool isInsideLastChar(HWND hWnd)
{
    float gx = g_gazeX.load();
    float gy = g_gazeY.load();
    if (gx < 0) return false;

    RECT rc; GetClientRect(hWnd, &rc);
    int screenW = rc.right - rc.left;
    int screenH = rc.bottom - rc.top;
    int px = (int)(gx * screenW);
    int py = (int)(gy * screenH);

    HDC hdc = GetDC(hWnd);
    std::wstring text = g_sections[g_currentSection];
    HFONT hFont = CreateFontW(32, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT old = (HFONT)SelectObject(hdc, hFont);
    SIZE szText, szLast;
    GetTextExtentPoint32W(hdc, text.c_str(), (int)text.size(), &szText);
    GetTextExtentPoint32W(hdc, text.c_str() + text.size() - 1, 1, &szLast);
    int x = (rc.right - szText.cx) / 2;
    int y = (rc.bottom - szText.cy) / 2;
    RECT last{ x + szText.cx - szLast.cx, y, x + szText.cx, y + szText.cy };
    SelectObject(hdc, old);
    DeleteObject(hFont);
    ReleaseDC(hWnd, hdc);

    InflateRect(&last, kGazeMarginPx, kGazeMarginPx);
    return (px >= last.left && px <= last.right && py >= last.top && py <= last.bottom);
}

// Returns true if gaze point is within the bounding box around the first character
bool isInsideFirstChar(HWND hWnd)
{
    float gx = g_gazeX.load();
    float gy = g_gazeY.load();
    if (gx < 0) return false;

    RECT rc; GetClientRect(hWnd, &rc);
    int screenW = rc.right - rc.left;
    int screenH = rc.bottom - rc.top;
    int px = (int)(gx * screenW);
    int py = (int)(gy * screenH);

    HDC hdc = GetDC(hWnd);
    std::wstring text = g_sections[g_currentSection];
    HFONT hFont = CreateFontW(32, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT old = (HFONT)SelectObject(hdc, hFont);
    SIZE szText, szFirst;
    GetTextExtentPoint32W(hdc, text.c_str(), (int)text.size(), &szText);
    GetTextExtentPoint32W(hdc, text.c_str(), 1, &szFirst);
    int x = (rc.right - szText.cx) / 2;
    int y = (rc.bottom - szText.cy) / 2;
    RECT first{ x, y, x + szFirst.cx, y + szText.cy };
    SelectObject(hdc, old);
    DeleteObject(hFont);
    ReleaseDC(hWnd, hdc);

    InflateRect(&first, kGazeMarginPx, kGazeMarginPx);
    return (px >= first.left && px <= first.right && py >= first.top && py <= first.bottom);
}

int APIENTRY _tWinMain(HINSTANCE hInst, HINSTANCE, LPTSTR, int)
{
    const TCHAR CLASS_NAME[] = _T("TobiiSeqWnd");
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    RECT work{}; SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0);
    int sw = work.right - work.left;
    int sh = work.bottom - work.top;

    HWND hWnd = CreateWindowEx(0, CLASS_NAME, _T("Reading Sequence"), WS_POPUP | WS_VISIBLE,
        work.left, work.top, sw, sh, nullptr, nullptr, hInst, nullptr);
    if (!hWnd) return 0;

    // Initialize with first set
    initializeSections();

    SetTimer(hWnd, 1, 16, nullptr); // refresh timer

    // Tobii Init
    TobiiResearchEyeTrackers* trackers = nullptr;
    if (tobii_research_find_all_eyetrackers(&trackers) != TOBII_RESEARCH_STATUS_OK || trackers->count == 0) {
        MessageBox(nullptr, _T("Eye tracker not found"), _T("Error"), MB_ICONERROR);
        return 0;
    }
    TobiiResearchEyeTracker* tracker = trackers->eyetrackers[0];
    tobii_research_subscribe_to_gaze_data(tracker, gaze_callback, nullptr);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        bool firstInside = isInsideFirstChar(hWnd);
        if (firstInside)
        {
            g_seenFirst = true; // gate has been met
        }

        bool inside = isInsideLastChar(hWnd);
        auto now = std::chrono::steady_clock::now();
        if (g_seenFirst && inside) {
            if (!g_inTarget) {
                g_inTarget = true;
                g_gazeStart = now;
                // No need to invalidate here - text doesn't change
            }
        }

        // Check timer if we've started the dwell process
        if (g_inTarget) {
            auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_gazeStart);
            if (dur.count() > kDwellTimeMs) {
                // advance section
                if (g_currentSection < (int)g_sections.size() - 1) {
                    g_currentSection++;
                    InvalidateRect(hWnd, nullptr, FALSE); // force redraw on section change
                }
                else {
                    // Move to next set if available
                    if (g_currentSet < 2) { // We have 3 sets (0, 1, 2)
                        g_currentSet++;
                        initializeSections();
                        InvalidateRect(hWnd, nullptr, FALSE);
                    }
                    else {
                        // All sets completed - could restart or exit
                        MessageBox(hWnd, _T("実験完了！"), _T("完了"), MB_OK);
                        PostQuitMessage(0);
                    }
                }
                g_inTarget = false;
                g_seenFirst = false; // reset for next section
            }
        }
    }

    tobii_research_unsubscribe_from_gaze_data(tracker, gaze_callback);
    tobii_research_free_eyetrackers(trackers);
    return 0;
} 