# Digital Clock Makefile
# Supports building terminal, Windows GUI, and Linux GTK versions

CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Platform detection
UNAME_S := $(shell uname -s)

# Default target
all: terminal

# Terminal version (cross-platform)
terminal: digital_clock
digital_clock: index.c
	$(CC) $(CFLAGS) -o digital_clock index.c

# Linux GTK version
gtk: digital_clock_gtk
digital_clock_gtk: digital_clock_gtk.c
ifeq ($(UNAME_S),Linux)
	$(CC) $(CFLAGS) -o digital_clock_gtk digital_clock_gtk.c `pkg-config --cflags --libs gtk+-3.0`
else
	@echo "GTK version can only be built on Linux systems"
	@exit 1
endif

# Windows GUI version (requires MinGW on Linux or Windows compiler)
win32: digital_clock_win32.exe
digital_clock_win32.exe: digital_clock_win32.c
ifeq ($(CC),x86_64-w64-mingw32-gcc)
	$(CC) $(CFLAGS) -o digital_clock_win32.exe digital_clock_win32.c -lgdi32 -luser32
else ifdef MINGW_PREFIX
	x86_64-w64-mingw32-gcc $(CFLAGS) -o digital_clock_win32.exe digital_clock_win32.c -lgdi32 -luser32
else
	@echo "Windows version requires MinGW cross-compiler"
	@echo "Install with: sudo apt-get install mingw-w64"
	@echo "Then run: make CC=x86_64-w64-mingw32-gcc win32"
	@exit 1
endif

# Build all versions (platform-dependent)
build-all:
ifeq ($(UNAME_S),Linux)
	$(MAKE) terminal
	$(MAKE) gtk
	@echo "Terminal and GTK versions built successfully"
	@echo "For Windows version, install MinGW and run: make CC=x86_64-w64-mingw32-gcc win32"
else
	$(MAKE) terminal
	@echo "Terminal version built successfully"
	@echo "GTK version requires Linux with GTK development libraries"
endif

# Installation targets
install-deps-ubuntu:
	sudo apt-get update
	sudo apt-get install build-essential libgtk-3-dev pkg-config

install-deps-fedora:
	sudo dnf groupinstall "Development Tools"
	sudo dnf install gtk3-devel pkg-config

install-deps-arch:
	sudo pacman -S base-devel gtk3 pkg-config

# Clean build artifacts
clean:
	rm -f digital_clock digital_clock_gtk digital_clock_win32.exe
	rm -f *.o

# Help target
help:
	@echo "Digital Clock Build System"
	@echo "========================="
	@echo ""
	@echo "Available targets:"
	@echo "  terminal     - Build cross-platform terminal version"
	@echo "  gtk          - Build Linux GTK GUI version"
	@echo "  win32        - Build Windows GUI version (requires MinGW)"
	@echo "  build-all    - Build all available versions for current platform"
	@echo "  clean        - Remove all build artifacts"
	@echo ""
	@echo "Installation helpers:"
	@echo "  install-deps-ubuntu  - Install dependencies on Ubuntu/Debian"
	@echo "  install-deps-fedora  - Install dependencies on Fedora"
	@echo "  install-deps-arch    - Install dependencies on Arch Linux"
	@echo ""
	@echo "Examples:"
	@echo "  make terminal                              # Build terminal version"
	@echo "  make gtk                                   # Build GTK version (Linux only)"
	@echo "  make CC=x86_64-w64-mingw32-gcc win32      # Cross-compile Windows version"
	@echo "  make build-all                            # Build all available versions"

# Run targets
run-terminal: digital_clock
	./digital_clock

run-gtk: digital_clock_gtk
	./digital_clock_gtk

# Test target
test: terminal
	@echo "Testing terminal version..."
	@timeout 3 ./digital_clock || echo "Terminal version test completed"

.PHONY: all terminal gtk win32 build-all clean help install-deps-ubuntu install-deps-fedora install-deps-arch run-terminal run-gtk test