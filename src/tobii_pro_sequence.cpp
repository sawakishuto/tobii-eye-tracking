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

// 4 sections of text
static const std::vector<std::wstring> g_sections = {
    L"古びた図書館で、ユキは一冊の詩集に手を伸ばした。その瞬間、同じ本に触れた手があった。そこにいたのは、少し年上の青年・リョウ。偶然が二人を引き合わせた。",
    L"ユキとリョウは、週末ごとに図書館で会い、本について語り合った。やがて、散歩や喫茶店にも行くようになり、彼女の胸には淡い想いが芽生え始めた。",
    L"リョウは海外留学を決意し、ユキに告げた。「また戻ってくるよ」と笑う彼に、ユキは言葉が見つからなかった。ただ「行ってらっしゃい」と微笑んだ。",
    L"1年後の冬、図書館の同じ詩集に、またふたつの手が重なった。そこには、少し大人びたリョウがいた。「ただいま」と言う声に、ユキの瞳が潤んだ。"
};
static int g_currentSection = 0;

constexpr int kGazeMarginPx = 40; // tolerance around last character

static std::chrono::steady_clock::time_point g_gazeStart;
static bool g_inTarget = false;

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

        // compute last char rect for debug if needed
        SIZE szLast;
        GetTextExtentPoint32W(hdc, text.c_str() + text.size() - 1, 1, &szLast);
        RECT lastRect{ x + sz.cx - szLast.cx, y, x + sz.cx, y + sz.cy };
        FrameRect(hdc, &lastRect, (HBRUSH)GetStockObject(GRAY_BRUSH)); // visualize target

        // Draw gaze point if valid
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

        SelectObject(hdc, old);
        DeleteObject(hFont);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_TIMER:
        InvalidateRect(hWnd, nullptr, FALSE);
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

        bool inside = isInsideLastChar(hWnd);
        auto now = std::chrono::steady_clock::now();
        if (inside) {
            if (!g_inTarget) {
                g_inTarget = true;
                g_gazeStart = now;
            }
            else {
                auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_gazeStart);
                if (dur.count() > 500) {
                    // advance section
                    if (g_currentSection < (int)g_sections.size() - 1) {
                        g_currentSection++;
                    }
                    g_inTarget = false;
                }
            }
        }
        else {
            g_inTarget = false;
        }
    }

    tobii_research_unsubscribe_from_gaze_data(tracker, gaze_callback);
    tobii_research_free_eyetrackers(trackers);
    return 0;
} 