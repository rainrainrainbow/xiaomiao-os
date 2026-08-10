# ================ 示例 .app: 像素鸟 (com.demo.pixel) ================
#
#  演示: 像素绘制 + 简单物理 (重力/跳跃)
#
{"id": "com.demo.pixel", "name": "像素鸟", "glyph": "🕹", "version": "1.3.0", "permissions": ["display","buttons","buzzer"]}

import hal

# 黑屏
hal.display_fill(0x0000)

bird_y = 64
vy = 0
g = 2     # 重力
score = 0

for frame in range(200):
    c = hal.buttons_peek()
    if c == 'B':
        hal.sys_exit()
    if c == 'A':
        vy = -8      # 跳跃
        hal.buzzer_beep(2000, 30)

    # 物理
    vy += g
    bird_y += vy
    if bird_y > 120: bird_y = 120
    if bird_y < 0: bird_y = 0

    # 全屏擦黑
    hal.display_fill(0x0000)
    # 鸟是一个 8x8 黄色方块
    for dy in range(0, 8):
        for dx in range(0, 8):
            hal.display_pixel(40 + dx, int(bird_y) + dy, 0xFFE0)
    hal.display_text(120, 4, str(frame))
    hal.time_sleep_ms(60)