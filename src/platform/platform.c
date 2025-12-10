#define _POSIX_C_SOURCE 199309L // For nanosleep
#include "platform.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
#else
    #include <unistd.h>
    #include <sys/time.h>
#endif

void platform_sleep_ms(int ms) {
    if (ms < 0) ms = 0;
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

void platform_sleep_until_next_second(void) {
#ifdef _WIN32
    SYSTEMTIME st;
    GetSystemTime(&st);
    int ms_to_wait = 1000 - st.wMilliseconds;
    if (ms_to_wait > 0)
        Sleep(ms_to_wait);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int ms_to_wait = 1000 - (tv.tv_usec / 1000);
    if (ms_to_wait > 0) {
        struct timespec ts;
        ts.tv_sec = ms_to_wait / 1000;
        ts.tv_nsec = (ms_to_wait % 1000) * 1000000L;
        nanosleep(&ts, NULL);
    }
#endif
}

void platform_get_config_path(char *buffer, int size) {
#ifdef _WIN32
    // On Windows, maybe use APPDATA or just local user profile
    // For simplicity matching the linux path style or use %USERPROFILE%/.digitalclockc.conf
    const char* user_profile = getenv("USERPROFILE");
    if (!user_profile) user_profile = "C:\\";
    snprintf(buffer, size, "%s\\.digitalclockc.conf", user_profile);
#else
    const char* home = getenv("HOME");
    if (!home) home = ".";
    snprintf(buffer, size, "%s/.digitalclockc.conf", home);
#endif
}
