#!/bin/bash
# tools/pack_app.sh - 打包 Python 脚本为 .app 安装包
#
# 用法: ./pack_app.sh <app_dir> <output.app>
# 示例: ./pack_app.sh my_app/ my_app.app

set -e

if [ $# -lt 2 ]; then
    echo "Usage: $0 <app_dir> <output.app>"
    echo ""
    echo "app_dir 必须包含:"
    echo "  manifest.json  - 应用清单"
    echo "  main.py        - 入口脚本"
    echo "  icon.png       - 图标 (可选)"
    echo "  lib/           - 私有库 (可选)"
    echo "  assets/        - 资源 (可选)"
    exit 1
fi

APP_DIR="$1"
OUTPUT="$2"

if [ ! -d "$APP_DIR" ]; then
    echo "Error: $APP_DIR not found"
    exit 1
fi

if [ ! -f "$APP_DIR/manifest.json" ]; then
    echo "Error: manifest.json not found in $APP_DIR"
    exit 1
fi

if [ ! -f "$APP_DIR/main.py" ]; then
    echo "Error: main.py not found in $APP_DIR"
    exit 1
fi

echo "Packing $APP_DIR -> $OUTPUT..."

# 用 zip 打包
cd "$APP_DIR"
rm -f "../$OUTPUT"
zip -r "../$OUTPUT" manifest.json main.py icon.png lib/ assets/ model/ 2>/dev/null || true

SIZE=$(stat -c%s "../$OUTPUT" 2>/dev/null || echo 0)
echo "Done! Size: $SIZE bytes"
echo ""
echo "Install: cp $OUTPUT /sdcard/apps/"
