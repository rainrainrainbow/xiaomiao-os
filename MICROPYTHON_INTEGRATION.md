# MicroPython 集成说明

本文档详细说明 XiaoMiao OS 的 MicroPython 集成实现。

## 架构概览

```
┌─────────────────────────────────────────────────────────┐
│                    应用层 (.app 包)                      │
│  main.py + manifest.json + icon.png + lib/              │
├─────────────────────────────────────────────────────────┤
│              MicroPython VM 运行时                       │
│  micropython_runtime.c                                  │
│  - ZIP 解压 (miniz)                                     │
│  - GC 堆管理 (256KB PSRAM)                              │
│  - sys.path 配置                                        │
│  - main.py 执行                                         │
├─────────────────────────────────────────────────────────┤
│              硬件 API 绑定模块                           │
│  xiaomiao_modules.c                                     │
│  - lcd: clear/text/rect/fill_rect                       │
│  - buttons: read/wait                                   │
│  - buzzer: beep                                         │
│  - power: voltage/percent                               │
│  - time: ms/sleep                                       │
├─────────────────────────────────────────────────────────┤
│              ESP-IDF 组件层                              │
│  components/micropython/                                │
│  - MicroPython v1.22.0 核心                             │
│  - 自定义模块注册                                       │
└─────────────────────────────────────────────────────────┘
```

## 自动拉取机制

### 本地构建
```bash
./build.sh
```
脚本会自动检测 MicroPython 源码是否存在，若不存在则调用 `scripts/fetch_micropython.sh` 拉取。

### CI 构建
GitHub Actions 工作流 (`.github/workflows/build.yml`) 在编译前自动执行：
```yaml
- name: Fetch MicroPython
  run: |
    chmod +x scripts/fetch_micropython.sh
    ./scripts/fetch_micropython.sh
```

## 条件编译

`main/CMakeLists.txt` 检测 MicroPython 组件是否存在：
```cmake
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../components/micropython/micropython/py/micropython.mk")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE XIAOMIAO_HAS_MICROPYTHON=1)
    message(STATUS "MicroPython component found - enabling full VM support")
else()
    message(STATUS "MicroPython component not found - building in stub mode")
endif()
```

当 `XIAOMIAO_HAS_MICROPYTHON` 定义时，`micropython_runtime.c` 编译完整 VM 支持；否则编译为 stub 模式。

## VM 生命周期

### 1. 初始化
```c
micropython_init()
├── 创建 /tmp 目录
├── 标记 initialized = true
└── 日志输出模式 (full/stub)
```

### 2. 启动应用
```c
micropython_launch_app(app)
├── extract_app()
│   ├── miniz 解压 .app (ZIP)
│   ├── 创建 /tmp/<package>/ 目录
│   └── 验证 manifest.json 存在
├── mpy_vm_init()
│   ├── 分配 256KB GC 堆 (PSRAM 优先)
│   ├── mp_stack_ctrl_init()
│   ├── gc_init() + mp_init()
│   ├── 配置 sys.path (app_dir + lib/)
│   └── xiaomiao_modules_register()
├── mpy_vm_run_main()
│   └── pyexec_file("main.py")
├── mpy_vm_deinit()
│   ├── mp_deinit()
│   └── free(gc_heap)
└── exit_cb(package_name, exit_code)
```

### 3. 停止应用
```c
micropython_stop_app()
├── mpy_vm_deinit()
└── 重置运行状态
```

## 硬件 API 使用示例

### LCD 显示
```python
import lcd

# 清屏 (黑色)
lcd.clear(0x0000)

# 显示文本 (x, y, text, color)
lcd.text(10, 20, "Hello!", 0xFFFF)

# 绘制矩形 (x, y, w, h, color)
lcd.rect(5, 5, 50, 30, 0xF800)

# 填充矩形
lcd.fill_rect(60, 5, 50, 30, 0x001F)
```

### 按键输入
```python
import buttons

# 读取所有按键状态
state = buttons.read()
if state['A']:
    print("A pressed")

# 等待任意按键按下
key = buttons.wait()
print(f"Pressed: {key}")
```

### 蜂鸣器
```python
import buzzer

# 播放音调 (频率Hz, 持续时间ms)
buzzer.beep(880, 200)  # A5 音符
```

### 电源状态
```python
import power

v = power.voltage()  # 电池电压 (float)
p = power.percent()  # 电量百分比 (int 0-100)
print(f"Battery: {v:.2f}V ({p}%)")
```

### 时间控制
```python
import time

ms = time.ms()      # 系统启动以来的毫秒数
time.sleep(1000)    # 延时 1000ms
```

## 应用包结构

```
my-app.app (ZIP)
├── manifest.json
├── main.py
├── icon.png (可选)
└── lib/ (可选)
    └── my_module.py
```

### manifest.json
```json
{
  "id": "com.example.myapp",
  "name": "My App",
  "version": "1.0.0",
  "icon": "🎮",
  "entry": "main.py"
}
```

### main.py 示例
```python
import lcd
import buttons
import time

# 初始化
lcd.clear(0x0000)
lcd.text(10, 10, "My App", 0xFFFF)

# 主循环
while True:
    key = buttons.wait()
    if key == 'A':
        lcd.text(10, 30, "A pressed!", 0x07E0)
    elif key == 'B':
        break  # 退出应用
    
    time.sleep(100)
```

## 内存管理

- **GC 堆**: 256KB，优先从 PSRAM 分配
- **栈大小**: 8KB
- **分配策略**: 
  1. 尝试 `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)`
  2. 失败则回退到 `MALLOC_CAP_INTERNAL`

## 构建产物

启用 MicroPython 后，固件大小约增加：
- Flash: +800KB (MicroPython 核心 + 模块)
- RAM: +256KB (GC 堆，运行时从 PSRAM 分配)

## 故障排查

### 构建失败：MicroPython 未找到
```bash
# 手动拉取
./scripts/fetch_micropython.sh

# 检查目录
ls components/micropython/micropython/py/
```

### 运行时崩溃：GC 堆分配失败
- 检查 PSRAM 是否正确配置 (`sdkconfig.defaults`)
- 减小 `MPY_HEAP_SIZE` (micropython_runtime.c)

### 应用无法启动
- 检查 .app 文件是否为有效 ZIP
- 确认 manifest.json 存在且格式正确
- 查看串口日志：`idf.py monitor`

## 下一步计划

1. **中文字体支持**: 集成 12×12 或 16×16 CJK 点阵字体
2. **更多硬件 API**: 
   - IMU (MPU6050) 加速度计读取
   - GD32 LED/电机控制
   - SD 卡文件操作
3. **图形库**: 集成 framebuffer 和简单图形原语
4. **网络支持**: WiFi 连接 + HTTP 客户端 (用于在线应用商店)
