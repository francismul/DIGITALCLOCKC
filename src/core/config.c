#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../vendor/ini.h"
#include "config.h"

#define CONFIG_FILENAME ".digitalclockc.conf"

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
