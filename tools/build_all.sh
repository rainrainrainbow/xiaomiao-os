#!/bin/bash
# tools/build_all.sh - 完整构建脚本
# 处理 MicroPython 子模块 + ESP-IDF 构建
set -e

echo "============================================"
echo "  XiaoMiao OS - Full Build Script"
echo "============================================"

# 检查 ESP-IDF
if [ -z "$IDF_PATH" ]; then
    echo "[!] IDF_PATH not set. Run: . \$HOME/esp/esp-idf/export.sh"
    echo "[!] Or: source /opt/esp/idf/export.sh"
    exit 1
fi
echo "[1] ESP-IDF: $IDF_PATH ($(idf.py --version 2>/dev/null || echo 'check failed'))"

# 检查 MicroPython 子模块
echo ""
echo "[2] Checking MicroPython submodule..."
if [ ! -f "micropython/py/mpconfig.h" ]; then
    echo "    MicroPython source not found in micropython/"
    echo "    Attempting to clone..."
    if command -v git >/dev/null 2>&1; then
        git clone --depth 1 --branch v1.24.0 https://github.com/micropython/micropython.git micropython/ || {
            echo "    [!] Clone failed. Building in LITE mode (no MP execution)."
            echo "    [!] MP code will be logged but not executed."
        }
        # 初始化子模块
        if [ -d "micropython/lib" ]; then
            (cd micropython && git submodule update --init --depth 1 lib/pystack || true)
        fi
    else
        echo "    [!] git not available. Building in LITE mode."
    fi
else
    echo "    Found: micropython/py/mpconfig.h"
fi

# 配置
echo ""
echo "[3] Configuring project..."
idf.py set-target esp32 2>&1 | tail -5

# 构建
echo ""
echo "[4] Building (this may take 5-10 minutes)..."
idf.py build 2>&1 | tail -20

# 合并
echo ""
echo "[5] Merging binary..."
idf.py merge-bin -o build/xiaomiao-os-merged.bin 2>&1 | tail -5

# 检查大小
SIZE=$(stat -c%s build/xiaomiao-os-merged.bin 2>/dev/null || echo 0)
MAX=$((2120 * 1024))
echo ""
echo "============================================"
echo "  Build Complete!"
echo "============================================"
echo "  File: build/xiaomiao-os-merged.bin"
echo "  Size: $SIZE / $MAX bytes"

if [ "$SIZE" -gt "$MAX" ]; then
    echo "  STATUS: ❌ EXCEEDS ota_0 partition!"
    exit 1
else
    PCT=$((SIZE * 100 / MAX))
    echo "  Status: ✅ OK ($PCT% of partition)"
fi

echo ""
echo "  Flash: esptool.py --chip esp32 -b 460800 write_flash 0x0 build/xiaomiao-os-merged.bin"
echo "  SD:    cp build/xiaomiao-os-merged.bin /sdcard/boot/xiaomiao-os.bin"
echo "============================================"
