# ================ 示例 .app: 贪吃蛇 (com.demo.snake) ================
#
#  文件格式:
#    第一行 = manifest JSON (包含 id/name/version/permissions)
#    第二行 = 空行
#    后面 = main.py 源码 (MicroPython)
#
#  拷贝到 SD 卡: /sdcard/roms/snake.app
#
{"id": "com.demo.snake", "name": "贪吃蛇", "glyph": "🐍", "version": "1.0.2", "permissions": ["display","buttons","buzzer"]}

import hal, time

# 清屏（黄色背景）
hal.display_fill(0xF6D4)
hal.display_text(20, 10, 'Snake Ready')

# 简单循环: 按 B 退出, 按其它按键蜂鸣
while True:
    c = hal.buttons_peek()
    if c == 'B':
        hal.sys_exit()
    if c:
        hal.buzzer_beep(800, 20)
    hal.time_sleep_ms(80)