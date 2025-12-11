// A simple Digital Clock That runs on the terminal

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

// Trying to make the application cros-platform (window & linux)
#ifdef _WIN32
#include <windows.h> 
#define CLEAR "cls"  // Command to clear the screen in Windows
#else
#define CLEAR "clear" // Command to clear the screen in Linux/Unix
#endif

/**
 * @brief Print the current time to the console.
 * 
 * Formats and displays the current time in 12 or 24 hour format.
 * 
 * @param format Time format: 12 for 12-hour, 24 for 24-hour
 */
void printTime(int format)
{
    time_t rawtime;
    struct tm *timeinfo;

    // Getting the Current System Time
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Determining the hour based on the selected format
    int hour = timeinfo->tm_hour;
    if (format == 12)
    {
        if (hour == 0)
        {
            hour = 12; // Incase of midnight (12 AM)
        }
        else if (hour > 12)
        {
            hour -= 12; // Convert to 12-hour format
        }
    }

    // Printing the current time in the selected format
    printf("%02d : %02d : %02d %s\n",
           hour,
           timeinfo->tm_min,
           timeinfo->tm_sec,
           (format == 12) ? (timeinfo->tm_hour >= 12 ? "PM" : "AM") : "");
}

// Function to clear the screen
/**
 * @brief Clear the terminal screen.
 * 
 * Uses system command to clear the console display.
 */
void clearScreen()
{
    system(CLEAR);
}

// Cross-platform sleep function
// in windows sleep takes in milliseconds while linux sleep takes in seconds
/**
 * @brief Sleep for a specified number of seconds.
 * 
 * Cross-platform sleep function that works on both Windows and Linux.
 * 
 * @param seconds Number of seconds to sleep
 */
void performSleep(int seconds)
{
#ifdef _WIN32
    Sleep(seconds * 1000);
#else
    sleep(seconds); 
#endif
}

// Prompting user on what clock format he/she prefers (12hr, 24hr)
/**
 * @brief Get the user's preferred time format.
 * 
 * Prompts the user to choose between 12-hour and 24-hour time format.
 * 
 * @return 12 for 12-hour format, 24 for 24-hour format
 */
int getTimeFormat()
{
    int format = 0;
    while (format != 1 && format != 2)
    {
        printf("Select time format:\n1. 12-hour\n2. 24-hour\nEnter 1 or 2: ");
        scanf("%d", &format);
        if (format != 1 && format != 2)
        {
            printf("Invalid input. Please enter 1 or 2.\n");
        }
    }
    return (format == 1) ? 12 : 24;
}

// Main function
/**
 * @brief Main entry point for the terminal clock application.
 * 
 * Runs an infinite loop displaying the current time.
 * 
 * @return Exit status (never reached)
 */
int main()
{
    int format = getTimeFormat(); // Getting user preferred time format

    while (1)
    {
        clearScreen(); // Clear the screen
        printTime(format); // print time
        performSleep(1); // perform one sec sleep
    }

    return 0;
}
