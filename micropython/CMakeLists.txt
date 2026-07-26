# MicroPython ESP32 Port - CMake Integration
#
# This file allows building MicroPython as part of the XiaoMiao OS firmware.
# It downloads and builds the MicroPython ESP32 port, then links it into our binary.
#
# Usage: The main CMakeLists.txt includes this via add_subdirectory()

cmake_minimum_required(VERSION 3.16)

# Option to enable/disable MicroPython
option(XIAOMIAO_ENABLE_MICROPYTHON "Enable MicroPython integration" ON)

if(NOT XIAOMIAO_ENABLE_MICROPYTHON)
    message(STATUS "MicroPython disabled - building without MP support")
    return()
endif()

# MicroPython source directory (relative to project root)
set(MP_SRC_DIR ${CMAKE_SOURCE_DIR}/micropython)

# Check if MicroPython source exists
if(NOT EXISTS ${MP_SRC_DIR}/py/mpconfig.h)
    message(WARNING "MicroPython source not found at ${MP_SRC_DIR}")
    message(WARNING "Run: git submodule update --init --recursive")
    message(WARNING "Building without MicroPython support")
    return()
endif()

message(STATUS "Building MicroPython from ${MP_SRC_DIR}")

# MicroPython core sources
set(MP_CORE_SRC
    py/mpstate.c
    py/mpconfig.c
    py/mpprint.c
    py/mpz.c
    py/mphal.c
    py/nlr.c
    py/nlrxtensa.c
    py/qstr.c
    py/qstrdefs.c
    py/stackctrl.c
    py/gc.c
    py/argcheck.c
    py/vm.c
    py/obj.c
    py/objarray.c
    py/objdict.c
    py/objexcept.c
    py/objfloat.c
    py/objfun.c
    py/objgenerator.c
    py/objlist.c
    py/objmodule.c
    py/objstr.c
    py/objtuple.c
    py/objtype.c
    py/objint.c
    py/objbool.c
    py/objnone.c
    py/objmap.c
    py/objset.c
    py/objstringio.c
    py/parse.c
    py/compile.c
    py/emitcommon.c
    py/emitbc.c
    py/formatfloat.c
    py/parsenum.c
    py/parsenumbase.c
    py/runtime.c
    py/runtime0.c
    py/runtime_utils.c
    py/scope.c
    py/showbc.c
    py/persistentcode.c
    py/builtinimport.c
    py/modbuiltins.c
    py/modcollections.c
    py/modgc.c
    py/modio.c
    py/modmath.c
    py/modmicropython.c
    py/modsys.c
    py/modtime.c
    py/modarray.c
    py/moderrno.c
    py/moduerrno.c
    py/asmxtensa.c
    py/emitnative.c
)

# MicroPython port sources (ESP32 specific)
set(MP_PORT_SRC
    ports/esp32/main.c
    ports/esp32/modmachine.c
    ports/esp32/modnetwork.c
    ports/esp32/modsocket.c
    ports/esp32/machine_adc.c
    ports/esp32/machine_i2c.c
    ports/esp32/machine_spi.c
    ports/esp32/machine_uart.c
    ports/esp32/machine_pin.c
    ports/esp32/machine_pwm.c
    ports/esp32/machine_rtc.c
    ports/esp32/machine_wdt.c
    ports/esp32/machine_timer.c
    ports/esp32/network_lan.c
    ports/esp32/mpthreadport.c
)

# Create MicroPython library
add_library(micropython STATIC
    ${MP_CORE_SRC}
    ${MP_PORT_SRC}
)

target_include_directories(micropython PRIVATE
    ${MP_SRC_DIR}/py
    ${MP_SRC_DIR}/ports/esp32
    ${MP_SRC_DIR}/ports/esp32/include
    ${IDF_PATH}/components/esp_wifi/include
)

target_compile_definitions(micropython PRIVATE
    MICROPY_HW_ENABLE_USBDEV=0
    MICROPY_HW_ENABLE_USBHID=0
    MICROPY_HW_HAS_FLASH=1
    MICROPY_PY_USOCKET=1
    MICROPY_PY_NETWORK=1
    MICROPY_PY_MACHINE=1
    MICROPY_PY_MACHINE_ADC=1
    MICROPY_PY_MACHINE_I2C=1
    MICROPY_PY_MACHINE_SPI=1
    MICROPY_PY_MACHINE_UART=1
    MICROPY_PY_MACHINE_PIN=1
    MICROPY_PY_MACHINE_PWM=1
    MICROPY_PY_MACHINE_TIMER=1
    MICROPY_PY_MACHINE_RTC=1
    MICROPY_PY_MACHINE_WDT=1
    MICROPY_PY_THREAD=0
    MICROPY_ENABLE_GC=1
    MICROPY_HEAP_SIZE=131072
    MICROPY_STACK_SIZE=16384
)

# Link MicroPython into the main component
target_link_libraries(main PRIVATE micropython)
