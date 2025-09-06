Testing library and framework: vendored minimal "acutest"-style single-header (test/lib/acutest.h).

Build example with MinGW (Windows):
  gcc -D_CRT_SECURE_NO_WARNINGS -Wall -Itest -o digital_clock_tests.exe test/test_digital_clock_win32_tests.c -lgdi32 -luser32

Build example with clang on non-Windows (for static analysis only; Win32 parts are stubbed):
  clang -Wall -Itest -DTEST_STUBS -o digital_clock_tests test/test_digital_clock_win32_tests.c

Focus areas include:
- Deterministic formatting in `GetTimeString` for both 12-hour and 24-hour modes, including adjustments for midnight and afternoon
- Deterministic mapping of weekdays and months in `GetDateString`
- WndProc WM_KEYDOWN toggling of global flags (timeFormat, showDate)

Notes:
- We override time() and localtime() within the test TU to keep outputs deterministic.
- WM_CREATE/WM_PAINT/WM_TIMER paths, GDI handles, and fonts are not exercised here.