#!/bin/bash
# Fetch MicroPython source code for XiaoMiao OS
# This script downloads MicroPython v1.22.0+ for ESP32

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
COMPONENT_DIR="$PROJECT_DIR/components/micropython"
MICROPYTHON_DIR="$COMPONENT_DIR/micropython"

MICROPYTHON_VERSION="v1.22.0"
MICROPYTHON_REPO="https://github.com/micropython/micropython.git"

echo "========================================="
echo "  Fetching MicroPython $MICROPYTHON_VERSION"
echo "========================================="

# Check if already exists
if [ -d "$MICROPYTHON_DIR/py" ]; then
    echo "MicroPython source already exists at:"
    echo "  $MICROPYTHON_DIR"
    echo ""
    read -p "Re-download? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Skipping download."
        exit 0
    fi
    rm -rf "$MICROPYTHON_DIR"
fi

# Clone MicroPython
echo "Cloning MicroPython from GitHub..."
git clone --depth 1 --branch "$MICROPYTHON_VERSION" "$MICROPYTHON_REPO" "$MICROPYTHON_DIR"

if [ $? -ne 0 ]; then
    echo "ERROR: Failed to clone MicroPython"
    exit 1
fi

echo ""
echo "MicroPython source downloaded successfully!"
echo "Location: $MICROPYTHON_DIR"
echo ""
echo "Next steps:"
echo "  1. Build XiaoMiao OS: ./build.sh"
echo "  2. The build system will automatically compile MicroPython"
echo ""