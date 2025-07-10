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

// 電気設備更新工事に関する通知
static const std::vector<std::wstring> g_set1 = {
    L"二〇二五年九月十日（火）午前零時から同日二十四時まで、\n本社ビル全十五基のエレベーターを制御盤交換工事のため終日停止いたします。",
    L"この工事により、人の上下移動は非常階段に限定されるため、総務部は九月九日（月）十八時までに\n階段照明の点灯確認と避難誘導サインの追加設置を完了してください。",
    L"各部署は九月九日（月）業務終了時までに大型機材や大量資料のフロア間移動を済ませ、\n当日搬出入を伴う会議や来客対応を極力回避するよう予定を調整してください。",
    L"身体に不自由のある社員や来訪者向けに、一階ロビーに臨時ワークスペース（Wi-Fi・電源完備）を用意しますので、\n必要な方は人事部まで利用申請をお願いします。",
    L"停電を伴わない工事ですが、エレベーター内監視カメラおよび管制室の監視装置は一時的にオフラインとなるため、\n施設警備課は人員を増強し、監視強化を実施します。",
    L"工事完了後、ビル管理会社が稼働試験を行い、九月十一日（水）午前六時までに全基の安全確認が取れ次第、\n社内メールで運行再開をお知らせします。",
    L"以上、エレベーター制御盤交換という単一の原因に起因する措置となりますので、\n安全確保と業務継続のため、皆さまのご理解とご協力をお願いいたします。"
};

// セット2：配管工事（実験用）
static const std::vector<std::wstring> g_set2 = {
    L"二〇二五年十月五日（日）午前五時から同日十八時まで、\n研究棟全域において老朽配管更新工事のため計画断水を実施いたします。",
    L"これにより上水・温水・実験用純水のすべてが停止するため、ラボ責任者は十月四日（土）二十時までに\n水を要する実験を終了し、培養装置や循環冷却水系を安全停止してください。",
    L"メンテナンス中は消火栓も使用不可となるため、安全衛生委員会は追加の消火器を各フロア非常口脇に設置し、\n設置完了を十月四日（土）二十二時までに報告書で提出してください。",
    L"給湯室・化粧室の水道も停止するため、総務部は隣接ビル一階の仮設トイレと手洗いステーションの設置を手配し、\n利用案内を掲示してください。",
    L"動物飼育室の自動給水ラインは別系統で稼働しますが、念のため飼育担当者は工事開始前に手動で給水量を確認し、\n異常があれば直ちに施設管理課へ連絡してください。",
    L"工事完了後の十月五日（日）十九時以降、施設管理課が残留塩素濃度と水圧を測定し、\n安全が確認されしだい一斉メールで断水解除を通知します。",
    L"以上、老朽配管更新という単一の要因に起因する措置となりますので、\n研究データ保全と安全確保のため、関係各位のご協力を強くお願い申し上げます。"
};

// セット3：駐車場工事（実験用）
static const std::vector<std::wstring> g_set3 = {
    L"二〇二五年十一月十四日（金）午前五時から同日十五時まで、\n本社ビル駐車場をアスファルト舗装改修工事のため全面閉鎖いたします。",
    L"閉鎖中は車両の入退場が禁止されるため、警備課は十一月十三日（木）十八時までに\n交通コーンと案内看板を設置し、迂回ルートを明示します。",
    L"車で通勤する社員は当日「イーストタワーガレージ」を利用し、\n利用料金（一人一日一五〇〇円を上限）を領収書とともに総務部へ申請してください。",
    L"当日納品予定の配送トラックは日程を変更するか、\n工事終了後の十六時以降に南門荷さばきエリアを利用してください。",
    L"工事による振動のため、隣接する自転車置き場も閉鎖し、代替として正面玄関付近に臨時ラックを設置しますので、\n自転車通勤者はそちらをお使いください。",
    L"騒音と舗装材の臭気を考慮し、人事部は希望部署にテレワークを許可します。\n各マネージャーは十一月十二日（水）正午までにテレワーク予定者一覧を提出してください。",
    L"舗装後の冷却とライン引きが完了し次第、十一月十五日（土）午前六時に駐車場を再開し、\n施設管理課が社内メールで通知します。"
};

// 全セットを結合
static std::vector<std::wstring> g_sections;
static int g_currentSet = 0; // 現在のセット番号 (0, 1, 2)
static int g_currentSection = 0;

