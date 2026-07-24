# XiaoMiao OS 应用开发指南

XiaoMiao OS 支持使用 MicroPython 开发应用程序。应用以 `.app` 包的形式分发，可以轻松安装和卸载。

## 应用包结构

每个应用是一个包含以下文件的目录（打包为 ZIP 后改扩展名为 `.app`）：

```
myapp.app/
├── manifest.json    # 应用清单（必需）
├── main.py          # 入口脚本（必需）
├── icon.png         # 应用图标 128x128（推荐）
├── lib/             # 私有库（可选）
├── assets/          # 资源文件（可选）
└── README.md        # 应用说明（可选）
```

## manifest.json 格式

```json
{
  "id": "com.example.myapp",
  "name": "我的应用",
  "version": "1.0.0",
  "author": "Your Name",
  "description": "应用简介",
  "icon": "icon.png",
  "main": "main.py"
}
```

### 字段说明

| 字段 | 必需 | 说明 |
|------|------|------|
| id | ✅ | 应用唯一标识符（建议使用反向域名格式） |
| name | ✅ | 应用显示名称 |
| version | ✅ | 版本号（语义化版本） |
| author | ✅ | 作者名称 |
| description | ❌ | 应用简介 |
| icon | ❌ | 图标文件路径（相对于应用根目录） |
| main | ✅ | 入口脚本路径 |

## API 参考

### display 模块

显示控制模块，提供图形绘制功能。

#### 基本操作

```python
import display

# 清屏（填充指定颜色）
display.clear(color=0x0000)  # RGB565 颜色

# 刷新显示（必须调用，否则不会显示）
display.refresh()
```

#### 文本绘制

```python
# 绘制文本
display.text(text, x, y, color, size)
# text: 字符串
# x, y: 坐标
# color: RGB565 颜色
# size: 字体大小 (1=小, 2=中, 3=大)

# 示例
display.text("Hello", 10, 20, 0xFFFF, 2)  # 白色，中等字体
```

#### 图形绘制

```python
# 绘制像素点
display.pixel(x, y, color)

# 绘制线条
display.line(x1, y1, x2, y2, color)

# 绘制矩形
display.rect(x, y, width, height, color)

# 绘制填充矩形
display.fill_rect(x, y, width, height, color)

# 绘制圆形
display.circle(x, y, radius, color)

# 绘制填充圆形
display.fill_circle(x, y, radius, color)
```

### buttons 模块

按键输入模块，提供非阻塞式按键检测。

```python
import buttons

# 轮询按键（非阻塞）
btn = buttons.poll()

# 按键常量
buttons.UP     # 上键
buttons.DOWN   # 下键
buttons.LEFT   # 左键
buttons.RIGHT  # 右键
buttons.A      # A键（确认）
buttons.B      # B键（返回）

# 返回值
# None: 无按键
# buttons.UP/DOWN/LEFT/RIGHT/A/B: 对应按键
```

### 示例代码

#### 简单应用

```python
import time
import display

def main():
    display.clear(0x0000)
    display.text("Hello World!", 30, 50, 0xFFFF, 2)
    display.refresh()
    
    while True:
        time.sleep(0.1)

if __name__ == "__main__":
    main()
```

#### 按键交互

```python
import time
import display
import buttons

def main():
    counter = 0
    
    while True:
        btn = buttons.poll()
        
        if btn == buttons.UP:
            counter += 1
        elif btn == buttons.DOWN:
            counter -= 1
        elif btn == buttons.B:
            break  # 退出应用
        
        display.clear(0x0000)
        display.text(f"Count: {counter}", 40, 50, 0xFFFF, 2)
        display.text("UP/DOWN to change", 20, 80, 0x07FF, 1)
        display.text("B to exit", 40, 100, 0x07FF, 1)
        display.refresh()
        
        time.sleep(0.1)

if __name__ == "__main__":
    main()
```

## 颜色参考（RGB565）

| 颜色 | 值 | 说明 |
|------|------|------|
| 黑色 | 0x0000 | |
| 白色 | 0xFFFF | |
| 红色 | 0xF800 | |
| 绿色 | 0x07E0 | |
| 蓝色 | 0x001F | |
| 黄色 | 0xFFE0 | |
| 青色 | 0x07FF | |
| 粉色 | 0xF81F | |

## 屏幕规格

- 分辨率：160 × 128 像素
- 色深：16位 RGB565
- 状态栏高度：12 像素（系统保留）
- 可用区域：160 × 116 像素

## 应用安装

1. 将应用目录打包为 ZIP 文件
2. 重命名扩展名为 `.app`
3. 复制到 SD 卡的 `/apps/` 目录
4. 重启系统或在桌面刷新应用列表

## 最佳实践

1. **性能优化**：减少 `display.refresh()` 调用频率，避免频繁刷新
2. **内存管理**：及时释放不用的资源，避免内存泄漏
3. **按键响应**：使用非阻塞式 `buttons.poll()`，避免阻塞主循环
4. **退出处理**：监听 B 键，允许用户退出应用
5. **边界检测**：绘制前检查坐标是否在屏幕范围内

## 调试技巧

1. 使用 `print()` 输出调试信息到串口
2. 在桌面按 `DOWN + B` 打开任务管理器查看运行状态
3. 使用 `UP + B` 组合键强制返回主页

## 示例应用

参考 `examples/` 目录下的示例应用：

- `hello.app` - 最简单的 Hello World 应用
- `clock.app` - 时钟应用，演示时间获取和显示
- `snake.app` - 贪吃蛇游戏，演示完整的游戏逻辑

## 发布应用

1. 确保 `manifest.json` 信息完整
2. 添加清晰的 `icon.png` 图标
3. 编写 `README.md` 说明文档
4. 测试所有功能正常工作
5. 打包为 `.app` 文件分发

## 常见问题

**Q: 应用无法启动？**
A: 检查 `manifest.json` 格式是否正确，`main.py` 是否存在。

**Q: 显示不刷新？**
A: 确保调用了 `display.refresh()`。

**Q: 按键无响应？**
A: 检查是否在循环中调用了 `buttons.poll()`，避免使用阻塞式延时。

**Q: 内存不足？**
A: 减少大对象的使用，及时释放资源，避免在循环中创建对象。

## 技术支持

- GitHub Issues: https://github.com/rainrainrainbow/xiaomiao-os/issues
- 社区论坛: （待建立）

---

Happy Coding! 🐱