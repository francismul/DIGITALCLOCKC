#include "plugin_registry.h"
#include <stddef.h>

#define MAX_PLUGINS 10

static Plugin* plugins[MAX_PLUGINS];
static int plugin_count = 0;

/**
 * @brief Initialize the plugin registry.
 * 
 * Must be called before registering any plugins.
 */
void plugin_registry_init(void) {
    plugin_count = 0;
}

/**
 * @brief Register a plugin with the registry.
 * 
 * Adds a plugin to the registry and calls its init function if available.
 * 
 * @param plugin Pointer to the plugin to register
 */
void plugin_register(Plugin* plugin) {
    if (plugin_count < MAX_PLUGINS) {
        plugins[plugin_count++] = plugin;
        if (plugin->init) {
            plugin->init();
        }
    }
}

/**
 * @brief Update all registered plugins.
 * 
 * Calls the update function for each registered plugin.
 */
void plugin_registry_update_all(void) {
    for (int i = 0; i < plugin_count; i++) {
        if (plugins[i]->update) {
            plugins[i]->update();
        }
    }
}

/**
 * @brief Draw all registered plugins.
 * 
 * Calls the draw function for each registered plugin.
 * 
 * @param ctx Nuklear context for rendering
 */
void plugin_registry_draw_all(struct nk_context* ctx) {
    for (int i = 0; i < plugin_count; i++) {
        if (plugins[i]->draw) {
            plugins[i]->draw(ctx);
        }
    }
}
