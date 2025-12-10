#include "../../vendor/nuklear.h"
#include "../core/time_utils.h"
#include "../core/config.h"
#include "../core/plugin_registry.h"

void draw_clock_ui(struct nk_context *ctx, int width, int height, AppConfig* config, const struct nk_user_font *big_font, const struct nk_user_font *footer_font) {
    if (nk_begin(ctx, "Digital Clock", nk_rect(0, 0, width, height),
        NK_WINDOW_BACKGROUND)) {
        
        // Dynamic array for layout
        nk_layout_row_dynamic(ctx, height * 0.4f, 1);
        
        // Time
        char timeStr[64];
        get_time_string(timeStr, sizeof(timeStr), config->time_format == 12);
        
        // Custom font would go here, for now using standard label
        // To make it big, we might need to change font in the backend or use a property
        // But Nuklear's standard font handling is via nk_style_push_font if loaded.
        if (big_font) nk_style_push_font(ctx, big_font);
        nk_label(ctx, timeStr, NK_TEXT_CENTERED);
        if (big_font) nk_style_pop_font(ctx);
        
        // Date
        if (config->show_date) {
            nk_layout_row_dynamic(ctx, height * 0.1f, 1);
            char dateStr[128];
            get_date_string(dateStr, sizeof(dateStr));
            nk_label(ctx, dateStr, NK_TEXT_CENTERED);
        }

        // Settings / Plugins Area
        // We can use a property to toggle this or just show it at bottom
        nk_layout_row_dynamic(ctx, 30, 1);
        if (nk_tree_push(ctx, NK_TREE_TAB, "Settings", NK_MINIMIZED)) {
            nk_layout_row_dynamic(ctx, 30, 1);
            int fmt = (config->time_format == 12) ? 0 : 1;
            if (nk_option_label(ctx, "12 Hour", fmt == 0)) config->time_format = 12;
            if (nk_option_label(ctx, "24 Hour", fmt == 1)) config->time_format = 24;
            
            int show = config->show_date;
            nk_checkbox_label(ctx, "Show Date", &show);
            config->show_date = show;
            
            if (nk_button_label(ctx, "Save Config")) {
                save_config(config);
            }
            nk_tree_pop(ctx);
        }


        // Plugins
        plugin_registry_draw_all(ctx);

        // Footer
        nk_layout_row_dynamic(ctx, 20, 1);
        if (footer_font) nk_style_push_font(ctx, footer_font);
        nk_label_colored(ctx, "Tech with backbone: The Mule", NK_TEXT_CENTERED, nk_rgb(80, 80, 80));
        if (footer_font) nk_style_pop_font(ctx);

    }
    nk_end(ctx);
}
