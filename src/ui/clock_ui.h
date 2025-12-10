#ifndef CLOCK_UI_H
#define CLOCK_UI_H

#include "../../vendor/nuklear.h"
#include "../core/config.h"

void draw_clock_ui(struct nk_context *ctx, int width, int height, AppConfig* config, const struct nk_user_font *big_font, const struct nk_user_font *footer_font);

#endif
