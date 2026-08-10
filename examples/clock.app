# ================ 示例 .app: 时钟 (com.demo.clock) ================
#
#  演示: 画电池图标 + 当前电池电量
#  需要 permissions: display, buttons, battery
#
{"id": "com.demo.clock", "name": "时钟", "glyph": "⏰", "version": "2.0.0", "permissions": ["display","buttons","battery"]}

import hal, time

# 启动时填绿色
hal.display_fill(0x0400)
hal.display_text(40, 50, 'Clock')
hal.display_text(20, 80, 'Press B quit')

t = 0
while True:
    if hal.buttons_peek() == 'B':
        hal.sys_exit()

    # 模拟数字时钟（用 sys_tick 而不是 wall clock, 因为 ESP32 无 RTC）
    hal.display_text(50, 30, '{:02d}:{:02d}'.format((t // 60) % 24, t % 60))
    t += 1
    hal.time_sleep_ms(1000)