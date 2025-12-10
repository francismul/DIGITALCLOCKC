#ifndef PLUGIN_REGISTRY_H
#define PLUGIN_REGISTRY_H

#include "../../vendor/nuklear.h"

typedef struct {
    const char* name;
    void (*init)(void);
    void (*update)(void); // Called every frame/tick
    void (*draw)(struct nk_context* ctx);
} Plugin;

void plugin_registry_init(void);
void plugin_register(Plugin* plugin);
void plugin_registry_update_all(void);
void plugin_registry_draw_all(struct nk_context* ctx);

#endif // PLUGIN_REGISTRY_H
