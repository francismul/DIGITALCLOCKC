#include <time.h>
#include <stdio.h>
#include "time_utils.h"

/**
 * @brief Get the current time as a formatted string.
 * 
 * Formats the current system time into a string buffer.
 * 
 * @param buffer Buffer to store the formatted time string
 * @param bufferSize Size of the buffer
 * @param format_12hr 1 for 12-hour format, 0 for 24-hour format
 */
void get_time_string(char *buffer, int bufferSize, int format_12hr)
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    int hour = timeinfo->tm_hour;
    if (format_12hr)
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

/**
 * @brief Get the current date as a formatted string.
 * 
 * Formats the current system date into a string buffer.
 * 
 * @param buffer Buffer to store the formatted date string
 * @param bufferSize Size of the buffer
 */
void get_date_string(char *buffer, int bufferSize)
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    const char *days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", 
                         "Thursday", "Friday", "Saturday"};
    const char *months[] = {"January", "February", "March", "April", "May", "June",
                           "July", "August", "September", "October", "November", "December"};

    snprintf(buffer, bufferSize, "%s, %s %02d, %d",
             days[timeinfo->tm_wday],
             months[timeinfo->tm_mon],
             timeinfo->tm_mday,
             timeinfo->tm_year + 1900);
}