constexpr int kGazeMarginPx = 20; // tolerance around last character - reduced for stricter detection
constexpr int kDwellTimeMs = 500; //5 gaze dwell time before advancing (in milliseconds)

static std::chrono::steady_clock::time_point g_gazeStart;
static bool g_inTarget = false;
static bool g_showGaze = true; // toggle visualization - enabled by default for debugging
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

        // Split text by newlines and draw multiple lines
        std::vector<std::wstring> lines;
        std::wstring currentLine;
        for (wchar_t c : text) {
            if (c == L'\n') {
                lines.push_back(currentLine);
                currentLine.clear();
            } else {
                currentLine += c;
            }
        }
        if (!currentLine.empty()) {
            lines.push_back(currentLine);
        }

        // Calculate total height and draw centered
        int lineHeight = 80; // Line spacing - increased further for better readability
        int totalHeight = (int)lines.size() * lineHeight;
        int startY = (rc.bottom - totalHeight) / 2;
        
        for (size_t i = 0; i < lines.size(); ++i) {
            SIZE sz;
            GetTextExtentPoint32W(hdc, lines[i].c_str(), (int)lines[i].size(), &sz);
            int x = (rc.right - sz.cx) / 2;
            int y = startY + (int)i * lineHeight;
            TextOutW(hdc, x, y, lines[i].c_str(), (int)lines[i].size());
        }

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
        if (!lines.empty()) {
            std::wstring lastLine = lines.back();
            if (!lastLine.empty()) {
                SIZE szLastLine, szLastChar;
                GetTextExtentPoint32W(hdc, lastLine.c_str(), (int)lastLine.size(), &szLastLine);
                GetTextExtentPoint32W(hdc, lastLine.c_str() + lastLine.size() - 1, 1, &szLastChar);
                int lastX = (rc.right - szLastLine.cx) / 2;
                int lastY = startY + ((int)lines.size() - 1) * lineHeight;
                RECT lastRect{ lastX + szLastLine.cx - szLastChar.cx, lastY, lastX + szLastLine.cx, lastY + 32 };
                
                // Target area calculation (no longer visualized)
            }
        }

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
    
    // Split text by newlines
    std::vector<std::wstring> lines;
    std::wstring currentLine;
    for (wchar_t c : text) {
        if (c == L'\n') {
            lines.push_back(currentLine);
            currentLine.clear();
        } else {
            currentLine += c;
        }
    }
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    RECT last{};
    if (!lines.empty()) {
        std::wstring lastLine = lines.back();
        if (!lastLine.empty()) {
            SIZE szLastLine, szLastChar;
            GetTextExtentPoint32W(hdc, lastLine.c_str(), (int)lastLine.size(), &szLastLine);
            GetTextExtentPoint32W(hdc, lastLine.c_str() + lastLine.size() - 1, 1, &szLastChar);
            
            int lineHeight = 80;
            int totalHeight = (int)lines.size() * lineHeight;
            int startY = (rc.bottom - totalHeight) / 2;
            int lastX = (rc.right - szLastLine.cx) / 2;
            int lastY = startY + ((int)lines.size() - 1) * lineHeight;
            
            // Create a more generous bounding box for the last character
            last = { lastX + szLastLine.cx - szLastChar.cx - 10, lastY - 5, 
                    lastX + szLastLine.cx + 10, lastY + 37 };
        }
    }
    
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
    
    // Split text by newlines
    std::vector<std::wstring> lines;
    std::wstring currentLine;
    for (wchar_t c : text) {
        if (c == L'\n') {
            lines.push_back(currentLine);
            currentLine.clear();
        } else {
            currentLine += c;
        }
    }
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    RECT first{};
    if (!lines.empty()) {
        std::wstring firstLine = lines[0];
        SIZE szFirstLine, szFirstChar;
        GetTextExtentPoint32W(hdc, firstLine.c_str(), (int)firstLine.size(), &szFirstLine);
        GetTextExtentPoint32W(hdc, firstLine.c_str(), 1, &szFirstChar);
        
        int lineHeight = 80;
        int totalHeight = (int)lines.size() * lineHeight;
        int startY = (rc.bottom - totalHeight) / 2;
        int firstX = (rc.right - szFirstLine.cx) / 2;
        first = { firstX, startY, firstX + szFirstChar.cx, startY + 32 };
    }
    
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