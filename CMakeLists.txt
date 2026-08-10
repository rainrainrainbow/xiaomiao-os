# ==================== 小喵 OS — 顶层 CMakeLists ====================
# 小喵 OS - LVGL 9.5 + ESP-IDF 5.5.4
# 目标芯片: ESP32-WROVER-B (4MB Flash / 8MB PSRAM)

cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME "xiaomiao-os")
set(PROJECT_VER  "0.1.0")

# 指定 esp-idf 需要 PSRAM
set(IDF_INIT_OPTIONS "PSRAM=8MB")

# CMakeLists.txt 必须留空行给 idf 依赖注入
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${PROJECT_NAME})
