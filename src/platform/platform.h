#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>

/**
 * @brief Sleep until the start of the next second.
 * 
 * Aligns updates to second boundaries for precise timing.
 */
void platform_sleep_until_next_second(void);

/**
 * @brief Sleep for a specified number of milliseconds.
 * 
 * Cross-platform sleep function.
 * 
 * @param ms Milliseconds to sleep
 */
void platform_sleep_ms(int ms);

/**
 * @brief Get the path to the configuration file.
 * 
 * Returns the full path to the user's config file.
 * 
 * @param buffer Buffer to store the path
 * @param size Size of the buffer
 */
void platform_get_config_path(char *buffer, int size);

#endif // PLATFORM_H
