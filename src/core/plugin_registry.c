#include "plugin_registry.h"
#include <stddef.h>

#define MAX_PLUGINS 10

static Plugin* plugins[MAX_PLUGINS];
static int plugin_count = 0;

void plugin_registry_init(void) {
    plugin_count = 0;
}

void plugin_register(Plugin* plugin) {
    if (plugin_count < MAX_PLUGINS) {
        plugins[plugin_count++] = plugin;
        if (plugin->init) {
            plugin->init();
        }
    }
}

void plugin_registry_update_all(void) {
    for (int i = 0; i < plugin_count; i++) {
        if (plugins[i]->update) {
            plugins[i]->update();
        }
    }
}

void plugin_registry_draw_all(struct nk_context* ctx) {
    for (int i = 0; i < plugin_count; i++) {
        if (plugins[i]->draw) {
            plugins[i]->draw(ctx);
        }
    }
}
