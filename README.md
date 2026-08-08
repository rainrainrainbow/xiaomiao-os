# 小喵OS — Xiaomiao OS

> ESP32-WROVER-B + ST7735 160×128 TFT + MicroSD + 6-key keypad + MicroPython 桌面系统

## 项目结构

```
xiaomiao-os/
├── CMakeLists.txt              # 顶层 ESP-IDF 项目
├── sdkconfig.defaults          # 板级配置
├── partitions.csv              # 分区表 (Loader兼容 + SPIFFS)
├── return_to_loader.h           # 返回 Loader 集成
├── .github/workflows/build.yml # GitHub Actions CI
├── docs/                        # 文档
├── main/
│   ├── CMakeLists.txt
│   ├── main.c                  # 主程序 (Phase 1: 硬件验证)
│   └── idf_component.yml       # LVGL 9.5.0 依赖
└── components/
    ├── xiaomiao_hal/            # 硬件抽象层
    │   ├── CMakeLists.txt
    │   ├── include/xiaomiao_hal.h
    │   └── xiaomiao_hal.c       # LCD/SD/I2C/Buzzer/Battery/Buttons
    └── app_manager/             # 应用管理器
        ├── CMakeLists.txt
        ├── include/app_manager.h
        └── app_manager.c        # .app 扫描/安装/卸载
```

## 开发阶段

| 阶段 | 内容 | 状态 |
|------|------|------|
| 一 | 环境搭建与基础硬件验证 | ✅ 项目骨架完成 |
| 二 | 桌面UI框架开发 (HTML模拟) | ⏳ 待审核HTML模拟 |
| 三 | C语言核心系统实现 (LVGL) | 🔜 等待审核后开始 |
| 四 | 系统功能完善 | 🔜 |
| 五 | 编译与交付 | 🔜 |

## 硬件引脚

| GPIO | Function |
|------|----------|
| 2 | Button UP |
| 4 | LCD DC |
| 5 | LCD CS |
| 12 | Button B (ESC) |
| 13 | Button DOWN |
| 14 | Buzzer (LEDC) |
| 15 | I2C SCL |
| 18 | LCD SCLK |
| 19 | LCD MISO |
| 21 | I2C SDA |
| 22 | SD CS |
| 23 | LCD MOSI |
| 27 | Button LEFT |
| 32 | Battery ADC (CH4) |
| 34 | Button A (ENTER) — input only, no pull-up |
| 35 | Button RIGHT — input only, no pull-up |

## 构建

```bash
. ~/esp/esp-idf/export.sh
cd xiaomiao-os
idf.py build
idf.py -p /dev/ttyACM0 flash
idf.py merge-bin
```

## License

MIT