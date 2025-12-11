#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <time.h>

/**
 * @brief Get the current time as a formatted string.
 * 
 * Formats the current system time into a string buffer.
 * 
 * @param buffer Buffer to store the formatted time string
 * @param bufferSize Size of the buffer
 * @param format_12hr 1 for 12-hour format, 0 for 24-hour format
 */
void get_time_string(char *buffer, int bufferSize, int format_12hr);

/**
 * @brief Get the current date as a formatted string.
 * 
 * Formats the current system date into a string buffer.
 * 
 * @param buffer Buffer to store the formatted date string
 * @param bufferSize Size of the buffer
 */
void get_date_string(char *buffer, int bufferSize);

#endif // TIME_UTILS_H
