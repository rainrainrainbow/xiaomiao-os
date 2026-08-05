# XiaoMiao OS

> 小喵掌机 MicroPython 桌面系统

为小喵（学而思）教育掌机打造的完整桌面操作系统，支持安装和运行基于 MicroPython 开发的应用程序。

## 硬件规格

| 组件 | 规格 |
|------|------|
| MCU | ESP32-WROVER-B (4MB Flash + 8MB PSRAM) |
| 协处理器 | GD32F350G8 (LED/电机控制) |
| 屏幕 | ST7735 160×128 TFT (SPI2 @ 60MHz) |
| 存储 | MicroSD (FAT32) + SPIFFS (128KB) |
| 输入 | 6键手柄 (UP/DOWN/LEFT/RIGHT/A/B) |
| 电池 | 3.7V LiPo (ADC 采样，分压比 9.1k/2.4k) |
| 蜂鸣器 | GPIO14 (LEDC PWM) |
| IMU | MPU6050 (I2C) |

## 系统架构

```
┌─────────────────────────────────────────────────────────┐
│                    应用层 (Application)                  │
│  .app 包 (ZIP) → main.py + manifest.json + icon.png    │
├─────────────────────────────────────────────────────────┤
│              MicroPython 运行时层                        │
│  micropython_runtime.c → VM 初始化 / 应用加载 / 生命周期 │
├─────────────────────────────────────────────────────────┤
│              桌面 UI 框架层                              │
│  LVGL 9.x → 桌面 / 应用列表 / 设置 / 商店 / 运行视图    │
├─────────────────────────────────────────────────────────┤
│              内核 / 服务层                               │
│  app_manager / app_store / power / 应用注册表            │
├─────────────────────────────────────────────────────────┤
│              HAL 层 (硬件抽象)                           │
│  ST7735 LCD / SD Card / SPIFFS / I2C / 按键 / 蜂鸣器   │
└─────────────────────────────────────────────────────────┘
```

## 项目结构

```
xiaomiao-os/
├── main/
│   ├── main.c                    # 入口 + 硬件初始化 + 主循环
│   ├── app_manager.h/c           # 应用注册表 + ZIP/JSON 解析
│   ├── app_store.h/c             # 应用商店 PoC
│   ├── power.h/c                 # 电源管理 (ADC + 滤波 + 低电检测)
│   ├── micropython_runtime.h/c   # MicroPython VM 集成 (条件编译)
│   ├── CMakeLists.txt            # 组件构建配置
│   └── ui/
│       ├── ui_main.h/c           # UI 管理器 (页面切换 + 事件分发)
│       ├── ui_desktop.h/c        # 桌面 3×2 图标网格
│       ├── ui_applist.h/c        # 应用列表 (垂直滚动)
│       ├── ui_settings.h/c       # 设置页面
│       ├── ui_appstore.h/c       # 应用商店页面
│       ├── ui_apprun.h/c         # 应用运行视图
│       ├── ui_statusbar.h/c      # 状态栏 (时间 + 电量)
│       └── ui_theme.h            # 主题色常量
├── components/
│   └── micropython/              # MicroPython ESP-IDF 组件
│       ├── CMakeLists.txt        # 组件构建配置
│       ├── idf_component.yml     # 组件依赖声明
│       └── modules/xiaomiao/     # 硬件 API 绑定模块
│           ├── xiaomiao_modules.h/c  # lcd/buttons/buzzer/power/time
├── scripts/
│   └── fetch_micropython.sh      # 自动拉取 MicroPython 源码
├── examples/
│   ├── hello-world/              # 示例应用：Hello World
│   │   ├── manifest.json
│   │   └── main.py
│   └── counter/                  # 示例应用：计数器
│       ├── manifest.json
│       └── main.py
├── partitions.csv                # Loader 兼容分区表
├── sdkconfig.defaults            # ESP-IDF 默认配置
├── return_to_loader.h            # 返回 Loader 机制
├── build.sh                      # 构建脚本 (自动拉取 MicroPython)
└── .github/workflows/build.yml   # CI 配置 (自动拉取 + 编译)
```

