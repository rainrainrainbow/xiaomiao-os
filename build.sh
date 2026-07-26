#!/bin/bash
# xiaomiao-os 一键构建脚本
set -e

echo "=========================================="
echo "  XiaoMiao OS Build Script"
echo "=========================================="

# 检查 ESP-IDF 环境
if [ -z "$IDF_PATH" ]; then
    echo "Error: IDF_PATH not set. Run: . \$HOME/esp/esp-idf/export.sh"
    exit 1
fi

echo ""
echo "[1/4] Cleaning previous build..."
rm -rf build/

echo ""
echo "[2/4] Configuring project..."
idf.py set-target esp32

echo ""
echo "[3/4] Building..."
idf.py build

echo ""
echo "[4/4] Merging binary..."
idf.py merge-bin -o build/xiaomiao-os-merged.bin

# 检查大小
SIZE=$(stat -c%s build/xiaomiao-os-merged.bin 2>/dev/null || stat -f%z build/xiaomiao-os-merged.bin)
MAX=$((2120 * 1024))  # ota_0 = 2120KB

echo ""
echo "=========================================="
echo "  Build Complete!"
echo "=========================================="
echo "  Merged bin: build/xiaomiao-os-merged.bin"
echo "  Size: $SIZE bytes"
if [ "$SIZE" -gt "$MAX" ]; then
    echo "  WARNING: Exceeds ota_0 partition size ($MAX bytes)!"
    exit 1
else
    echo "  OK: Within ota_0 partition limit"
fi
echo ""
echo "  Flash command:"
echo "    esptool.py --chip esp32 -b 460800 write_flash 0x0 build/xiaomiao-os-merged.bin"
echo ""
echo "  Or copy to SD card:"
echo "    cp build/xiaomiao-os-merged.bin /sdcard/boot/xiaomiao-os.bin"
echo "=========================================="
