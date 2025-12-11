#ifndef PLUGIN_REGISTRY_H
#define PLUGIN_REGISTRY_H

#include <stddef.h>

/* Forward declare nk_context to avoid including nuklear.h */
struct nk_context;

/**
 * @brief Plugin interface structure.
 * 
 * Defines the interface that all plugins must implement.
 */
typedef struct {
    const char* name; /**< Plugin name for identification */
    void (*init)(void); /**< Initialization function, called once */
    void (*update)(void); /**< Update function, called every frame/tick */
    void (*draw)(struct nk_context* ctx); /**< Draw function, called to render UI */
} Plugin;

/**
 * @brief Initialize the plugin registry.
 * 
 * Must be called before registering any plugins.
 */
void plugin_registry_init(void);

/**
 * @brief Register a plugin with the registry.
 * 
 * Adds a plugin to the registry and calls its init function if available.
 * 
 * @param plugin Pointer to the plugin to register
 */
void plugin_register(Plugin* plugin);

/**
 * @brief Update all registered plugins.
 * 
 * Calls the update function for each registered plugin.
 */
void plugin_registry_update_all(void);

/**
 * @brief Draw all registered plugins.
 * 
 * Calls the draw function for each registered plugin.
 * 
 * @param ctx Nuklear context for rendering
 */
void plugin_registry_draw_all(struct nk_context* ctx);

#endif // PLUGIN_REGISTRY_H
