#include <gtk/gtk.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

// Global variables
GtkWidget *time_label;
GtkWidget *date_label;
GtkWidget *window;
int timeFormat = 24; // default to 24hr
int showDate = 1;    // 1 to show date, 0 to hide
guint timer_id;

// Function to format current time as a string
/**
 * @brief Get the current time as a formatted string.
 * 
 * Formats the current system time into a string buffer.
 * 
 * @param buffer Buffer to store the formatted time string
 * @param bufferSize Size of the buffer
 * @param format Time format: 12 for 12-hour, 24 for 24-hour
 */
void get_time_string(char *buffer, int bufferSize, int format)
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    int hour = timeinfo->tm_hour;
    if (format == 12)
    {
        if (hour == 0)
            hour = 12;
        else if (hour > 12)
            hour -= 12;
        snprintf(buffer, bufferSize, "%02d:%02d:%02d %s",
                 hour,
                 timeinfo->tm_min,
                 timeinfo->tm_sec,
                 (timeinfo->tm_hour >= 12 ? "PM" : "AM"));
    }
    else
    {
        snprintf(buffer, bufferSize, "%02d:%02d:%02d",
                 hour,
                 timeinfo->tm_min,
                 timeinfo->tm_sec);
    }
}

// Function to format current date as a string
/**
 * @brief Get the current date as a formatted string.
 * 
 * Formats the current system date into a string buffer.
 * 
 * @param buffer Buffer to store the formatted date string
 * @param bufferSize Size of the buffer
 */
void get_date_string(char *buffer, int bufferSize)
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    const char *days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", 
                         "Thursday", "Friday", "Saturday"};
    const char *months[] = {"January", "February", "March", "April", "May", "June",
                           "July", "August", "September", "October", "November", "December"};

    snprintf(buffer, bufferSize, "%s, %s %02d, %d",
             days[timeinfo->tm_wday],
             months[timeinfo->tm_mon],
             timeinfo->tm_mday,
             timeinfo->tm_year + 1900);
}

// Timer callback function to update the display
/**
 * @brief Update the time and date display.
 * 
 * Timer callback that refreshes the GUI with current time and date.
 * 
 * @param data User data (unused)
 * @return TRUE to continue the timer
 */
gboolean update_time(gpointer data)
{
    char timeStr[64];
    char dateStr[128];
    
    // Get current time
    get_time_string(timeStr, sizeof(timeStr), timeFormat);
    
    // Update time label with markup for styling
    char time_markup[256];
    snprintf(time_markup, sizeof(time_markup), 
             "<span font='Arial Bold 60' color='#00FF00'>%s</span>", timeStr);
    gtk_label_set_markup(GTK_LABEL(time_label), time_markup);
    
    // Update date label if enabled
    if (showDate)
    {
        get_date_string(dateStr, sizeof(dateStr));
        char date_markup[256];
        snprintf(date_markup, sizeof(date_markup), 
                 "<span font='Arial 30' color='#00FF00'>%s</span>", dateStr);
        gtk_label_set_markup(GTK_LABEL(date_label), date_markup);
        gtk_widget_show(date_label);
    }
    else
    {
        gtk_widget_hide(date_label);
    }
    
    return TRUE; // Continue calling this function
}

// Key press event handler
/**
 * @brief Handle key press events.
 * 
 * Processes keyboard shortcuts for toggling time format and date display.
 * 
 * @param widget The widget that received the event
 * @param event The key press event
 * @param data User data (unused)
 * @return FALSE to allow further processing
 */
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    switch (event->keyval)
    {
        case GDK_KEY_t:
        case GDK_KEY_T:
            // Toggle time format
            timeFormat = (timeFormat == 24) ? 12 : 24;
            update_time(NULL); // Update immediately
            break;
            
        case GDK_KEY_d:
        case GDK_KEY_D:
            // Toggle date display
            showDate = !showDate;
            update_time(NULL); // Update immediately
            break;
            
        case GDK_KEY_q:
        case GDK_KEY_Q:
        case GDK_KEY_Escape:
            // Quit application
            gtk_main_quit();
            break;
    }
    
    return FALSE;
}

// Window close event handler
/**
 * @brief Handle window close events.
 * 
 * Cleans up resources when the window is closed.
 * 
 * @param widget The widget that received the event
 * @param event The event
 * @param data User data (unused)
 * @return FALSE to allow the window to close
 */
gboolean on_window_delete(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    if (timer_id > 0)
    {
        g_source_remove(timer_id);
    }
    gtk_main_quit();
    return FALSE;
}

// Setup CSS styling
/**
 * @brief Set up CSS styling for the application.
 * 
 * Applies custom CSS to style the GTK widgets.
 */
void setup_styling()
{
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css = 
        "window {"
        "    background-color: #000000;"
        "}"
        "label {"
        "    background-color: transparent;"
        "}";
    
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    g_object_unref(provider);
}

// Menu callback functions
/**
 * @brief Toggle time format callback.
 * 
 * Switches between 12-hour and 24-hour time display.
 * 
 * @param menuitem The menu item that was activated
 * @param user_data User data (unused)
 */
