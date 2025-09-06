# DIGITALCLOCKC

A simple, cross-platform digital clock application written in pure C, featuring both terminal and GUI implementations.

## Overview

DIGITALCLOCKC provides two distinct digital clock implementations:

1. **Terminal Version** (`index.c`) - A cross-platform command-line digital clock
2. **GUI Version** (`digital_clock_win32.c`) - A Windows GUI application using Win32 API

Both versions offer customizable time formats and real-time updates, making them perfect for desktop displays, learning C programming, or as a foundation for more complex timing applications.

## Features

### Terminal Version (Cross-Platform)
- ✅ **Cross-platform compatibility** - Works on Windows, Linux, and Unix systems
- ✅ **Multiple time formats** - Choose between 12-hour (AM/PM) and 24-hour formats
- ✅ **Real-time updates** - Display refreshes every second
- ✅ **Clean interface** - Automatically clears screen for a clean display
- ✅ **Interactive setup** - Prompts user for preferred time format

### GUI Version (Windows)
- ✅ **Native Windows GUI** - Uses Win32 API for optimal performance
- ✅ **Elegant design** - Green digital text on black background
- ✅ **Date display** - Shows current date alongside time
- ✅ **Keyboard shortcuts** - Easy toggling of display options
- ✅ **Custom fonts** - Uses Arial font for clear readability
- ✅ **Resizable window** - Standard Windows window controls

## System Requirements

### Terminal Version
- **Compiler**: GCC or compatible C compiler
- **OS**: Windows, Linux, macOS, or any Unix-like system
- **C Standard**: C99 or later

### GUI Version
- **OS**: Windows (any version supporting Win32 API)
- **Compiler**: GCC with MinGW, Visual Studio, or compatible Windows C compiler
- **Libraries**: GDI32, User32 (typically included with Windows development tools)

## Installation & Building

### Terminal Version

#### On Linux/macOS/Unix:
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

### GUI Version (Windows Only)

#### With MinGW:
```cmd
# Compile the GUI version
gcc -o digital_clock_gui.exe digital_clock_win32.c -lgdi32 -luser32

# Run the application
digital_clock_gui.exe
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

### GUI Version
1. Run the compiled executable
2. A window will open displaying the current time and date
3. Use keyboard shortcuts to customize the display:
   - **T** or **t**: Toggle between 12-hour and 24-hour time formats
   - **D** or **d**: Toggle date display on/off
4. Close the window or press Alt+F4 to exit

## Code Structure

```
DIGITALCLOCKC/
├── index.c                    # Cross-platform terminal implementation
├── digital_clock_win32.c      # Windows GUI implementation
├── test/                      # Test files and framework
│   ├── test_digital_clock_win32_tests.c
│   ├── lib/acutest.h         # Testing framework
│   └── run.sh                # Test runner script
├── VERSION                    # Current version (v1.0.0)
├── RELEASE_INSTRUCTIONS.md    # Release process documentation
└── README.md                 # This file
```

## Key Functions

### Terminal Version (`index.c`)
- `printTime(int format)` - Displays formatted current time
- `getTimeFormat()` - Prompts user for time format preference
- `clearScreen()` - Cross-platform screen clearing
- `performSleep(int seconds)` - Cross-platform sleep function

### GUI Version (`digital_clock_win32.c`)
- `GetTimeString()` - Formats time as string for display
- `GetDateString()` - Formats date as string for display
- `WndProc()` - Windows message handler for GUI events
- `WinMain()` - Entry point for Windows GUI application

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

## Testing

The project includes a test suite for the GUI version:

```bash
# Build and run tests (Linux/macOS with clang)
cd test
clang -Wall -Itest -DTEST_STUBS -o digital_clock_tests test_digital_clock_win32_tests.c
./digital_clock_tests
```

## Version Information

- **Current Version**: v1.0.0
- **License**: [Add license information]
- **Author**: Francis Mulumba (francismul)

## Platform-Specific Notes

### Windows
- The GUI version provides the best experience on Windows
- MinGW-w64 is recommended for compilation
- Both versions work with standard Windows command prompt

### Linux/Unix
- Only the terminal version is available
- Requires a terminal emulator for best experience
- Works with most standard C compilers (gcc, clang)

### macOS
- Terminal version works with Xcode command line tools
- Use `brew install gcc` if needed

## Troubleshooting

### Common Issues

**"Command not found" error:**
- Ensure the compiled binary has execute permissions: `chmod +x digital_clock`
- Check that you're in the correct directory

**Compilation errors on Windows:**
- Install MinGW-w64 or Visual Studio Build Tools
- Ensure PATH includes compiler location

**Display issues:**
- For terminal version, ensure your terminal supports ANSI escape sequences
- Try a different terminal emulator if text doesn't display correctly

## Future Enhancements

- [ ] Color customization options
- [ ] Multiple timezone support
- [ ] Alarm functionality
- [ ] Configuration file support
- [ ] System tray integration (Windows)
- [ ] GTK version for Linux GUI
- [ ] Web-based version

---

**Happy Timing! ⏰**

For questions, suggestions, or contributions, please open an issue or submit a pull request.
