#include "../core/plugin_registry.h"
#include <stdio.h>
#include <time.h>

// Simple stopwatch state
static int running = 0;
static time_t start_time;
static double elapsed = 0.0;

/**
 * Initialize the stopwatch state to stopped and zero elapsed time.
 */
static void stopwatch_init(void) {
    running = 0;
    elapsed = 0.0;
}

/**
 * Update stopwatch elapsed time when running.
 *
 * When the stopwatch is active, sets the module's `elapsed` value to the
 * difference between the current system time and the stored `start_time`.
 */
static void stopwatch_update(void) {
    if (running) {
        time_t now;
        time(&now);
        elapsed = difftime(now, start_time);
    }
}

/**
 * Render the stopwatch UI and handle user interactions.
 *
 * Displays the elapsed time formatted as HH:MM:SS and provides controls to start/stop and reset
 * the stopwatch. Pressing Start captures the current time and resumes timing (preserving prior
 * elapsed time); pressing Stop pauses the stopwatch; pressing Reset stops timing and clears
 * the elapsed time.
 *
 * @param ctx Nuklear UI context used to build and render the stopwatch widgets.
 */
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