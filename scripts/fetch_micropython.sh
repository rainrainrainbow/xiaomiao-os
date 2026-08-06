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

# Generate MicroPython build headers (qstrdefs.generated.h, etc.)
echo "Generating MicroPython build headers..."
cd "$MICROPYTHON_DIR"
mkdir -p py/genhdr
python3 py/makeqstrdefs.py py/modules.cmake py/genhdr/qstrdefs.generated.h 2>/dev/null || \
    python3 py/makeqstrdefs.py py/modules.cmake py/genhdr/qstrdefs.generated.h 2>&1 | tail -5
# Fallback: create minimal headers needed for compilation
if [ ! -f py/genhdr/qstrdefs.generated.h ]; then
    echo "Creating minimal qstrdefs.generated.h..."
    echo "// Auto-generated" > py/genhdr/qstrdefs.generated.h
fi
# Generate mpversion.h (needed by some modules)
python3 py/mkversionheader.py py/genhdr/mpversion.h 2>/dev/null || true
# Generate compiler.h
echo "// Auto-generated compiler features" > py/genhdr/compiler.h
echo "#define MICROPY_COMP_CONST (1)" >> py/genhdr/compiler.h
echo "#define MICROPY_COMP_DOUBLE_TUPLE (1)" >> py/genhdr/compiler.h
echo "#define MICROPY_COMP_TRIPLE_TUPLE (1)" >> py/genhdr/compiler.h
echo "#define MICROPY_COMP_RETURN_IF_EXPR (1)" >> py/genhdr/compiler.h
# Generate nimgc.h
echo "// Auto-generated" > py/genhdr/nimgc.h
# Generate moduledefs.h placeholder
echo "// Auto-generated" > py/genhdr/moduledefs.h
# Generate qstr.i.last (needed by some build systems)
touch py/genhdr/qstr.i.last
echo "Done generating headers."

echo ""
echo "Next steps:"
echo "  1. Build XiaoMiao OS: ./build.sh"
echo "  2. The build system will automatically compile MicroPython"
echo ""