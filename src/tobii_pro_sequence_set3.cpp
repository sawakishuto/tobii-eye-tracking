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

// セットC：入札不正問題への対応
static const std::vector<std::wstring> g_sections = {
    L"2025年3月20日、当社の営業担当Hさんが、地方自治体の入札で、ライバル会社の\n見積もり金額を事前に入手し、社内チャットで共有するという問題が起きました。\nそして、その情報をもとにライバル社より1%だけ安い金額で見積もりを提出して\nいたことが発覚しました。これは法律や会社のルールに違反する重大な不正行為\nです。",
    L"この問題は、入札先である自治体の職員が、当社から提出された見積もりファイル\nのデータ情報（プロパティ）に、ライバル会社A社の名前が含まれているのを見つけた\nことがきっかけで発覚しました。社内でHさんに事情を聞いたところ、別の会社に\n勤める友人から公開前の見積もり情報をもらい、それを部署のチャットに投稿して、\n金額を決める参考にしたことを認めました。さらに、上司であるI課長もチャットを\n見ていながら「競争が厳しいから仕方ない」と不正を止めなかった事実も確認され、\n組織全体の管理に問題があったことが明らかになりました。",
    L"直接の原因は、営業担当Hさんの「ルールを守る」という意識が低かったことです。\nしかしその背景には、部署全体で不正を防ぐための教育がきちんと行われていな\nかったことや、成果ばかりを重視する評価制度がプレッシャーとなり、不正行為に\nつながったことがあります。この問題によって、国から罰金を科されたり、今後の\n入札に参加できなくなったりする危険性があり、会社の信用が大きく下がる恐れも\nあります。",
    L"緊急の対応として、問題となった入札はすぐに辞退し、自治体へ謝罪文を送りま\nした。社内では、Hさんを自宅待機処分とし、関わっていたI課長も管理職から外し、\n第三者委員会による調査を受けさせています。二度とこのような問題を起こさない\nため、全社員に法律に関するeラーニングを義務付け、評価項目に「ルールを守って\nいるか」という視点を加えます。また、会社の機密情報が不正にやり取りされて\nいないか監視するシステムを導入し、定期的にチェックする体制を整えました。"
};

// https://docs.google.com/forms/d/e/1FAIpQLSd2zlpgSSqp8xYSSZYzjhofR-wfqqFIwUzlQcHKTa-MFpEe0g/viewform

static int g_currentSection = 0;

// ========================================
// 参加者番号設定（実験開始前にここを変更してください）
// ========================================
static int g_participantNumber = 1; // 参加者番号（1-8に設定）

constexpr int kGazeMarginPx = 20; // tolerance around last character - reduced for stricter detection
constexpr int kDwellTimeMs = 1000; // gaze dwell time before advancing (in milliseconds)

static std::chrono::steady_clock::time_point g_gazeStart;
static bool g_inTarget = false;
static bool g_showGaze = true; // toggle visualization - enabled by default for debugging
static bool g_seenFirst = false; // whether the first char has been gazed at in current section

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

        // セクション情報は非表示（画面をクリーンに保つため）

        SelectObject(hdc, old);
        DeleteObject(hFont);

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

    HWND hWnd = CreateWindowEx(0, CLASS_NAME, _T("入札不正問題への対応"), WS_POPUP | WS_VISIBLE,
        work.left, work.top, sw, sh, nullptr, nullptr, hInst, nullptr);
    if (!hWnd) return 0;

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
                    // All sections completed
                    MessageBox(hWnd, _T("入札不正問題への対応完了！"), _T("完了"), MB_OK);
                    PostQuitMessage(0);
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