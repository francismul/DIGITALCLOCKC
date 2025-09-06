// Tests for test_digital_clock_win32.c
// Framework: minimal acutest-style (vendored)
// Focus: GetTimeString, GetDateString formatting and WndProc WM_KEYDOWN toggling

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
// Provide minimal Win32 constants/types for non-Windows static analysis/linting in CI
typedef void* HWND;
typedef unsigned int UINT;
typedef unsigned long WPARAM;
typedef long LPARAM;
typedef long LRESULT;
#define CALLBACK
#define WM_CREATE   0x0001
#define WM_TIMER    0x0113
#define WM_PAINT    0x000F
#define WM_KEYDOWN  0x0100
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "lib/acutest.h"

// We want to stub time() and localtime() used by the code under test to make deterministic.
// We accomplish this by providing our own symbols and using macros so calls resolve to our stubs.
static time_t g_fake_time = 0;
static struct tm g_fake_tm;

static time_t stub_time(time_t* tloc) {
    if (tloc) *tloc = g_fake_time;
    return g_fake_time;
}

static struct tm* stub_localtime(const time_t* timer) {
    (void)timer;
    return &g_fake_tm;
}

// Redefine time/localtime seen by the unit under test
#define time     stub_time
#define localtime stub_localtime

// Pull in declarations of the functions under test by declaring the same signatures.
// Then, include the source file so macros apply. This is a common C unit-testing tactic for pure fns.
void GetTimeString(char *buffer, int bufferSize, int format);
void GetDateString(char *buffer, int bufferSize);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Include the code under test directly so that our macro overrides apply.
#include "test/test_digital_clock_win32.c"

// Helper to set a deterministic fake clock
static void set_fake_datetime(int year, int mon0, int mday, int wday0, int hour24, int min, int sec) {
    memset(&g_fake_tm, 0, sizeof(g_fake_tm));
    g_fake_tm.tm_year = year - 1900;
    g_fake_tm.tm_mon  = mon0;      // 0-based
    g_fake_tm.tm_mday = mday;
    g_fake_tm.tm_wday = wday0;     // 0=Sun
    g_fake_tm.tm_hour = hour24;
    g_fake_tm.tm_min  = min;
    g_fake_tm.tm_sec  = sec;
    // g_fake_time is not used by stub_localtime; keep zero.
}

// ---------- Tests for GetTimeString ----------

TEST_CASE(get_time_string_24h_basic_format) {
    set_fake_datetime(2025, 8, 6, 6, 9, 7, 5); // Sep 6, 2025, Sat (6), 09:07:05
    char buf[64];
    GetTimeString(buf, sizeof(buf), 24);
    // Expect HH:MM:SS with two digits each, no AM/PM
    TEST_CHECK(strlen(buf) == 8);
    TEST_CHECK(buf[2] == ':' && buf[5] == ':');
    // Digits check
    for (int i = 0; i < 8; ++i) {
        if (i == 2 || i == 5) continue;
        TEST_CHECK(buf[i] >= '0' && buf[i] <= '9');
    }
}

TEST_CASE(get_time_string_12h_am_midnight_edge) {
    set_fake_datetime(2025, 0, 1, 4, 0, 0, 0); // Jan 1, 2025, Thu (4), 00:00:00
    char buf[64];
    GetTimeString(buf, sizeof(buf), 12);
    // Expect "12:00:00 AM"
    TEST_CHECK(strcmp(buf, "12:00:00 AM") == 0);
}

TEST_CASE(get_time_string_12h_pm_afternoon_adjust) {
    set_fake_datetime(2025, 8, 6, 6, 15, 30, 45); // 15:30:45 -> 03:30:45 PM
    char buf[64];
    GetTimeString(buf, sizeof(buf), 12);
    TEST_CHECK(strstr(buf, "PM") != NULL);
    TEST_CHECK(buf[0] == '0' && buf[1] == '3'); // 03:...
    TEST_CHECK(buf[2] == ':' && buf[5] == ':');
}

// ---------- Tests for GetDateString ----------

TEST_CASE(get_date_string_full_names_correct_mapping) {
    // Saturday, September 06, 2025
    set_fake_datetime(2025, 8, 6, 6, 10, 11, 12);
    char buf[128];
    GetDateString(buf, sizeof(buf));
    TEST_CHECK(strstr(buf, "Saturday, September 06, 2025") != NULL);
}

TEST_CASE(get_date_string_month_and_weekday_bounds) {
    // Test boundary months and weekdays mapping
    struct { int mon; const char* month; int wday; const char* day; int mday; int year; } cases[] = {
        {0,  "January",   0, "Sunday",    1, 2023},
        {1,  "February",  1, "Monday",   28, 2024},
        {2,  "March",     2, "Tuesday",  15, 2022},
        {3,  "April",     3, "Wednesday",30, 2021},
        {4,  "May",       4, "Thursday", 31, 2020},
        {5,  "June",      5, "Friday",   12, 2019},
        {6,  "July",      6, "Saturday",  4, 2018},
        {7,  "August",    0, "Sunday",   19, 2017},
        {8,  "September", 1, "Monday",   25, 2016},
        {9,  "October",   2, "Tuesday",  31, 2015},
        {10,"November",   3, "Wednesday", 1, 2014},
        {11,"December",   4, "Thursday", 25, 2013},
    };
    char buf[128];
    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i) {
        set_fake_datetime(cases[i].year, cases[i].mon, cases[i].mday, cases[i].wday, 1, 2, 3);
        GetDateString(buf, sizeof(buf));
        char expected[128];
        snprintf(expected, sizeof(expected), "%s, %s %02d, %d",
                 cases[i].day, cases[i].month, cases[i].mday, cases[i].year);
        TEST_CHECK_MSG(strcmp(buf, expected) == 0, "Date string mapping mismatch");
    }
}

// ---------- Tests for WndProc key handling (toggle globals) ----------

TEST_CASE(wndproc_toggle_time_format_on_T_key) {
    // Ensure initial global state as in the file: timeFormat = 24
    timeFormat = 24;
    // Press 'T'
    WndProc(NULL, WM_KEYDOWN, (WPARAM)'T', 0);
    TEST_CHECK(timeFormat == 12);
    // Press 't' -> back to 24
    WndProc(NULL, WM_KEYDOWN, (WPARAM)'t', 0);
    TEST_CHECK(timeFormat == 24);
}

TEST_CASE(wndproc_toggle_show_date_on_D_key) {
    // Initial: showDate = 1
    showDate = 1;
    // Press 'D'
    WndProc(NULL, WM_KEYDOWN, (WPARAM)'D', 0);
    TEST_CHECK(showDate == 0);
    // Press 'd' -> back to 1
    WndProc(NULL, WM_KEYDOWN, (WPARAM)'d', 0);
    TEST_CHECK(showDate == 1);
}

// ---------- Test list and main ----------

TEST_LIST_BEGIN
    TEST_LIST_ENTRY(get_time_string_24h_basic_format)
    TEST_LIST_ENTRY(get_time_string_12h_am_midnight_edge)
    TEST_LIST_ENTRY(get_time_string_12h_pm_afternoon_adjust)
    TEST_LIST_ENTRY(get_date_string_full_names_correct_mapping)
    TEST_LIST_ENTRY(get_date_string_month_and_weekday_bounds)
    TEST_LIST_ENTRY(wndproc_toggle_time_format_on_T_key)
    TEST_LIST_ENTRY(wndproc_toggle_show_date_on_D_key)
TEST_LIST_END

int main(void) {
    return test_run_all(tests);
}