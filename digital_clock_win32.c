#include <windows.h>
#include <time.h>
#include <stdio.h>

// Global variables
const char g_szClassName[] = "DigitalClockClass";
int timeFormat = 24; // default to 24hr
int showDate = 1;    // 1 to show date, 0 to hide

HFONT hFontTime = NULL; // global font handles
HFONT hFontDate = NULL;

// Function to format current time as a string
void GetTimeString(char *buffer, int bufferSize, int format)
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    int hour = timeinfo->tm_hour;
    if (format == 12)
    {
        if (hour == 0)
            hour = 12;
        else if (hour > 12)
            hour -= 12;
        snprintf(buffer, bufferSize, "%02d:%02d:%02d %s",
                 hour,
                 timeinfo->tm_min,
                 timeinfo->tm_sec,
                 (timeinfo->tm_hour >= 12 ? "PM" : "AM"));
    }
    else
    {
        snprintf(buffer, bufferSize, "%02d:%02d:%02d",
                 hour,
                 timeinfo->tm_min,
                 timeinfo->tm_sec);
    }
}

// Function to format current date as a string
void GetDateString(char *buffer, int bufferSize)
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    snprintf(buffer, bufferSize, "%s, %s %02d, %d",
             // Day of week
             (timeinfo->tm_wday == 0 ? "Sunday" :
              timeinfo->tm_wday == 1 ? "Monday" :
              timeinfo->tm_wday == 2 ? "Tuesday" :
              timeinfo->tm_wday == 3 ? "Wednesday" :
              timeinfo->tm_wday == 4 ? "Thursday" :
              timeinfo->tm_wday == 5 ? "Friday" : "Saturday"),
             // Month
             (timeinfo->tm_mon == 0 ? "January" :
              timeinfo->tm_mon == 1 ? "February" :
              timeinfo->tm_mon == 2 ? "March" :
              timeinfo->tm_mon == 3 ? "April" :
              timeinfo->tm_mon == 4 ? "May" :
              timeinfo->tm_mon == 5 ? "June" :
              timeinfo->tm_mon == 6 ? "July" :
              timeinfo->tm_mon == 7 ? "August" :
              timeinfo->tm_mon == 8 ? "September" :
              timeinfo->tm_mon == 9 ? "October" :
              timeinfo->tm_mon == 10 ? "November" : "December"),
             timeinfo->tm_mday,
             timeinfo->tm_year + 1900);
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static char timeStr[64];

    switch (msg)
    {
    case WM_CREATE:
        // Create fonts once
        hFontTime = CreateFont(
            60, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS, "Arial");

        hFontDate = CreateFont(
            30, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS, "Arial");

        // Start timer
        SetTimer(hwnd, 1, 1000, NULL);
        break;

    case WM_TIMER:
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Get current time
        GetTimeString(timeStr, sizeof(timeStr), timeFormat);

        RECT rect;
        GetClientRect(hwnd, &rect);

        // Draw time in upper half
        RECT timeRect = rect;
        timeRect.bottom = rect.bottom / 2;
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFontTime);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 255, 0)); // green text
        DrawText(hdc, timeStr, -1, &timeRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Draw date in lower half if enabled
        if (showDate)
        {
            char dateStr[128];
            GetDateString(dateStr, sizeof(dateStr));
            RECT dateRect = rect;
            dateRect.top = rect.bottom / 2;
            SelectObject(hdc, hFontDate);
            DrawText(hdc, dateStr, -1, &dateRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        SelectObject(hdc, hOldFont);
        EndPaint(hwnd, &ps);
    }
    break;

    case WM_KEYDOWN:
        if (wParam == 'T' || wParam == 't')
        {
            timeFormat = (timeFormat == 24) ? 12 : 24;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if (wParam == 'D' || wParam == 'd')
        {
            showDate = !showDate;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        if (hFontTime)
            DeleteObject(hFontTime);
        if (hFontDate)
            DeleteObject(hFontDate);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    // Register window class
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Window Registration Failed!", "Error!",
                   MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Create window
    hwnd = CreateWindowEx(
        0,
        g_szClassName,
        "Digital Clock - Pure C (Win32 API)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 200,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL)
    {
        char msg[256];
        sprintf(msg, "Window Creation Failed! Error: %lu", GetLastError());
        MessageBox(NULL, msg, "Error!",
                   MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Main message loop
    while (GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
