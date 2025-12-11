#ifndef CLOCK_UI_H
#define CLOCK_UI_H

/* Forward declarations to minimize nuklear.h inclusion */
struct nk_context;
struct nk_user_font;

#include "../core/config.h"

/**
 * @brief Draw the main clock user interface.
 * 
 * Renders the digital clock UI using Nuklear GUI library.
 * 
 * @param ctx Nuklear context for rendering
 * @param width Window width
 * @param height Window height
 * @param config Application configuration
 * @param big_font Font for time display
 * @param footer_font Font for footer text
 */
void draw_clock_ui(struct nk_context *ctx, int width, int height, AppConfig* config, const struct nk_user_font *big_font, const struct nk_user_font *footer_font);

#endif
