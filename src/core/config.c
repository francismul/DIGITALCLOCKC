#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../vendor/ini.h"
#include "config.h"

#define CONFIG_FILENAME ".digitalclockc.conf"

/**
 * Parse INI handler that maps "Settings" keys to fields in an AppConfig.
 *
 * @param user Pointer to an AppConfig instance to populate.
 * @param section INI section name.
 * @param name INI key name.
 * @param value INI key value as a string.
 * @returns `1` if a known setting was parsed and applied to `user`, `0` if the section/name is unrecognized.
 */
static int handler(void* user, const char* section, const char* name, const char* value)
{
    AppConfig* config = (AppConfig*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    if (MATCH("Settings", "time_format")) {
        config->time_format = atoi(value);
    } else if (MATCH("Settings", "show_date")) {
        config->show_date = atoi(value);
    } else {
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

#include "../platform/platform.h"

/**
 * Load application configuration from the platform-specific INI file into `config`.
 *
 * Initializes `config` with sensible defaults and then attempts to parse the INI
 * file located at the platform-specific configuration path. If the file cannot
 * be read or parsed, `config` remains set to the defaults.
 *
 * @param config Pointer to an AppConfig structure to populate. On return,
 *        `config->time_format` and `config->show_date` will be set (defaults:
 *        `time_format = 24`, `show_date = 1`), or overwritten by values read
 *        from the INI file if present.
 */
void load_config(AppConfig* config)
{
    // Defaults
    config->time_format = 24;
    config->show_date = 1;

    char path[1024];
    platform_get_config_path(path, sizeof(path));

    if (ini_parse(path, handler, config) < 0) {
        // Can't load, use defaults.
        // printf("Can't load '%s', using defaults\n", path);
    }
}

/**
 * Write application configuration to the platform-specific config file in INI format.
 *
 * This function serializes the provided AppConfig into a file determined by
 * platform_get_config_path(). It writes a `[Settings]` section containing
 * `time_format` and `show_date`, overwriting any existing file at that path.
 *
 * If the config file cannot be opened for writing, the function returns
 * without creating or modifying a file.
 *
 * @param config Pointer to the configuration values to persist.
 */
void save_config(const AppConfig* config)
{
    char path[1024];
    platform_get_config_path(path, sizeof(path));

    FILE* file = fopen(path, "w");
    if (!file) return;

    fprintf(file, "[Settings]\n");
    fprintf(file, "time_format=%d\n", config->time_format);
    fprintf(file, "show_date=%d\n", config->show_date);
    
    fclose(file);
}