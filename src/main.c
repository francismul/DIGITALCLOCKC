#define _POSIX_C_SOURCE 200809L
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <io.h>
    #define ISATTY _isatty
    #define FILENO _fileno
#else
    #include <unistd.h>
    #define ISATTY isatty
    #define FILENO fileno
#endif

/**
 * @brief Print a dragon banner to the console.
 * 
 * Displays an ASCII art dragon banner if running in a terminal.
 */
static void print_dragon_banner(void) {
    static const char *art[] = {
        "                           ______________",
        "                    ,===:'.,            `-._",
        "                         `:.`---.__         `-._",
        "                           `:.     `--.         `.",
        "                             \\.        `.         `.",
        "                     (,,(,    \\.         `.   ____,-`.,",
        "                  (,'     `/   \\.   ,--.___`.'",
        "              ,  ,'  ,--.  `,   \\.;'         `",
        "               `{D, {    \\  :    \\;",
        "                 V,,'    /  /    //",
        "                 j;;    /  ,' ,-//.    ,---.      ,",
        "                 \\;'   /  ,' /  _  \\  /  _  \\   ,'/",
        "                       \\   `'  / \\  `'  / \\  `.' /",
        "                        `.___,'   `.__,'   `.__,'",
        "",
        "            ╔════════════════════════════════╗",
        "            ║   THANK YOU FOR SUPPORTING     ║",
        "            ║      OPEN SOURCE SOFTWARE      ║",
        "            ╚════════════════════════════════╝"
    };
    size_t lines = sizeof(art)/sizeof(art[0]);
    printf("\n");
    for (size_t i = 0; i < lines; ++i) {
        printf("%s\n", art[i]);
    }
    printf("\n");
}

#include "nuklear_config.h"
#define NK_IMPLEMENTATION
#include "../vendor/nuklear.h"

#include "core/config.h"
#include "core/plugin_registry.h"
#include "core/time_utils.h"
#include "platform/platform.h"
#include "ui/clock_ui.h"

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 200

/**
 * @brief Print the dragon banner once if running in a terminal.
 * 
 * Ensures the banner is only printed once per session.
 */
static void maybe_print_dragon(void) {
    static int printed = 0;
    if (printed) return;
    if (ISATTY(FILENO(stdout))) {
        print_dragon_banner();
        printed = 1;
    }
}

#ifdef _WIN32
    #define NK_GDI_IMPLEMENTATION
    #include <windows.h>
    #include "../vendor/nuklear_gdi.h"

    /**
     * @brief Windows window procedure callback.
     * 
     * Handles Windows messages for the application window.
     * 
     * @param wnd Window handle
     * @param msg Message identifier
     * @param wparam Message parameter
     * @param lparam Message parameter
     * @return Message result
     */
    LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        if (nk_gdi_handle_event(wnd, msg, wparam, lparam))
            return 0;
        return DefWindowProc(wnd, msg, wparam, lparam);
    }

    /**
     * @brief Main entry point for Windows version.
     * 
     * Initializes the application, sets up the GUI, and runs the main loop.
     * 
     * @return Exit status
     */
    int main(void) {
        GdiFont* font;
        GdiFont* big_font;
        GdiFont* footer_font;
        struct nk_context *ctx;
        AppConfig config;
        
        maybe_print_dragon();
        load_config(&config);
        plugin_registry_init();
        // Register plugins (e.g. built-in ones)
        extern Plugin stopwatch_plugin;
        plugin_register(&stopwatch_plugin);

        WNDCLASSW wc;
        memset(&wc, 0, sizeof(wc));
        wc.style = CS_DBLCLKS;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandleW(0);
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"NuklearClock";
        RegisterClassW(&wc);

        HWND wnd = CreateWindowW(L"NuklearClock", L"Digital Clock (Nuklear)",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
            NULL, NULL, wc.hInstance, NULL);

        HDC dc = GetDC(wnd);
        if (!dc) {
            return 1;
        }

        font = nk_gdifont_create("Arial", 18);
        big_font = nk_gdifont_create("Arial", 48);
        footer_font = nk_gdifont_create_with_style("Arial", 18, FW_NORMAL, TRUE);
        ctx = nk_gdi_init(font, dc, WINDOW_WIDTH, WINDOW_HEIGHT);

        if (big_font) {
            nk_gdi_set_font(big_font);
            nk_style_set_font(ctx, &font->nk);
        }
        if (footer_font) {
            nk_gdi_set_font(footer_font);
            nk_style_set_font(ctx, &font->nk);
        }

        while (1) {
            MSG msg;
            nk_input_begin(ctx);
            if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) break;
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            nk_input_end(ctx);

                 draw_clock_ui(ctx, WINDOW_WIDTH, WINDOW_HEIGHT, &config,
                     big_font ? &big_font->nk : NULL,
                     footer_font ? &footer_font->nk : NULL);
             plugin_registry_update_all();

            nk_gdi_render(nk_rgb(30,30,30));
            // Adaptive refresh: Wait until next second if we are just showing connection
            // But nuklear needs responsiveness for UI.
            // We can check if any input happened or if we are animating.
            // For a clock, we need at least 1-second updates.
            // A simple approach: Sleep for small amount to save CPU, but ensure we wake up for the second tick.
            // platform_sleep_until_next_second(); // This might freeze UI for ~1s.
            // Better: Sleep small amount (e.g. 50ms) to poll inputs, unless 
            // "Low Power Mode" is active and window not focused.
            // For now, let's just sleep 30ms to yield CPU.
            platform_sleep_ms(30); 
        }

        ReleaseDC(wnd, dc);
        nk_gdifont_del(font);
        if (big_font) nk_gdifont_del(big_font);
        if (footer_font && footer_font != font) nk_gdifont_del(footer_font);
        return 0;
    }

