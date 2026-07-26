#!/bin/bash
# tools/gen_example_app.sh - 生成示例 App 目录结构
# 然后打包为 .app

set -e

mkdir -p hello_xiaomiao/{lib,assets}

# manifest.json
cat > hello_xiaomiao/manifest.json << 'EOF'
{
  "package_name": "com.xiaomiao.hello",
  "version": "1.0.0",
  "display_name": "你好世界",
  "description": "示例应用 - 展示全部系统 API",
  "author": "XiaoMiao Team",
  "entry_point": "main.py",
  "icon": "icon.png",
  "permissions": ["storage", "display", "sensor", "music"],
  "required_api_version": "1.0"
}
EOF

# main.py
cat > hello_xiaomiao/main.py << 'PYEOF'
import xiaomiao as xm
import time

xm.screen.clear()
xm.screen.text("Hello XiaoMiao!", x=10, y=10, color=0xFFFF)
xm.screen.rect(5, 30, 100, 40, color=0x00A6FF)

v = xm.sensor.battery()
print(f"Battery: {v:.2f}V")

xm.screen.text("Press A to start", x=10, y=50, color=0x7FD858)
xm.key.wait_press()

for i in range(20):
    xm.screen.clear()
    ax = xm.sensor.acc_x()
    ay = xm.sensor.acc_y()
    xm.screen.text(f"Acc: {ax},{ay}", x=5, y=10, color=0xFFFF)
    v = xm.sensor.battery()
    xm.screen.text(f"Battery: {v:.2f}V", x=5, y=25, color=0x00A6FF)
    if xm.key.a_pressed():
        xm.music.tone(440, 0.1)
    if xm.key.b_pressed():
        break
    if abs(ax) > 20000:
        xm.screen.text("SHAKE!", x=40, y=50, color=0xE81123)
        xm.music.tone(880, 0.2)
    time.sleep(0.1)

xm.screen.clear()
xm.screen.text("Goodbye!", x=30, y=50, color=0xFFFF)
xm.music.tone(523, 0.3)
time.sleep(0.5)
xm.music.tone(440, 0.3)
time.sleep(0.5)
xm.music.tone(349, 0.5)
print("App finished")
PYEOF

# 生成占位 icon (1x1 PNG)
echo "Generating icon..."
python3 -c "
import struct, zlib
def make_png():
    # 32x32 solid color PNG
    w, h = 32, 32
    color = (0, 166, 255)
    raw = b''
    for y in range(h):
        raw += b'\x00'  # filter byte
        for x in range(w):
            raw += bytes(color)
    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t+d) & 0xffffffff)
    sig = b'\x89PNG\r\n\x1a\n'
    ihdr = struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)
    idat = zlib.compress(raw)
    png = sig + chunk(b'IHDR', ihdr) + chunk(b'IDAT', idat) + chunk(b'IEND', b'')
    with open('hello_xiaomiao/icon.png', 'wb') as f:
        f.write(png)
make_png()
" 2>/dev/null || echo "PNG gen failed - icon optional"

# 打包
echo ""
echo "Packing..."
cd hello_xiaomiao
zip -r ../hello_xiaomiao.app manifest.json main.py icon.png 2>/dev/null || true
cd ..

SIZE=$(stat -c%s hello_xiaomiao.app 2>/dev/null || echo 0)
echo ""
echo "============================================"
echo "  App created: hello_xiaomiao.app ($SIZE bytes)"
echo "============================================"
echo "  Install: cp hello_xiaomiao.app /sdcard/apps/"
echo ""
echo "  Contents:"
unzip -l hello_xiaomiao.app
echo "============================================"
