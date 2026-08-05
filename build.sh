#!/bin/bash
# XiaoMiao OS Build Script
# Requires ESP-IDF v5.x environment

set -e

echo "========================================="
echo "  XiaoMiao OS Build System"
echo "========================================="

# Check ESP-IDF environment
if [ -z "$IDF_PATH" ]; then
    echo "Error: ESP-IDF not found. Please run:"
    echo "  source \$IDF_PATH/export.sh"
    exit 1
fi

echo "ESP-IDF: $IDF_PATH"
echo "Target: esp32"

# Fetch MicroPython if not present
if [ ! -d "components/micropython/micropython/py" ]; then
    echo ""
    echo "MicroPython source not found. Fetching..."
    chmod +x scripts/fetch_micropython.sh
    ./scripts/fetch_micropython.sh
else
    echo "MicroPython source: present"
fi

# Set target
idf.py set-target esp32

# Build
echo ""
echo "Building XiaoMiao OS..."
idf.py build

if [ $? -eq 0 ]; then
    echo ""
    echo "========================================="
    echo "  Build Successful!"
    echo "========================================="
    echo ""
    echo "Firmware location:"
    echo "  build/merged.bin"
    echo ""
    echo "Flash commands:"
    echo "  idf.py flash           # Flash to device"
    echo "  idf.py monitor         # Serial monitor"
    echo "  idf.py flash monitor   # Flash + monitor"
    echo ""
else
    echo ""
    echo "Build failed!"
    exit 1
fi
