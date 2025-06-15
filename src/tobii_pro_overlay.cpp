#include <windows.h>
#include <tchar.h>
#include <atomic>
#include <chrono>
#include <thread>
#include "tobii_research.h"
#include "tobii_research_streams.h"

#pragma comment(lib, "Msimg32.lib") // for AlphaBlend if needed

static std::atomic<float> g_x{0.5f};
static std::atomic<float> g_y{0.5f};

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rc; GetClientRect(hWnd, &rc);
            HBRUSH hBrushBack = CreateSolidBrush(RGB(0,0,0));
            FillRect(hdc, &rc, hBrushBack);
            DeleteObject(hBrushBack);

            int screenW = rc.right - rc.left;
            int screenH = rc.bottom - rc.top;

            float gx = g_x.load();
            float gy = g_y.load();
            int px = static_cast<int>(gx * screenW);
            int py = static_cast<int>(gy * screenH);
            int radius = 15;
            HBRUSH hDot = CreateSolidBrush(RGB(255,0,0));
            SelectObject(hdc, hDot);
            Ellipse(hdc, px - radius, py - radius, px + radius, py + radius);
            DeleteObject(hDot);

            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_TIMER:
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void gaze_callback(TobiiResearchGazeData* gaze, void*)
{
    const TobiiResearchGazePoint& lgp = gaze->left_eye.gaze_point;
    const TobiiResearchGazePoint& rgp = gaze->right_eye.gaze_point;
    float x=-1, y=-1;
    if(lgp.validity==TOBII_RESEARCH_VALIDITY_VALID)
    {
        x=lgp.position_on_display_area.x;
        y=lgp.position_on_display_area.y;
    }
    else if(rgp.validity==TOBII_RESEARCH_VALIDITY_VALID)
    {
        x=rgp.position_on_display_area.x;
        y=rgp.position_on_display_area.y;
    }
    if(x>=0 && y>=0)
    {
        g_x.store(x);
        g_y.store(y);
    }
}

int APIENTRY _tWinMain(HINSTANCE hInst, HINSTANCE, LPTSTR, int)
{
    const TCHAR CLASS_NAME[] = _T("TobiiOverlayWnd");
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_CROSS);
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    HWND hWnd = CreateWindowEx(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOPMOST, CLASS_NAME, _T("Tobii Gaze Overlay"), WS_POPUP,
                                0,0, sw, sh, nullptr, nullptr, hInst, nullptr);
    if(!hWnd) return 0;

    // Make window click-through & transparent (black colorkey)
    SetLayeredWindowAttributes(hWnd, RGB(0,0,0), 255, LWA_COLORKEY);

    // 60 fps timer for continuous repaint
    SetTimer(hWnd, 1, 16, nullptr);

    ShowWindow(hWnd, SW_SHOW);

    // Init Tobii
    TobiiResearchEyeTrackers* trackers=nullptr;
    if(tobii_research_find_all_eyetrackers(&trackers)!=TOBII_RESEARCH_STATUS_OK || trackers->count==0)
    {
        MessageBox(nullptr, _T("Eye tracker not found"), _T("Error"), MB_ICONERROR);
        return 0;
    }
    TobiiResearchEyeTracker* tracker = trackers->eyetrackers[0];
    tobii_research_subscribe_to_gaze_data(tracker, gaze_callback, nullptr);

    // main loop
    MSG msg;
    while(GetMessage(&msg, nullptr, 0,0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    tobii_research_unsubscribe_from_gaze_data(tracker, gaze_callback);
    tobii_research_free_eyetrackers(trackers);
    return 0;
} 