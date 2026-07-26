"""
时钟应用 - XiaoMiao OS
显示当前时间和日期
"""

import time
import display

def main():
    """时钟应用主函数"""
    display.clear(0x0000)  # 黑色背景
    
    while True:
        # 获取当前时间
        t = time.localtime()
        hour = t[3]
        minute = t[4]
        second = t[5]
        year = t[0]
        month = t[1]
        day = t[2]
        
        # 显示时间 (大字体)
        time_str = f"{hour:02d}:{minute:02d}:{second:02d}"
        display.text(time_str, 20, 30, 0xFFFF, 3)  # 白色，3号字体
        
        # 显示日期
        date_str = f"{year}-{month:02d}-{day:02d}"
        display.text(date_str, 40, 70, 0x07FF, 2)  # 青色，2号字体
        
        # 刷新显示
        display.refresh()
        
        # 每秒更新
        time.sleep(1)

if __name__ == "__main__":
    main()