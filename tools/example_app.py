#!/usr/bin/env python3
"""
example_app.py - 示例 MicroPython App

演示 xiaomiao 系统模块的全部 API。
此文件可直接在 XiaoMiao OS 上运行。

安装方式:
  1. 打包为 .app:
     zip -r my_app.app manifest.json main.py icon.png
  2. 拷贝到 SD 卡 /sdcard/apps/
  3. 在桌面上即可看到图标
"""

# 导入 XiaoMiao 系统模块
import xiaomiao as xm
import time

# === 显示 ===
xm.screen.clear()
xm.screen.text("Hello XiaoMiao!", x=10, y=10, color=0xFFFF)
xm.screen.rect(5, 30, 100, 40, color=0x00A6FF)

# === 电池检测 ===
v = xm.sensor.battery()
print(f"Battery: {v:.2f}V")

# === 按键检测 ===
xm.screen.text("Press A to start", x=10, y=50, color=0x7FD858)
xm.key.wait_press()

# === 循环 + 传感器 ===
for i in range(20):
    xm.screen.clear()
    
    # 显示加速度
    ax = xm.sensor.acc_x()
    ay = xm.sensor.acc_y()
    xm.screen.text(f"Acc: {ax},{ay}", x=5, y=10, color=0xFFFF)
    
    # 显示电量
    v = xm.sensor.battery()
    xm.screen.text(f"Battery: {v:.2f}V", x=5, y=25, color=0x00A6FF)
    
    # 按键检测
    if xm.key.a_pressed():
        xm.music.tone(440, 0.1)
    
    if xm.key.b_pressed():
        break
    
    # 摇一摇检测
    if abs(ax) > 20000:
        xm.screen.text("SHAKE!", x=40, y=50, color=0xE81123)
        xm.music.tone(880, 0.2)
    
    time.sleep(0.1)

# === 结束 ===
xm.screen.clear()
xm.screen.text("Goodbye!", x=30, y=50, color=0xFFFF)
xm.music.tone(523, 0.3)
time.sleep(0.5)
xm.music.tone(440, 0.3)
time.sleep(0.5)
xm.music.tone(349, 0.5)

print("App finished")
