# DIGITALCLOCKC

A simple, cross-platform digital clock application written in pure C, featuring both terminal and GUI implementations.

## Overview

DIGITALCLOCKC provides three distinct digital clock implementations:

1. **Terminal Version** (`index.c`) - A cross-platform command-line digital clock
2. **Windows GUI Version** (`digital_clock_win32.c`) - A Windows GUI application using Win32 API
3. **Linux GUI Version** (`digital_clock_gtk.c`) - A Linux GUI application using GTK3

All versions offer customizable time formats and real-time updates, making them perfect for desktop displays, learning C programming, or as a foundation for more complex timing applications.

## Features

### Terminal Version (Cross-Platform)
- ✅ **Cross-platform compatibility** - Works on Windows, Linux, and Unix systems
- ✅ **Multiple time formats** - Choose between 12-hour (AM/PM) and 24-hour formats
- ✅ **Real-time updates** - Display refreshes every second
- ✅ **Clean interface** - Automatically clears screen for a clean display
- ✅ **Interactive setup** - Prompts user for preferred time format

### Windows GUI Version
- ✅ **Native Windows GUI** - Uses Win32 API for optimal performance
- ✅ **Elegant design** - Green digital text on black background
- ✅ **Date display** - Shows current date alongside time
- ✅ **Keyboard shortcuts** - Easy toggling of display options
- ✅ **Custom fonts** - Uses Arial font for clear readability
- ✅ **Resizable window** - Standard Windows window controls

### Linux GUI Version
- ✅ **Native Linux GUI** - Uses GTK3 for optimal performance and native look
- ✅ **Consistent design** - Green digital text on black background matching Windows version
- ✅ **Date display** - Shows current date alongside time with toggle functionality
- ✅ **Keyboard shortcuts** - Same shortcuts as Windows version (T, D, Q/Escape)
- ✅ **Right-click menu** - Context menu for easy access to features
- ✅ **About dialog** - Built-in help and version information
- ✅ **Resizable window** - Standard GTK window controls
- ✅ **CSS styling** - Custom styling for consistent appearance across themes

## System Requirements

### Terminal Version
- **Compiler**: GCC or compatible C compiler
- **OS**: Windows, Linux, macOS, or any Unix-like system
- **C Standard**: C99 or later

### Windows GUI Version
- **OS**: Windows (any version supporting Win32 API)
- **Compiler**: GCC with MinGW, Visual Studio, or compatible Windows C compiler
- **Libraries**: GDI32, User32 (typically included with Windows development tools)

### Linux GUI Version
- **OS**: Linux (any distribution with GTK3 support)
- **Compiler**: GCC or compatible C compiler
- **Libraries**: GTK3 development libraries, pkg-config
- **Dependencies**: `libgtk-3-dev` (Ubuntu/Debian), `gtk3-devel` (Fedora), `gtk3` (Arch)

## Installation & Building

### Quick Start with Makefile (Recommended)

```bash
# Clone the repository
git clone https://github.com/francismul/DIGITALCLOCKC.git
cd DIGITALCLOCKC

# Build terminal version (works on all platforms)
make terminal

# Build Linux GTK GUI version (Linux only)
make gtk

# Build Windows GUI version (requires MinGW cross-compiler)
make CC=x86_64-w64-mingw32-gcc win32

# Build all available versions for your platform
make build-all

# See all available options
make help
```

### Install Dependencies

#### Ubuntu/Debian:
```bash
make install-deps-ubuntu
# Or manually:
sudo apt-get install build-essential libgtk-3-dev pkg-config
```

#### Fedora:
```bash
make install-deps-fedora
# Or manually:
sudo dnf groupinstall "Development Tools"
sudo dnf install gtk3-devel pkg-config
```

#### Arch Linux:
```bash
make install-deps-arch
# Or manually:
sudo pacman -S base-devel gtk3 pkg-config
```

### Manual Building

#### Terminal Version

##### On Linux/macOS/Unix:
```bash
# Clone the repository
git clone https://github.com/francismul/DIGITALCLOCKC.git
cd DIGITALCLOCKC

# Compile the terminal version
gcc -o digital_clock index.c

# Run the application
./digital_clock
```

#### On Windows (with MinGW):
```cmd
# Compile the terminal version
gcc -o digital_clock.exe index.c

# Run the application
digital_clock.exe
```

#### Linux GUI Version

```bash
# Install GTK3 development libraries first
sudo apt-get install libgtk-3-dev pkg-config  # Ubuntu/Debian
# OR
sudo dnf install gtk3-devel pkg-config         # Fedora
# OR  
sudo pacman -S gtk3 pkg-config                 # Arch Linux

# Compile the Linux GUI version
gcc -o digital_clock_gtk digital_clock_gtk.c `pkg-config --cflags --libs gtk+-3.0`

# Run the application
./digital_clock_gtk
```

#### Windows GUI Version

##### With MinGW:
```cmd
# Compile the GUI version
gcc -o digital_clock_gui.exe digital_clock_win32.c -lgdi32 -luser32

# Run the application
digital_clock_gui.exe
```

