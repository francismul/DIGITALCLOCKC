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

/**
 * @brief Sleep for a specified number of milliseconds.
 * 
 * Cross-platform sleep function.
 * 
 * @param ms Milliseconds to sleep
 */
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

/**
 * @brief Sleep until the start of the next second.
 * 
 * Aligns updates to second boundaries for precise timing.
 */
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

/**
 * @brief Get the path to the configuration file.
 * 
 * Returns the full path to the user's config file.
 * 
 * @param buffer Buffer to store the path
 * @param size Size of the buffer
 */
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
