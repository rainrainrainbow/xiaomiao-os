# MicroPython component for XiaoMiao OS
# This component wraps the MicroPython interpreter and custom modules

set(MICROPYTHON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/micropython)

# Check if MicroPython source exists
if(NOT EXISTS ${MICROPYTHON_DIR}/py/micropython.mk)
    message(WARNING "MicroPython source not found. Run scripts/fetch_micropython.sh first.")
    return()
endif()

set(MICROPY_SOURCE_DIR ${MICROPYTHON_DIR}/py)
set(MICROPY_PORT_DIR ${MICROPYTHON_DIR}/ports/esp32)
set(MICROPY_MODULES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/modules)

# Collect MicroPython core sources
file(GLOB MICROPY_CORE_SOURCES
    ${MICROPY_SOURCE_DIR}/*.c
    ${MICROPY_PORT_DIR}/*.c
)

# Collect custom module sources
file(GLOB XIAOMIAO_MODULE_SOURCES
    ${MICROPY_MODULES_DIR}/xiaomiao/*.c
)

idf_component_register(
    SRCS ${MICROPY_CORE_SOURCES} ${XIAOMIAO_MODULE_SOURCES}
    INCLUDE_DIRS
        ${MICROPYTHON_DIR}
        ${MICROPYTHON_DIR}/py
        ${MICROPY_PORT_DIR}
        ${MICROPY_PORT_DIR}/boards/ESP32_GENERIC
        ${MICROPY_MODULES_DIR}
    PRIV_REQUIRES
        esp_lcd
        esp_driver_gpio
        esp_driver_ledc
        esp_adc
        fatfs
    REQUIRES
        freertos
        esp_system
        esp_timer
)

# MicroPython build flags
target_compile_options(${COMPONENT_LIB} PRIVATE
    -DMICROPY_ENABLE_COMPILER=1
    -DMICROPY_PY_BUILTINS=1
    -DMICROPY_PY_GC=1
    -DMICROPY_PY_ARRAY=1
    -DMICROPY_PY_COLLECTIONS=1
    -DMICROPY_PY_MATH=1
    -DMICROPY_PY_IO=1
    -DMICROPY_PY_STRUCT=1
    -DMICROPY_PY_SYS=1
    -DMICROPY_HEAP_SIZE=65536
    -Wno-unused-function
    -Wno-unused-variable
)