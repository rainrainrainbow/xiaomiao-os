"""
Hello World 应用 - XiaoMiao OS
简单的示例应用，显示 Hello World 文本
"""

import time
import display

def main():
    """Hello World 主函数"""
    display.clear(0x0000)  # 黑色背景
    
    # 显示标题
    display.text("Hello", 50, 40, 0xFFFF, 2)  # 白色，2号字体
    display.text("World!", 45, 65, 0xF81F, 2)  # 粉色，2号字体
    
    # 显示提示
    display.text("Press B to exit", 30, 100, 0x07FF, 1)  # 青色，1号字体
    
    # 刷新显示
    display.refresh()
    
    # 保持运行，等待用户按 B 键退出
    while True:
        time.sleep(0.1)

if __name__ == "__main__":
    main()