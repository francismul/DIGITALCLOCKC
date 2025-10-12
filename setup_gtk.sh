#!/bin/bash
# GTK Installation and Build Verification Script for Digital Clock v2.0.0

echo "Digital Clock v2.0.0 - GTK Build Verification"
echo "=============================================="

# Check if we're on Linux
if [[ "$OSTYPE" != "linux-gnu"* ]]; then
    echo "❌ GTK version requires Linux. Current OS: $OSTYPE"
    echo "💡 Use the terminal version for cross-platform compatibility"
    exit 1
fi

echo "✅ Linux detected: $OSTYPE"

# Check for required tools
echo ""
echo "Checking build dependencies..."

# Check gcc
if command -v gcc &> /dev/null; then
    echo "✅ GCC compiler found: $(gcc --version | head -n1)"
else
    echo "❌ GCC compiler not found"
    echo "💡 Install with: sudo apt-get install build-essential"
    exit 1
fi

# Check pkg-config
if command -v pkg-config &> /dev/null; then
    echo "✅ pkg-config found: $(pkg-config --version)"
else
    echo "❌ pkg-config not found"
    echo "💡 Install with: sudo apt-get install pkg-config"
    exit 1
fi

# Check GTK3 development libraries
if pkg-config --exists gtk+-3.0; then
    GTK_VERSION=$(pkg-config --modversion gtk+-3.0)
    echo "✅ GTK3 development libraries found: $GTK_VERSION"
else
    echo "❌ GTK3 development libraries not found"
    echo "💡 Install with one of the following:"
    echo "   Ubuntu/Debian: sudo apt-get install libgtk-3-dev"
    echo "   Fedora:        sudo dnf install gtk3-devel"
    echo "   Arch Linux:    sudo pacman -S gtk3"
    exit 1
fi

echo ""
echo "🎉 All dependencies satisfied!"
echo ""
echo "Building GTK version..."

# Try to build
if gcc -o digital_clock_gtk digital_clock_gtk.c $(pkg-config --cflags --libs gtk+-3.0); then
    echo "✅ GTK version built successfully!"
    echo ""
    echo "Run with: ./digital_clock_gtk"
    echo ""
    echo "Keyboard shortcuts:"
    echo "  T - Toggle time format (12/24 hour)"
    echo "  D - Toggle date display"
    echo "  Q/Escape - Quit application"
    echo "  Right-click for context menu"
else
    echo "❌ Build failed"
    echo "💡 Check the error messages above and ensure all dependencies are installed"
    exit 1
fi

echo ""
echo "🚀 Digital Clock v2.0.0 GTK version ready!"