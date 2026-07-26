# XiaoMiao OS

> 小喵（学而思）教育掌机 · 图形化编程桌面系统

基于 ESP32-WROVER-B + MicroPython 的完整桌面操作系统，支持积木式图形化编程。

## 硬件

- **主控**: ESP32-WROVER-B (4MB Flash / 8MB PSRAM)
- **协处理器**: GD32F350G8
- **屏幕**: ST7735 160×128 SPI TFT (旋转90°)
- **存储**: MicroSD (共享 SPI2)
- **输入**: 6 键 (上/下/左/右/A/B)
- **传感器**: MPU6050 六轴陀螺仪
- **音频**: 蜂鸣器 (LEDC)
- **电池检测**: ADC 分压 (9.1k/2.4k)

## 功能特性

### 桌面系统
- Windows Phone 风格磁贴界面
- 4×2 图标网格 + 分页
- 顶部状态栏（时间/电量）
- 设置中心（亮度/电池/存储/WiFi）

### 积木编辑器（形态二：菜单式编排）
- 6 大分类 × 15 块积木
  - ⚡ 事件: 开机/按键A/摇一摇
  - 🔀 控制: 如果/重复/循环/等待
  - 🖥️ 显示: 文字/矩形/清屏
  - ⌨️ 按键: 按键检测/等待按键
  - 📡 传感器: 加速度/陀螺仪/电池
  - 🔊 声音: 音符/停止
- 菜单式插入/删除/缩进
- 实时程序树预览
- 一键生成 MicroPython 代码

### MicroPython 运行时
- 嵌入式 MicroPython 解释器
- `xiaomiao` 系统模块:
  - `xm.screen` - 显示 (text/rect/clear)
  - `xm.key` - 按键 (a_pressed/b_pressed/direction)
  - `xm.sensor` - 传感器 (acc/battery/gyro/temp)
  - `xm.music` - 声音 (tone/stop/melody)
  - `xm.storage` - 存储 (write/read/exists)
  - `xm.system` - 系统 (exit/sleep/info/reboot)
  - `xm.time` - 时间 (sleep/ticks_ms)

### App 系统
- `.app` 格式 = ZIP 包
- manifest.json + main.py + icon + lib/ + assets/
- SD 卡安装/卸载
- 沙箱隔离执行

## 构建

### 本地构建

```bash
# 1. 安装 ESP-IDF v5.5.4
. $HOME/esp/esp-idf/export.sh

# 2. 克隆并构建
git clone --recursive <repo-url> xiaomiao-os
cd xiaomiao-os
./build.sh

# 3. 烧录
idf.py -p /dev/ttyACM0 flash
# 或拷贝到 SD 卡
cp build/xiaomiao-os-merged.bin /sdcard/boot/xiaomiao-os.bin
```

### GitHub Actions 自动构建

每次 push 到 main 分支会自动:
1. 安装 ESP-IDF v5.5.4
2. 拉取 LVGL 9.5.0 依赖
3. 编译项目
4. 合并为 `xiaomiao-os-merged.bin`
5. 上传产物（保留 30 天）

手动触发: 在 GitHub 仓库的 Actions 页面点击 "Run workflow"

## 项目结构

```
xiaomiao-os/
├── CMakeLists.txt           # 顶层 ESP-IDF 项目
├── sdkconfig.defaults       # 板级配置
├── partitions.csv           # Loader 兼容分区表
├── return_to_loader.h       # Loader 返回机制
├── build.sh                 # 一键构建脚本
├── README.md
├── .github/workflows/
│   └── build.yml            # GitHub Actions
├── micropython/             # MP 移植层
│   ├── CMakeLists.txt
│   └── port/
│       ├── mp_port_xiaomiao.c       # MP 适配层
│       ├── mp_module_xiaomiao_full.c # 完整 C 模块
│       └── CMakeLists.txt
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml
    ├── main.c               # 系统入口
    ├── hal/                 # 硬件驱动
    │   ├── lcd.c            # ST7735 @60MHz 三缓冲
    │   ├── sdcard.c         # MicroSD (SDSPI)
    │   ├── keys.c           # 6键扫描+消抖+ADC电池
    │   ├── backlight.c      # GPIO0 PWM
    │   ├── battery.c        # 分压公式
    │   ├── i2c_bus.c        # 100kHz I2C
    │   ├── gyro.c           # MPU6050
    │   └── buzzer.c         # LEDC 蜂鸣器
    ├── ui/                  # UI 框架
    │   ├── theme.c          # WP 磁贴配色
    │   ├── fonts.c          # 字体注册
    │   ├── components.c     # 通用组件
    │   └── page_manager.c   # 页面管理
    ├── desktop/             # 桌面系统
    │   ├── desktop.c        # 磁贴主界面
    │   ├── launcher.c       # 应用列表
    │   ├── statusbar.c      # 状态栏
    │   └── settings.c       # 设置页
    ├── block_editor/        # 积木编辑器
    │   ├── editor.c         # 主框架(三栏)
    │   ├── block_def.c      # 积木定义(6×15)
    │   ├── block_tree.c     # 程序树
    │   ├── codegen.c        # 代码生成
    │   ├── renderer.c       # 屏幕渲染
    │   └── input.c          # 按键输入
    ├── micropython/         # MP 引擎
    │   ├── mp_engine.c      # 执行接口
    │   └── mp_module_xiaomiao.c # 模块注册
    └── app_runtime/         # App 运行时
        ├── app_manager.c    # 扫描/安装/启动
        ├── app_format.c     # .app(ZIP)解析
        └── app_sandbox.c    # 沙箱隔离
```

## 分区表

| 分区 | 偏移 | 大小 | 用途 |
|------|------|------|------|
| nvs | 0xA000 | 20KB | 系统配置 |
| factory | 0x10000 | 568KB | Loader |
| ota_0 | 0xA0000 | 2120KB | **XiaoMiao OS** |
| ota_1 | 0x2C0000 | 1280KB | 备用 |
| storage | 0x420000 | 1000KB | 用户数据 |

## SD 卡目录结构

```
/sdcard/
├── apps/              # 已安装的 .app
├── data/              # App 运行数据
├── programs/          # 积木程序 (.json)
├── system/            # 系统资源
└── roms/              # ROM 镜像
```

## 快速启动

正常开机 → 自动运行上次 ROM (Loader 快速启动)
按住 **B 键开机** → 进入 Loader 菜单 → 选择 `xiaomiao-os.bin`

## License

MIT
