#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int time_format; // 12 or 24
    int show_date;   // 0 or 1
    // Future expansion: colors, fonts
} AppConfig;

// Load config from ~/.digitalclockc.conf. Returns default if file missing.
void load_config(AppConfig* config);

// Save config to ~/.digitalclockc.conf
void save_config(const AppConfig* config);

#endif // CONFIG_H
