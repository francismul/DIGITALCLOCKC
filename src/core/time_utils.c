#include <time.h>
#include <stdio.h>
#include "time_utils.h"

/**
 * Format the current local time into the provided buffer.
 *
 * When format_12hr is non-zero the string is formatted as "HH:MM:SS AM/PM";
 * when format_12hr is zero the string is formatted as "HH:MM:SS" (24-hour).
 * In 12-hour mode midnight is represented as 12:MM:SS AM.
 *
 * @param buffer Destination buffer to receive the null-terminated time string.
 * @param bufferSize Size of the destination buffer in bytes.
 * @param format_12hr Non-zero to produce 12-hour time with AM/PM, zero to produce 24-hour time.
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
 * Write the current local date into the provided buffer in the format "Day, Month DD, YYYY".
 *
 * @param buffer Destination buffer to receive the formatted date string; must be non-NULL.
 * @param bufferSize Size of the destination buffer in bytes; the written output will be truncated if it does not fit.
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