#else
    #define NK_XLIB_IMPLEMENTATION
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/Xos.h>
    #include "../vendor/nuklear_xlib.h"

    /**
     * @brief Main entry point for Linux/X11 version.
     * 
     * Initializes the application, sets up the GUI, and runs the main loop.
     * 
     * @return Exit status
     */
    int main(void) {
        Display *dpy;
        Window win;
        XFont *font;
        XFont *big_font;
        XFont *footer_font;
        struct nk_context *ctx;
        AppConfig config;

        maybe_print_dragon();
        load_config(&config);
        plugin_registry_init();
        
        // Register plugins
        // Forward declaration for the sample plugin
        // In a real build system, this might be handled via a header or dynamic loading
        // For now we declare it here or include stopwatch.c? No, linking handles it.
        // We need the symbol 'stopwatch_plugin' to be available.
        // We included plugin_registry.h, we need to declare the external plugin var.
        extern Plugin stopwatch_plugin;
        plugin_register(&stopwatch_plugin);

        dpy = XOpenDisplay(NULL);
        if (!dpy) return 1;

        int screen = DefaultScreen(dpy);
        win = XCreateSimpleWindow(dpy, RootWindow(dpy, screen), 
            10, 10, WINDOW_WIDTH, WINDOW_HEIGHT, 1, 
            BlackPixel(dpy, screen), WhitePixel(dpy, screen));
        
        XSetStandardProperties(dpy, win, "Digital Clock (Nuklear)", "DigitalClock", 
            None, NULL, 0, NULL);
        
        XSelectInput(dpy, win, ExposureMask | KeyPressMask | ButtonPressMask | 
            ButtonReleaseMask | PointerMotionMask | StructureNotifyMask); // Add StructureNotifyMask to get ConfigureNotify for resize
            
        XMapWindow(dpy, win);
        
        // Handle delete window protocol
        Atom wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(dpy, win, &wm_delete_window, 1);

        font = nk_xfont_create(dpy, "fixed");
        // Try to load a larger font. If unavailable, fallback to fixed.
        // Standard X11 font naming: -foundry-family-weight-slant-width-style-pixel-size-...
        // Try a common bold font.
        big_font = nk_xfont_create(dpy, "-*-helvetica-bold-r-normal--*-240-*-*-*-*-*-*");
        if (!big_font) big_font = nk_xfont_create(dpy, "fixed"); // Fallback

        footer_font = nk_xfont_create(dpy, "-*-helvetica-medium-o-normal--*-120-*-*-*-*-*-*");
        if (!footer_font) footer_font = font;

        ctx = nk_xlib_init(font, dpy, screen, win, WINDOW_WIDTH, WINDOW_HEIGHT);

        if (big_font) {
            nk_xlib_set_font(big_font);
            nk_style_set_font(ctx, &font->handle);
        }
        if (footer_font && footer_font != font) {
            nk_xlib_set_font(footer_font);
            nk_style_set_font(ctx, &font->handle);
        }

        int running = 1;
        while (running) {
            XEvent evt;
            nk_input_begin(ctx);
            while (XPending(dpy)) {
                XNextEvent(dpy, &evt);
                if (evt.type == ClientMessage) {
                    if ((Atom)evt.xclient.data.l[0] == wm_delete_window)
                        running = 0;
                }
                if (XFilterEvent(&evt, win)) continue;
                nk_xlib_handle_event(dpy, screen, win, &evt);
            }
            nk_input_end(ctx);

            /* Logic */
            plugin_registry_update_all();
            
            /* Draw */
             // Get current window size for responsive layout
            XWindowAttributes win_attr;
            XGetWindowAttributes(dpy, win, &win_attr);
            
            draw_clock_ui(ctx, win_attr.width, win_attr.height, &config,
                big_font ? &big_font->handle : NULL,
                footer_font ? &footer_font->handle : NULL);

            nk_xlib_render(win, nk_rgb(30,30,30));
            
            /* Power Saving: Sleep a bit to prevent 100% CPU on loop */
             platform_sleep_ms(30);
        }

        nk_xlib_shutdown();
        nk_xfont_del(dpy, font);
        if (big_font) nk_xfont_del(dpy, big_font);
        if (footer_font && footer_font != font) nk_xfont_del(dpy, footer_font);
        XCloseDisplay(dpy);
        return 0;
    }
#endif
