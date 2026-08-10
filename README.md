# 小喵 OS · xiaomiao-os

> 安卓风格的桌面操作系统，运行在学而思小喵教育掌机（ESP32-WROVER-B 4MB Flash + 8MB PSRAM）
> 上。App 以 MicroPython 沙盒方式运行，类似 Android 的应用隔离。

![硬件](https://img.shields.io/badge/hardware-ESP32--WROVER--B-blue)
![屏幕](https://img.shields.io/badge/display-ST7735%20160x128-orange)
![语言](https://img.shields.io/badge/MicroPython-1.23+-green)
![构建](https://img.shields.io/badge/build-GitHub%20Actions-yellow)
![授权](https://img.shields.io/badge/license-MIT-lightgrey)

---

## 项目定位

这是**为小喵掌机开发的桌面 OS 固件**，不是工具脚本，也不是单纯的 LVGL demo。
目标是把原厂封闭的"学而思内容机"改造成可加载第三方 App 的开放平台。

设计参考 Android：

| Android 概念 | 小喵 OS 对应 |
| --- | --- |
| Activity | MicroPython App |
| Application Class | `app_info_t` |
| Manifest | `.app` 文件头部 JSON |
| Permissions | `MP_PERM_DISPLAY / BUTTONS / ...` |
| Intent (启动) | `app_manager_launch(app_info_t*)` |
| Settings | `core/settings.c` (NVS) |
| Launcher | `app_manager.c` SCREEN_HOME |
| Wallpaper | `lv_color_hex(0xF6D34A)` |
| Status bar | 状态栏（时间/电量） |

---

## 硬件

| 模块 | 接口 | 引脚 |
| --- | --- | --- |
| ST7735 TFT 160×128 | SPI2 (60MHz) | SCLK=18 MOSI=23 MISO=19 CS=5 DC=4 RST=枚举外加 |
| TF 卡 (SDSPI) | SPI2 (共享) | CS=22 |
| 按键 UP/DOWN/LEFT/RIGHT/B | GPIO (低有效) | 2 / 13 / 27 / 35 / 12 |
| 按键 A | **ADC 复用** (GPIO34 / ADC1_CH6) | 9.1k+2.4k 分压，<0.3V 为按下 |
| 背光 | LEDC PWM (低电平点亮) | GPIO0 / Timer1 / Ch1 |
| 蜂鸣器 | LEDC | GPIO14 / Timer0 / Ch0 |
| MPU6050 | I2C | SCL=15 SDA=21 (addr 0x68) |
| GD32 协处理器 | UART0 (协议) | TX=1 RX=3 |
| 电池电压 | ADC1_CH6 (与 A 键共用) | GPIO34 |

> **关键硬件修订（v0.2）**：
> - 背光改为 **GPIO0 LEDC 控制**（低电平点亮，可软件调节亮度）
> - **A 键复用电池分压电路**（按下短路分压到 GND → ADC ≈ 0V）

---

## 软件架构

```
xiaomiao-os/
├── CMakeLists.txt          # 顶层 (project xiaomiao-os)
├── sdkconfig.defaults      # 240MHz, 4MB Flash, 8MB PSRAM, LVGL_CONF
├── partitions.csv          # nvs / phy_init / factory / storage / loader
├── Kconfig.projbuild       # 启用 MicroPython 的开关 (CONFIG_ENABLE_MICROPYTHON)
├── lvgl_conf.h             # LVGL 9.5 极小屏配置
├── main/
│   ├── main.c              # 入口
│   ├── return_to_loader.h  # 兼容原厂 Loader
│   ├── hal/                # 7 个硬件模块
│   │   ├── lcd_st7735.{c,h}    # SPI + 三缓冲 + 背光 PWM
│   │   ├── keys.{c,h}          # 5 GPIO + ADC 复用 A 键 + 长按
│   │   ├── battery.{c,h}       # 9.1k+2.4k 分压 + A 键迟滞阈值
│   │   ├── sdcard.{c,h}        # SPI2 共享 SDSPI
│   │   ├── buzzer.{c,h}        # LEDC Timer0 Ch0
│   │   ├── mpu6050.{c,h}       # I2C 0x68
│   │   └── led_motor.{c,h}     # GD32 UART0 协议
│   ├── ui/
│   │   ├── theme.{c,h}         # 小喵配色 (黄/棕/黑/红)
│   │   ├── input.{c,h}         # LVGL keypad device
│   │   ├── toast.{c,h}         # 提示气泡
│   │   └── canvas.{c,h}        # Python App 的独占画布
│   ├── core/
│   │   ├── app_manager.{c,h}   # 5 屏状态机 + 安装/启动/卸载
│   │   ├── settings.{c,h}      # NVS 持久化
│   │   └── app_loader.{c,h}    # 扫描 /sdcard/roms/*.app
│   ├── mpy/
│   │   ├── mp_runtime.{c,h}    # MicroPython 运行时 + 后台任务沙盒
│   │   └── mp_bindings.{c,h}   # HAL 暴露给 `import hal`
│   ├── desktop/
│   │   └── builtins.{c,h}      # 内置 10 个 App 占位
│   ├── idf_component.yml       # LVGL 9.5 + ST7735 驱动
│   ├── Kconfig.projbuild       # CONFIG_ENABLE_MICROPYTHON
│   └── lvgl_conf.h
├── examples/                  # 3 个示例 .app（直接拷到 SD 卡）
│   ├── snake.app
│   ├── clock.app
│   └── pixel.app
├── .github/workflows/
│   ├── build.yml               # 每次 push 编译 + Artifacts
│   └── release.yml             # 打 tag v* → GitHub Releases
└── README.md
```

---

## 编译

### 本地（可选）

> 注意：小喵掌机是低性能 ESP32 + Windows 路径长容易出锅，所以**推荐 GitHub Actions**。

```bash
# 安装 ESP-IDF v5.5.4
git clone --depth 1 --branch v5.5.4 --recurse-submodules https://github.com/espressif/esp-idf.git
./esp-idf/install.sh esp32
source ./esp-idf/export.sh

# 进入项目
cd xiaomiao-os
idf.py set-target esp32
idf.py build
idf.py merge-bin -o dist/xiaomiao-os-merged.bin
```

合并后的 `dist/xiaomiao-os-merged.bin` 是 **Loader 兼容**的单文件固件，可直接拷到 TF 卡刷入。

### GitHub Actions（推荐）

1. Fork 这个仓库
2. 推到 main 分支 → 自动 build + Artifacts
3. 打 tag `v0.2.0` → 自动 Release + 上传 .bin

---

## MicroPython 集成（两种模式）

### 默认：Stub 模式（编译快、桌面仍可用）

- `CONFIG_ENABLE_MICROPYTHON=n` （sdkconfig 默认）
- 所有 `.app` 启动只显示状态条，Python 代码不执行
- 优点：编译只要 1 分钟，bin 体积小（~1.2MB）
- 缺点：App 是占位

### 完整：真 MicroPython（推荐给最终用户）

1. 取消 `main/idf_component.yml` 里 `micropython/micropython: ^1.23` 的注释
2. `sdkconfig.defaults` 改成 `CONFIG_ENABLE_MICROPYTHON=y`
3. Actions 自动重新编译，bin 体积约 1.8MB
4. App 真正能跑 Python：

```python
import hal

hal.display_fill(0xF6D4)     # 黄底
hal.display_text(20, 50, 'Hi')
hal.buzzer_beep(880, 200)
while hal.buttons_peek() != 'B':
    hal.time_sleep_ms(50)
hal.sys_exit()
```

---

## `.app` 文件格式

文件 = **manifest JSON (第一行)** + **空行 (第二行)** + **Python 源码**：

```
{"id": "com.demo.snake", "name": "贪吃蛇", "glyph": "🐍", "version": "1.0.2", "permissions": ["display","buttons","buzzer"]}

import hal
hal.display_fill(0xF6D4)
...
```

拷贝到：`/sdcard/roms/snake.app`，重启后桌面自动出现图标。

### Manifest 字段

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `id` | ✓ | 反向域名，唯一 |
| `name` | ✓ | 显示名（中文 OK） |
| `glyph` | - | emoji 图标（默认 📦） |
| `version` | - | 任意字符串 |
| `permissions` | - | 数组：`display/buttons/buzzer/battery/imu/led/sd/net` |

### Python API（`import hal`）

| 函数 | 说明 |
| --- | --- |
| `hal.display_fill(color)` | RGB565 填屏 |
| `hal.display_text(x, y, s)` | 写字 |
| `hal.display_pixel(x, y, c)` | 单像素 |
| `hal.buttons_peek()` | 取按键（'U/D/L/R/A/B' 或 ''） |
| `hal.buttons_wait()` | 阻塞等下一次按键 |
| `hal.buzzer_beep(freq, ms)` | 蜂鸣 |
| `hal.battery_level()` | 电量 0~1 |
| `hal.battery_voltage()` | 电压 V |
| `hal.imu_is_ready()` | IMU 是否在 |
| `hal.led_set(0/1/2)` | 关/亮/呼吸 |
| `hal.time_sleep_ms(ms)` | 延时 |
| `hal.sys_exit()` | 退出 App |

---

## 6 键输入

| 按键 | 桌面 | 列表 | 设置 | 积木编辑器 | App 运行中 |
| --- | --- | --- | --- | --- | --- |
| ↑↓←→ | 移动光标 | 上下选择 | 上下选择 | 切换面板 / 移行 | 转发给 App |
| **A** | 启动选中 App | 启动 App | 进入项 | 选积木（按数字键直接选） | 转发 |
| **B** | 退回 / 切到列表 | 回桌面 | 回桌面 | 删除行 | **退出 App → 桌面** |
| **A 长按 360ms** | 进入图标整理模式 | — | — | 生成 main.py + 打包 .app | — |
| **1-9** | — | — | — | 数字键直接选对应积木 | — |
| **Del/Ins** | — | — | — | 删除/复制行 | — |

---

## 阶段进度

- [x] 调研 + HTML 模拟器解析
- [x] 顶层工程 / Actions / HAL / UI / 5 屏
- [x] App 管理器 + 内置 App 占位
- [x] `.app` 加载器 + canvas
- [x] MicroPython 运行时 + 后台任务沙盒 + HAL 绑定
- [x] HAL 硬件修订（背光 LEDC / A 键 ADC 复用）
- [x] 示例 `.app`（snake / clock / pixel）
- [ ] GitHub 仓库创建 + push（等待用户 token）
- [ ] Actions 第一次真编译（默认 stub 模式）
- [ ] 启用 `CONFIG_ENABLE_MICROPYTHON=y` + 二次编译验证

---

## 致谢

- 学而思小喵团队（硬件 + 原厂 Loader 兼容）
- LVGL 9.5 (LVGL Kft)
- MicroPython (Damien P. George + contributors)
- ESP-IDF (Espressif Systems)
- ZYoungInc 提供的电路原理图分析