void toggle_time_format_callback(GtkMenuItem *menuitem, gpointer user_data)
{
    timeFormat = (timeFormat == 24) ? 12 : 24;
    update_time(NULL); // Update immediately
}

void toggle_date_display_callback(GtkMenuItem *menuitem, gpointer user_data)
{
    showDate = !showDate;
    update_time(NULL); // Update immediately
}

// Create about dialog
/**
 * @brief Show the about dialog.
 * 
 * Displays information about the application.
 */
void show_about_dialog()
{
    GtkWidget *about_dialog = gtk_about_dialog_new();
    
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dialog), "Digital Clock");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog), "2.0.0");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog), 
        "A simple digital clock application for Linux\n\nKeyboard shortcuts:\n"
        "T - Toggle time format (12/24 hour)\n"
        "D - Toggle date display\n"
        "Q/Escape - Quit application");
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dialog), "Copyright © 2025 Francis Mulumba");
    gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(about_dialog), GTK_LICENSE_MIT_X11);
    
    gtk_dialog_run(GTK_DIALOG(about_dialog));
    gtk_widget_destroy(about_dialog);
}

// Right-click context menu
/**
 * @brief Show the context menu on right-click.
 * 
 * Creates and displays a context menu with application options.
 * 
 * @param widget The widget that was clicked
 * @param event The button press event
 */
void show_context_menu(GtkWidget *widget, GdkEventButton *event)
{
    if (event->button == GDK_BUTTON_SECONDARY) // Right click
    {
        GtkWidget *menu = gtk_menu_new();
        
        // Toggle time format menu item
        GtkWidget *toggle_format = gtk_menu_item_new_with_label(
            timeFormat == 24 ? "Switch to 12-hour format" : "Switch to 24-hour format");
        g_signal_connect(toggle_format, "activate", 
            G_CALLBACK(toggle_time_format_callback), NULL);
        
        // Toggle date menu item
        GtkWidget *toggle_date = gtk_menu_item_new_with_label(
            showDate ? "Hide Date" : "Show Date");
        g_signal_connect(toggle_date, "activate", 
            G_CALLBACK(toggle_date_display_callback), NULL);
        
        // Separator
        GtkWidget *separator = gtk_separator_menu_item_new();
        
        // About menu item
        GtkWidget *about_item = gtk_menu_item_new_with_label("About");
        g_signal_connect_swapped(about_item, "activate", 
            G_CALLBACK(show_about_dialog), NULL);
        
        // Quit menu item
        GtkWidget *quit_item = gtk_menu_item_new_with_label("Quit");
        g_signal_connect_swapped(quit_item, "activate", 
            G_CALLBACK(gtk_main_quit), NULL);
        
        // Add items to menu
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), toggle_format);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), toggle_date);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), about_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit_item);
        
        // Show all menu items
        gtk_widget_show_all(menu);
        
        // Display the menu
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
        
        // Release our reference to the menu to avoid memory leaks
        // GTK keeps the menu alive during popup, so we can safely drop our reference
        g_object_ref_sink(menu);
        g_object_unref(menu);
    }
}

// Button press event handler for context menu
/**
 * @brief Handle button press events for context menu.
 * 
 * Shows the context menu on right-click.
 * 
 * @param widget The widget that received the event
 * @param event The button press event
 * @param data User data (unused)
 * @return FALSE to allow further processing
 */
gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    show_context_menu(widget, event);
    return FALSE;
}

/**
 * @brief Main entry point for the GTK clock application.
 * 
 * Initializes GTK, creates the window, and runs the main loop.
 * 
 * @param argc Number of command line arguments
 * @param argv Command line arguments
 * @return Exit status
 */
int main(int argc, char *argv[])
{
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Create main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Digital Clock - Pure C (GTK)");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    
    // Setup CSS styling
    setup_styling();
    
    // Create vertical box container
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Create time label
    time_label = gtk_label_new("");
    gtk_widget_set_halign(time_label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(time_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), time_label, TRUE, TRUE, 0);
    
    // Create date label
    date_label = gtk_label_new("");
    gtk_widget_set_halign(date_label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(date_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), date_label, TRUE, TRUE, 0);
    
    // Set up event handlers
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_delete), NULL);
    g_signal_connect(window, "delete-event", G_CALLBACK(on_window_delete), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);
    g_signal_connect(window, "button-press-event", G_CALLBACK(on_button_press), NULL);
    
    // Make window focusable to receive key events
    gtk_widget_set_can_focus(window, TRUE);
    gtk_widget_grab_focus(window);
    
    // Add event mask for button presses
    gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK);
    
    // Start timer to update every second
    timer_id = g_timeout_add(1000, update_time, NULL);
    
    // Initial update
    update_time(NULL);
    
    // Show all widgets
    gtk_widget_show_all(window);
    
    // Print usage information to console
    printf("Digital Clock v2.0.0 - Linux GTK Version\n");
    printf("========================================\n");
    printf("Keyboard shortcuts:\n");
    printf("  T - Toggle time format (12/24 hour)\n");
    printf("  D - Toggle date display\n");
    printf("  Q/Escape - Quit application\n");
    printf("  Right-click for context menu\n");
    printf("========================================\n");
    
    // Run GTK main loop
    gtk_main();
    
    return 0;
}