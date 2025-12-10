#include "../core/plugin_registry.h"
#include <stdio.h>
#include <time.h>

// Simple stopwatch state
static int running = 0;
static time_t start_time;
static double elapsed = 0.0;

static void stopwatch_init(void) {
    running = 0;
    elapsed = 0.0;
}

static void stopwatch_update(void) {
    if (running) {
        time_t now;
        time(&now);
        elapsed = difftime(now, start_time);
    }
}

static void stopwatch_draw(struct nk_context* ctx) {
    if (nk_tree_push(ctx, NK_TREE_TAB, "Stopwatch", NK_MINIMIZED)) {
        nk_layout_row_dynamic(ctx, 30, 1);
        char buffer[32];
        int h = (int)(elapsed / 3600);
        int m = (int)((elapsed - h*3600) / 60);
        int s = (int)(elapsed - h*3600 - m*60);
        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", h, m, s);
        nk_label(ctx, buffer, NK_TEXT_CENTERED);

        nk_layout_row_dynamic(ctx, 30, 2);
        if (nk_button_label(ctx, running ? "Stop" : "Start")) {
            if (running) {
                running = 0;
            } else {
                running = 1;
                time(&start_time);
                start_time -= (time_t)elapsed; // Resume
            }
        }
        if (nk_button_label(ctx, "Reset")) {
            running = 0;
            elapsed = 0.0;
        }
        nk_tree_pop(ctx);
    }
}

Plugin stopwatch_plugin = {
    .name = "Stopwatch",
    .init = stopwatch_init,
    .update = stopwatch_update,
    .draw = stopwatch_draw
};
