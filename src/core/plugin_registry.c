#include "plugin_registry.h"
#include <stddef.h>

#define MAX_PLUGINS 10

static Plugin* plugins[MAX_PLUGINS];
static int plugin_count = 0;

/**
 * Reset the plugin registry to an empty state.
 *
 * Sets the registered plugin count to zero. Previously stored plugin pointers
 * in the internal array are not modified. */
void plugin_registry_init(void) {
    plugin_count = 0;
}

/**
 * Register a plugin with the global registry and invoke its initializer if present.
 *
 * Adds the given plugin to the registry when there is available capacity. If the plugin
 * is successfully added and its `init` pointer is non-NULL, the `init` function is invoked.
 * If the registry has reached MAX_PLUGINS, the function has no effect.
 *
 * @param plugin Plugin to register; must point to a valid Plugin instance. Passing NULL
 *               results in undefined behavior.
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
 * Invoke the update callback for every registered plugin that provides one.
 *
 * Iterates all currently registered plugins and calls each plugin's `update`
 * function if the function pointer is non-NULL.
 */
void plugin_registry_update_all(void) {
    for (int i = 0; i < plugin_count; i++) {
        if (plugins[i]->update) {
            plugins[i]->update();
        }
    }
}

/**
 * Invoke the draw callback of every registered plugin with the provided Nuklear context.
 *
 * Iterates through all plugins currently registered and calls each plugin's `draw`
 * function when it is non-null.
 *
 * @param ctx Pointer to the Nuklear GUI context passed to each plugin's draw callback.
 */
void plugin_registry_draw_all(struct nk_context* ctx) {
    for (int i = 0; i < plugin_count; i++) {
        if (plugins[i]->draw) {
            plugins[i]->draw(ctx);
        }
    }
}