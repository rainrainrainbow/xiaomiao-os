# XiaoMiao OS - MicroPython 桌面系统

小喵掌机上的完整桌面操作系统，支持安装和运行基于 MicroPython 开发的应用程序。

## 🎯 项目目标

在学而思小喵教育掌机（ESP32-WROVER-B + GD32F350G8）上创建一个功能完整的桌面操作系统，提供类似 Windows Phone 或 Android 的交互体验。

## 📁 项目结构

```
xiaomiao-os/
├── CMakeLists.txt              # ESP-IDF 项目配置
├── sdkconfig.defaults          # 板级配置
├── partitions.csv              # 分区表（Loader兼容）
├── .gitignore
├── README.md                   # 本文档
├── ui-simulator.html           # UI 模拟器（HTML/CSS/JS）
└── main/
    ├── CMakeLists.txt          # 组件配置
    ├── idf_component.yml       # LVGL 9.5.0 依赖
    ├── return_to_loader.h      # 返回Loader集成
    ├── main.c                  # 主入口（硬件初始化+模块整合）
    ├── lvgl_port/              # LVGL 移植层
    │   ├── lvgl_port.h         # 显示驱动+按键输入+组合键
    │   └── lvgl_port.c
    ├── app_manager/            # 应用管理器
    │   ├── app_manager.h       # SD卡扫描+manifest解析
    │   └── app_manager.c
    ├── micropython/            # MicroPython 运行时
    │   ├── micropython_runtime.h
    │   └── micropython_runtime.c
    ├── config_manager/         # 配置管理器
    │   ├── config_manager.h    # JSON配置持久化
    │   └── config_manager.c
    ├── power_manager/          # 电源管理
    │   ├── power_manager.h     # 电池监测+自动休眠
    │   └── power_manager.c
    └── ui/                     # 桌面 UI
        ├── ui_main.h           # LVGL 界面系统
        └── ui_main.c
```

## 🔧 硬件规格

| 参数 | 值 |
|------|------|
| MCU | ESP32-WROVER-B |
| Flash | 4MB QIO 80MHz |
| PSRAM | 8MB Quad 80MHz |
| LCD | ST7735 SPI TFT 160×128 |
| SD卡 | MicroSD (共享SPI2) |
| 按键 | 6键手柄 (UP/DOWN/LEFT/RIGHT/A/B) |
| I2C | SCL=15, SDA=21 @100kHz |
| 蜂鸣器 | GPIO14 (LEDC PWM) |
| 电池ADC | GPIO34 (9.1k/2.4k分压) |

### 引脚定义

```
LCD:  SCLK=18  MOSI=23  MISO=19  CS=5  DC=4   (SPI2 @ 40MHz)
SD:   CS=22    (共享 SPI2)
Keys: UP=2  DOWN=13  LEFT=27  RIGHT=35  A=34  B=12  (低电平有效)
I2C:  SCL=15  SDA=21  (100kHz)
Buzz: GPIO14 (LEDC Timer0/Ch0)
Batt: GPIO34 (ADC1_CH6, 带分压电阻)
```

## 🎮 操作说明

### 硬件按键

| 按键 | 功能 |
|------|------|
| UP/DOWN/LEFT/RIGHT | 方向导航 |
| A | 确认/进入 |
| B | 返回上一页 |

### 组合键

| 组合 | 功能 |
|------|------|
| UP + B | 返回主页 |
| DOWN + B | 打开任务管理器 |

### 任务管理器

| 操作 | 按键 |
|------|------|
| 选择应用 | UP/DOWN |
| 进入应用 | A |
| 锁定/解锁 | LEFT |
| 销毁应用 | RIGHT |
| 返回 | B |

## 🚀 快速开始

### 1. 环境准备

```bash
# 安装 ESP-IDF v5.5.4
mkdir -p ~/esp
cd ~/esp
git clone -b v5.5.4 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32

# 激活环境
. ~/esp/esp-idf/export.sh
```

### 2. 构建项目

```bash
cd xiaomiao-os
idf.py build
```

### 3. 烧录固件

```bash
# 通过 USB (GD32 UART 桥)
idf.py -p /dev/ttyACM0 flash

# 或使用 esptool
esptool.py --chip esp32 -b 460800 write_flash 0x0 build/xiaomiao-os-merged.bin
```

### 4. 生成合并固件

```bash
idf.py merge-bin
cp build/xiaomiao-os-merged.bin /sdcard/roms/xiaomiao-os.bin
```

## 🎨 UI 模拟器

在浏览器中打开 `ui-simulator.html` 可以体验完整的 UI 交互：

```bash
# 直接用浏览器打开
open ui-simulator.html
# 或
xdg-open ui-simulator.html
```

### 键盘控制

| 按键 | 功能 |
|------|------|
| W/A/S/D | 方向键 (上/左/下/右) |
| J | A键 (确认) |
| K | B键 (返回) |
| Space | 主页 |
| L | 切换主页/应用列表 |

### 功能演示

- **启动动画**: 2.5秒 Logo 展示
- **桌面**: 3×2 应用图标网格 + 分页圆点
- **设置页面**: 主题选择、关于等
- **任务管理器**: 运行中的应用管理
- **主题系统**: 6种主题（默认/海洋/森林/星空/火焰/冰晶）

## 📦 应用包格式 (.app)

应用打包为 ZIP 文件，扩展名改为 `.app`：