## 分区表

| 名称 | 类型 | 偏移 | 大小 | 说明 |
|------|------|------|------|------|
| factory | app | 0x10000 | 696KB | Loader 固件 |
| ota_0 | app | 0xBE000 | 3.2MB | XiaoMiao OS |
| vfs | data | 0x4EE000 | 128KB | SPIFFS 持久化 |

## 应用包格式 (.app)

应用包为标准 ZIP 格式，包含：

```
my-app.app (ZIP)
├── manifest.json    # 应用清单
├── main.py          # 入口脚本
└── icon.png         # 图标 (可选)
```

**manifest.json 示例：**
```json
{
  "id": "com.example.hello",
  "name": "Hello World",
  "version": "1.0.0",
  "icon": "👋",
  "entry": "main.py"
}
```

## 编译指南

### 环境要求

- ESP-IDF v5.3+
- Python 3.8+
- CMake 3.16+

### 步骤

```bash
# 1. 安装 ESP-IDF
git clone -b v5.3 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32
source export.sh

# 2. 编译 XiaoMiao OS
cd xiaomiao-os
idf.py set-target esp32
idf.py build

# 3. 烧录
idf.py flash monitor
```

或使用构建脚本：
```bash
chmod +x build.sh
./build.sh
```

### 输出产物

- `build/merged.bin` — 完整固件 (可直接烧录)
- `build/xiaomiao-os.bin` — 应用固件
- `build/partition_table/partition-table.bin` — 分区表
- `build/bootloader/bootloader.bin` — 引导加载程序

## SD 卡目录结构

```
/sdcard/
├── apps/              # 已安装应用
│   ├── com.example.hello/
│   │   ├── manifest.json
│   │   ├── main.py
│   │   └── icon.png
│   └── com.example.counter/
│       └── ...
└── store/             # 应用商店源 (.app 文件)
    ├── hello-world.app
    └── counter.app
```

## 系统应用

| 包名 | 功能 |
|------|------|
| sys.desktop | 桌面 (3×2 图标网格) |
| sys.applist | 应用列表 |
| sys.settings | 设置 |
| sys.store | 应用商店 |
| sys.about | 关于 |

## 按键映射

| 物理按键 | GPIO | LVGL Key | 功能 |
|----------|------|----------|------|
| UP | GPIO2 | LV_KEY_UP | 上导航 |
| DOWN | GPIO13 | LV_KEY_DOWN | 下导航 |
| LEFT | GPIO27 | LV_KEY_LEFT | 左导航 |
| RIGHT | GPIO35 | LV_KEY_RIGHT | 右导航 |
| A | GPIO34 | LV_KEY_ENTER | 确认/启动 |
| B | GPIO12 | LV_KEY_ESC | 返回/退出 |

## 电源管理

- 电池 ADC: GPIO34 (ADC1_CH6)
- 分压电阻: 9.1kΩ / 2.4kΩ
- 计算公式: V_bat = V_adc × 4.791
- 百分比映射:
  - ≤3.0V → 0%
  - 3.7V → 50%
  - 4.2V → 100%
- 低电警告: ≤3.3V (蜂鸣器短响)
- 临界电量: ≤3.1V

## 待完成事项

- [x] MicroPython 集成 (ESP-IDF 组件 + 自动拉取 + VM 生命周期)
- [x] 硬件 API 绑定 (lcd/buttons/buzzer/power/time 模块)
- [x] ZIP 解压 (miniz 库支持 deflate)
- [ ] 中文字体 (精简 CJK 点阵字体)
- [ ] WiFi 应用商店 (在线下载)
- [ ] 积木编辑器 (图形化编程)

## License

MIT