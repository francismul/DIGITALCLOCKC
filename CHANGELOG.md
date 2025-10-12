# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v2.0.0] - 2025-10-12

### Added
- **Linux GTK GUI Version** (`digital_clock_gtk.c`)
  - Native Linux GUI implementation using GTK3
  - Feature parity with Windows GUI version
  - Green digital text on black background matching Windows version
  - Date display alongside time with toggle functionality
  - Keyboard shortcuts (T for time format, D for date toggle, Q/Escape to quit)
  - Right-click context menu for easy access to features
  - About dialog with version information and keyboard shortcuts
  - Resizable window with proper GTK styling
  - Cross-platform GUI support (Windows + Linux)

- **Enhanced Build System**
  - Comprehensive Makefile with multiple targets
  - Platform-specific build instructions
  - Dependency installation helpers for major Linux distributions
  - Cross-compilation support for Windows builds on Linux
  - Build verification and testing targets
  - Clean and help targets for better developer experience

- **Improved Project Structure**
  - Organized build system for all three versions (terminal, Windows GUI, Linux GUI)
  - Platform detection and appropriate build target selection
  - Installation documentation for development dependencies

### Changed
- **Version Bump**: Major version increase to 2.0.0 due to significant new GUI platform support
- **Multi-Platform Support**: Now officially supports GUI applications on both Windows and Linux
- **Build Process**: Streamlined build process with automated dependency checking

### Technical Features
- **GTK3 Integration**: Full GTK3 widget toolkit integration for native Linux look and feel
- **CSS Styling**: Custom CSS for consistent black background across GTK themes
- **Event Handling**: Comprehensive keyboard and mouse event handling
- **Memory Management**: Proper GTK resource management and cleanup
- **Timer Integration**: GTK timeout functions for smooth 1-second updates
- **Menu System**: Context menu system with right-click functionality

### Dependencies
- **Linux GTK Version**: Requires GTK3 development libraries and pkg-config
- **Build Tools**: Standard build tools (gcc, make) for all platforms
- **Cross-Compilation**: MinGW support for building Windows version on Linux

## [v1.1.0] - 2024-10-12

### Added
- Comprehensive README documentation with detailed features, installation instructions, and usage examples
- Build instructions for both terminal and GUI versions
- System requirements documentation
- Enhanced project overview with clear feature breakdown

### Changed
- Improved documentation structure and formatting
- Updated README with better organization and readability

### Fixed
- Various cleanup improvements to codebase

## [v1.0.3] - 2024

### Fixed
- Release patch improvements
- General bug fixes and stability improvements

## [v1.0.2] - 2024

### Fixed
- Build system fixes
- Compilation improvements

## [v1.0.1] - 2024

### Fixed
- Critical build fixes
- Resolved compilation issues

## [v1.0.0] - 2024

### Added
- Initial stable release
- VERSION file with proper version tracking
- GitHub release workflow with manual execution trigger
- Comprehensive release instructions documentation

### Changed
- Established stable API and feature set

## [Pre-1.0.0] - Development Releases

### Added
- **GUI Digital Clock** (`digital_clock_win32.c`)
  - Native Windows GUI implementation using Win32 API
  - Date display alongside time
  - Keyboard shortcuts for display options
  - Custom Arial font for clear readability
  - Resizable window with standard Windows controls
  - Green digital text on black background
  - Toggle functionality for display options

- **Terminal Digital Clock** (`index.c`)
  - Cross-platform command-line digital clock
  - Support for both 12-hour (AM/PM) and 24-hour time formats
  - Real-time updates with automatic screen clearing
  - Interactive time format selection
  - Cross-platform compatibility (Windows, Linux, Unix)
  - Clean and minimal interface

- **Development Infrastructure**
  - GitHub Actions release workflow
  - Unit testing framework integration
  - Build system configuration
  - Cross-platform compilation support

### Technical Features
- **Cross-Platform Sleep Function**: Handles different sleep implementations between Windows and Unix-like systems
- **Memory Management**: Proper font handle management in GUI version
- **Time Formatting**: Robust time formatting functions for both CLI and GUI versions
- **User Input Validation**: Input validation for time format selection
- **Screen Management**: Automatic screen clearing for terminal version

### Build System
- GCC compilation support
- MinGW compatibility for Windows builds
- Makefile configuration (implied from build process)
- Binary output generation (`digital_clock` executable)

### Documentation
- Initial project documentation
- Code comments and inline documentation
- Basic usage instructions

---

## Version History Summary

- **v1.1.0**: Documentation overhaul and comprehensive README
- **v1.0.x**: Stability and build fixes leading to stable release
- **v1.0.0**: First stable release with core functionality
- **Pre-1.0.0**: Feature development including GUI and terminal implementations

## Upcoming Features

Future versions may include:
- macOS native GUI implementation
- Customizable color themes
- Alarm and timer functionality
- Multiple timezone support
- Configuration file support
- More font options and sizing