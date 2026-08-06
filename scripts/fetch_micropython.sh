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

# Generate MicroPython build headers (qstrdefs.generated.h, root_pointers.h, etc.)
echo "Generating MicroPython build headers..."
cd "$MICROPYTHON_DIR"
mkdir -p py/genhdr

# Generate mpversion.h
python3 py/mkversionheader.py py/genhdr/mpversion.h 2>/dev/null || true

# Generate qstrdefs.generated.h (use makeqstrdefs.py or fallback)
python3 py/makeqstrdefs.py py/modules.cmake py/genhdr/qstrdefs.generated.h 2>/dev/null || true
# Fallback: create minimal qstrdefs.generated.h if makeqstrdefs.py fails
# (makeqstrdefs.py may fail because it needs modules.cmake which needs the port build system)
if [ ! -f py/genhdr/qstrdefs.generated.h ] || [ ! -s py/genhdr/qstrdefs.generated.h ]; then
    echo "Creating minimal qstrdefs.generated.h..."
    echo "#ifndef MICROPY_INCLUDED_PY_GENHDR_QSTRDEFS_GENERATED_H" > py/genhdr/qstrdefs.generated.h
    echo "#define MICROPY_INCLUDED_PY_GENHDR_QSTRDEFS_GENERATED_H" >> py/genhdr/qstrdefs.generated.h
    # Minimal header - just enough to compile (no Q() entries, just header guard)
    echo "// Auto-generated minimal qstrdefs" >> py/genhdr/qstrdefs.generated.h
    echo "#endif" >> py/genhdr/qstrdefs.generated.h
fi

# Generate root_pointers.h (needed by mpstate.h)
python3 -c "
import sys
# Scan all .c and .h files for MP_ROOT_POINTER declarations
# and generate root_pointers.h
import os, re
root_pointers = set()
for root, dirs, files in os.walk('.'):
    for f in files:
        if f.endswith(('.c', '.h')):
            path = os.path.join(root, f)
            try:
                with open(path, 'r') as fh:
                    for line in fh:
                        m = re.search(r'MP_ROOT_POINTER\s*\(\s*(\w+)', line)
                        if m:
                            root_pointers.add(m.group(1))
            except:
                pass
with open('py/genhdr/root_pointers.h', 'w') as f:
    for rp in sorted(root_pointers):
        f.write(f'MP_ROOT_POINTER({rp});\\n')
print(f'Generated root_pointers.h with {len(root_pointers)} entries')
" 2>/dev/null || echo "// Auto-generated" > py/genhdr/root_pointers.h

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

# Generate qstr.i.last
touch py/genhdr/qstr.i.last

echo "Done generating headers."
echo "Headers in py/genhdr:"
ls -la py/genhdr/

echo ""
echo "Next steps:"
echo "  1. Build XiaoMiao OS: ./build.sh"
echo "  2. The build system will automatically compile MicroPython"
echo ""