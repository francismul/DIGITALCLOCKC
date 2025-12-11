#ifndef CONFIG_H
#define CONFIG_H

/**
 * @brief Configuration structure for the digital clock application.
 */
typedef struct {
    int time_format; /**< Time format: 12 for 12-hour, 24 for 24-hour */
    int show_date;   /**< Whether to show the date: 1 to show, 0 to hide */
    // Future expansion: colors, fonts
} AppConfig;

/**
 * @brief Load configuration from the user's config file.
 * 
 * Loads settings from ~/.digitalclockc.conf. If the file is missing or invalid,
 * default values are used.
 * 
 * @param config Pointer to AppConfig structure to fill with loaded values
 */
void load_config(AppConfig* config);

/**
 * @brief Save configuration to the user's config file.
 * 
 * Saves the current configuration to ~/.digitalclockc.conf.
 * 
 * @param config Pointer to AppConfig structure containing values to save
 */
void save_config(const AppConfig* config);

#endif // CONFIG_H
