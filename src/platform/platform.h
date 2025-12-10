#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>

// Sleep until the start of the next second to align updates.
void platform_sleep_until_next_second(void);

// General sleep in milliseconds
void platform_sleep_ms(int ms);

// Get path to config file
void platform_get_config_path(char *buffer, int size);

#endif // PLATFORM_H
