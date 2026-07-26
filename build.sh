#!/bin/bash
# XiaoMiao OS 构建脚本
# 用法: ./build.sh [clean|build|flash|monitor]

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=================================${NC}"
echo -e "${GREEN}XiaoMiao OS Build System${NC}"
echo -e "${GREEN}=================================${NC}"

# 检查ESP-IDF环境
if [ -z "$IDF_PATH" ]; then
    echo -e "${YELLOW}ESP-IDF 环境未加载，尝试加载...${NC}"
    if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
        . $HOME/esp/esp-idf/export.sh
    elif [ -f "/opt/esp/idf/export.sh" ]; then
        . /opt/esp/idf/export.sh
    else
        echo -e "${RED}错误: 未找到 ESP-IDF${NC}"
        echo "请先安装 ESP-IDF v5.5.4:"
        echo "  mkdir -p ~/esp && cd ~/esp"
        echo "  git clone -b v5.5.4 --recursive https://github.com/espressif/esp-idf.git"
        echo "  cd esp-idf && ./install.sh esp32"
        exit 1
    fi
fi

echo -e "${GREEN}ESP-IDF 版本: ${IDF_VERSION}${NC}"

# 设置目标芯片
idf.py set-target esp32

# 处理命令
case "$1" in
    clean)
        echo -e "${YELLOW}清理构建...${NC}"
        idf.py fullclean
        ;;
    build)
        echo -e "${GREEN}开始构建...${NC}"
        idf.py build
        echo -e "${GREEN}构建完成!${NC}"
        echo -e "固件位置: build/xiaomiao-os.bin"
        echo -e "合并固件: build/merged.bin"
        ;;
    flash)
        echo -e "${GREEN}烧录固件...${NC}"
        if [ -z "$2" ]; then
            echo -e "${YELLOW}用法: ./build.sh flash <端口>${NC}"
            echo -e "示例: ./build.sh flash /dev/ttyUSB0"
            exit 1
        fi
        idf.py -p $2 flash
        ;;
    monitor)
        echo -e "${GREEN}启动监视器...${NC}"
        if [ -z "$2" ]; then
            echo -e "${YELLOW}用法: ./build.sh monitor <端口>${NC}"
            exit 1
        fi
        idf.py -p $2 monitor
        ;;
    *)
        echo -e "${YELLOW}用法: ./build.sh [clean|build|flash|monitor]${NC}"
        echo ""
        echo "命令:"
        echo "  clean    - 清理构建"
        echo "  build    - 构建固件"
        echo "  flash    - 烧录固件 (需要指定端口)"
        echo "  monitor  - 启动串口监视器 (需要指定端口)"
        ;;
esac