##### Cross-compile from Linux:
```bash
# Install MinGW cross-compiler
sudo apt-get install mingw-w64  # Ubuntu/Debian

# Compile Windows version
x86_64-w64-mingw32-gcc -o digital_clock_win32.exe digital_clock_win32.c -lgdi32 -luser32
```

#### With Visual Studio:
1. Open Visual Studio Command Prompt
2. Compile: `cl digital_clock_win32.c user32.lib gdi32.lib`
3. Run: `digital_clock_win32.exe`

## Usage

### Terminal Version
1. Run the compiled executable
2. Choose your preferred time format when prompted:
   - Enter `1` for 12-hour format (with AM/PM)
   - Enter `2` for 24-hour format
3. The clock will display and update every second
4. Press `Ctrl+C` to exit

### Windows GUI Version
1. Run the compiled executable (`digital_clock_win32.exe`)
2. A window will open displaying the current time and date
3. Use keyboard shortcuts to customize the display:
   - **T** or **t**: Toggle between 12-hour and 24-hour time formats
   - **D** or **d**: Toggle date display on/off
4. Close the window or press Alt+F4 to exit

### Linux GUI Version
1. Run the compiled executable (`digital_clock_gtk`)
2. A GTK window will open displaying the current time and date
3. Use keyboard shortcuts to customize the display:
   - **T** or **t**: Toggle between 12-hour and 24-hour time formats
   - **D** or **d**: Toggle date display on/off
   - **Q** or **Escape**: Quit the application
4. Right-click anywhere in the window for a context menu with additional options
5. Close the window normally or use keyboard shortcuts to exit


## Key Functions

### Terminal Version (`index.c`)
- `printTime(int format)` - Displays formatted current time
- `getTimeFormat()` - Prompts user for time format preference
- `clearScreen()` - Cross-platform screen clearing
- `performSleep(int seconds)` - Cross-platform sleep function

### Windows GUI Version (`digital_clock_win32.c`)
- `GetTimeString()` - Formats time as string for display
- `GetDateString()` - Formats date as string for display
- `WndProc()` - Windows message handler for GUI events
- `WinMain()` - Entry point for Windows GUI application

### Linux GUI Version (`digital_clock_gtk.c`)
- `get_time_string()` - Formats time as string for display
- `get_date_string()` - Formats date as string for display
- `update_time()` - GTK timer callback for display updates
- `on_key_press()` - Keyboard event handler
- `show_context_menu()` - Right-click context menu handler
- `setup_styling()` - CSS styling setup for GTK widgets

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Add tests if applicable
5. Commit your changes (`git commit -m 'Add amazing feature'`)
6. Push to the branch (`git push origin feature/amazing-feature`)
7. Open a Pull Request

### Development Guidelines
- Follow existing code style and conventions
- Ensure cross-platform compatibility for terminal version
- Test on both Windows and Unix-like systems when possible
- Add appropriate comments for complex logic
- Update documentation for new features


## Version Information

- **Current Version**: v2.0.0
- **License**: [MIT License](LICENSE)
- **Author**: Francis Mule (francismul)

## Platform-Specific Notes

### Windows
- The GUI version provides the best experience on Windows
- MinGW-w64 is recommended for compilation
- Both terminal and GUI versions work with standard Windows command prompt

### Linux
- Both terminal and GTK GUI versions are available
- GTK version provides native Linux desktop integration
- Requires GTK3 development libraries for GUI compilation
- Terminal version works with most standard C compilers (gcc, clang)

### macOS
- Terminal version works with Xcode command line tools
- Use `brew install gcc` if needed
- GUI version not yet available (future enhancement)

## Troubleshooting

### Common Issues

**"Command not found" error:**
- Ensure the compiled binary has execute permissions: `chmod +x digital_clock`
- Check that you're in the correct directory

**Compilation errors on Windows:**
- Install MinGW-w64 or Visual Studio Build Tools
- Ensure PATH includes compiler location

**GTK compilation errors on Linux:**
- Install GTK3 development libraries: `sudo apt-get install libgtk-3-dev pkg-config`
- Ensure pkg-config is installed and working: `pkg-config --modversion gtk+-3.0`

**GUI application won't start:**
- For GTK version, ensure you're running in a graphical environment (X11/Wayland)
- Check that all required GTK3 libraries are installed

**Display issues:**
- For terminal version, ensure your terminal supports ANSI escape sequences
- Try a different terminal emulator if text doesn't display correctly

## Future Enhancements

- [ ] Color customization options
- [ ] Multiple timezone support
- [ ] Alarm functionality
- [ ] Configuration file support
- [ ] System tray integration (Windows/Linux)
- [ ] macOS native GUI version
- [ ] Web-based version
- [ ] Themes and customizable fonts
- [ ] Window transparency options

---

**Happy Timing! ⏰**

For questions, suggestions, or contributions, please open an issue or submit a pull request.
