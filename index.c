#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

// Define a macro to check the operating system
#ifdef _WIN32
#include <windows.h> // Include Windows-specific header
#define CLEAR "cls"  // Command to clear the screen in Windows
#else
#define CLEAR "clear" // Command to clear the screen in Linux/Unix
#endif

// Function to print the current time
void printTime(int format)
{
    time_t rawtime;
    struct tm *timeinfo;

    // Get the current system time
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Determine the hour based on the selected format
    int hour = timeinfo->tm_hour;
    if (format == 12)
    {
        if (hour == 0)
        {
            hour = 12; // Midnight case (12 AM)
        }
        else if (hour > 12)
        {
            hour -= 12; // Convert to 12-hour format
        }
    }

    // Print the current time in the selected format
    printf("%02d : %02d : %02d %s\n",
           hour,
           timeinfo->tm_min,
           timeinfo->tm_sec,
           (format == 12) ? (timeinfo->tm_hour >= 12 ? "PM" : "AM") : "");
}

// Function to clear the screen
void clearScreen()
{
    system(CLEAR);
}

// Cross-platform sleep function
void performSleep(int seconds)
{
#ifdef _WIN32
    Sleep(seconds * 1000); // Sleep takes milliseconds in Windows
#else
    sleep(seconds); // Sleep takes seconds in Linux/Unix
#endif
}

// Function to get time format from the user
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
int main()
{
    int format = getTimeFormat(); // Get user preference for time format

    while (1)
    {
        clearScreen(); // Clear the screen

        // Print the current time
        printTime(format);

        // Perform sleep for 1 second
        performSleep(1);
    }

    return 0;
}