```
com.example.myapp.app/
├── manifest.json          # 应用清单 (必需)
├── icon.png               # 图标 128x128 (必需)
├── main.py                # 入口脚本 (必需)
├── lib/                   # 私有库 (可选)
├── assets/                # 资源文件 (可选)
└── model/                 # ML模型 (可选)
```

### manifest.json 示例

```json
{
  "id": "com.example.myapp",
  "name": "我的应用",
  "version": "1.0.0",
  "author": "YourName",
  "description": "这是一个示例应用",
  "icon": "icon.png",
  "main": "main.py"
}
```

## 📊 分区表

| 分区 | 偏移 | 大小 | 用途 |
|------|------|------|------|
| nvs | 0xA000 | 20KB | 系统配置 |
| phy_init | 0xF000 | 4KB | PHY校准 |
| factory | 0x10000 | 568KB | Loader固件 |
| otadata | 0xBE000 | 8KB | 启动选择 |
| ota_0 | 0xC0000 | 3.25MB | ROM运行槽 |

## 🔋 电池检测

GPIO34 通过 9.1k/2.4k 分压电阻连接电池：

```
电池 (3.0-4.2V) ──┬── 9.1kΩ ──┬── GPIO34 (ADC)
                  │           │
                  └── 2.4kΩ ──┘
                              │
                             GND
```

分压比: (9.1 + 2.4) / 2.4 = 4.79

ADC 读数 × 4.79 = 实际电池电压

## 🏗️ 模块架构

### lvgl_port - LVGL 移植层

- **显示驱动**: ST7735 双缓冲 DMA 刷新
- **按键输入**: 6键 GPIO 扫描 + 去抖
- **组合键检测**: 500ms 时间窗口
- **按键事件分发**: 单键回调机制
- **LVGL v9 API**: `lv_display_create`, `lv_indev_create`

### app_manager - 应用管理器

- **SD卡扫描**: 自动检测 `/sdcard/apps/` 目录
- **manifest解析**: cJSON 解析应用信息
- **应用生命周期**: 启动/停止/状态管理

### config_manager - 配置管理器

- **JSON持久化**: 基于cJSON的配置存储
- **配置路径**: `/sdcard/.xiaomiao/config.json`
- **支持类型**: 整数、字符串、布尔值
- **自动保存**: 主题切换等配置自动持久化

### power_manager - 电源管理

- **电池监测**: ADC采样 + 分压计算 + 校准
- **电量计算**: 分段线性插值算法
- **低电量告警**: 可配置阈值（默认20%）
- **自动休眠**: 可配置超时时间（默认5分钟）
- **事件回调**: 低电量/临界/休眠/唤醒事件

### ui - 桌面界面

- **状态栏**: 时间 + 电池
- **应用网格**: 3×2 图标布局
- **页面指示器**: 底部圆点分页
- **主题系统**: 6种可切换主题（持久化）
- **任务管理器**: 运行应用管理
- **配置联动**: 主题选择自动保存

## ⚠️ 注意事项

1. **GPIO34/35**: 仅输入，无内部上拉，需外部上拉电阻
2. **SPI总线共享**: LCD和SD卡共享SPI2，通过CS区分
3. **PSRAM限制**: ESP32不能从PSRAM执行代码，仅用于数据存储
4. **Flash限制**: ROM镜像须 ≤ 3.25MB (ota_0分区)
5. **背光控制**: 背光硬接VCC，无法软件调节亮度

## 📝 开发计划

### 阶段一：环境搭建与基础验证 ✅
- [x] 项目骨架创建
- [x] 硬件初始化代码
- [x] Return-to-Loader集成

### 阶段二：UI设计与模拟 ✅
- [x] HTML/CSS/JS UI模拟器 v2
- [x] 3×2 桌面 + 分页圆点
- [x] 6种主题系统
- [x] 任务管理器（进入/锁定/销毁）

### 阶段三：C语言核心实现 ✅
- [x] LVGL v9 移植层（显示+输入+组合键）
- [x] 应用管理器（SD卡扫描+manifest解析）
- [x] MicroPython 运行时桩
- [x] 桌面 UI（状态栏+网格+页面指示器）
- [x] 设置界面 + 主题选择
- [x] 任务管理器界面
- [x] 按键导航系统

### 阶段四：系统功能完善 ✅
- [x] MicroPython 运行时框架（任务执行+脚本管理）
- [x] JSON配置持久化（config_manager模块）
- [x] 电源管理（电池监测+自动休眠）
- [x] 主题持久化（配置联动）
- [ ] 应用安装/卸载（待阶段五实现）

### 阶段五：编译与交付 ✅
- [x] GitHub Action配置（CI/CD自动构建）
- [x] 修复ADC资源冲突（统一由power_manager管理）
- [x] 完善LVGL字体配置（添加montserrat_8/28）
- [ ] 最终测试（需ESP-IDF环境）
- [ ] merged.bin交付（CI自动生成）

## 📄 许可证

MIT License

## 🙏 致谢

- [xiaomiao-firmware](https://github.com/jsfaint/xiaomiao-firmware) - 硬件Skill模板
- [xiaomiao-loader](https://github.com/jsfaint/xiaomiao-loader) - ROM Loader参考
- [xueersi-idf](https://github.com/ZyoungInc/xueersi-idf) - 硬件资料与逆向成果