# XiaoMiao OS main component
# Includes MicroPython interpreter sources directly (merged into libmain.a)

set(MICROPYTHON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../components/micropython/micropython)
set(MICROPY_PORT_DIR ${MICROPYTHON_DIR}/ports/esp32)
set(MICROPY_MODULES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../components/micropython/modules)

# Collect MicroPython core sources (only py/*.c, not ports/esp32/*.c which needs MicroPython's own build env)
file(GLOB MICROPY_CORE_SOURCES
    ${MICROPYTHON_DIR}/py/*.c
)

# Collect custom module sources
file(GLOB XIAOMIAO_MODULE_SOURCES
    ${MICROPY_MODULES_DIR}/xiaomiao/*.c
)

idf_component_register(SRCS "main.c"
                            "app_manager.c"
                            "app_store.c"
                            "power.c"
                            "micropython_runtime.c"
                            "ui/ui_statusbar.c"
                            "ui/ui_desktop.c"
                            "ui/ui_applist.c"
                            "ui/ui_settings.c"
                            "ui/ui_apprun.c"
                            "ui/ui_appstore.c"
                            "ui/ui_main.c"
                            ${MICROPY_CORE_SOURCES}
                            ${XIAOMIAO_MODULE_SOURCES}
                       PRIV_REQUIRES esp_lcd
                                     esp_timer
                                     esp_driver_gpio
                                     esp_driver_spi
                                     esp_driver_ledc
                                     esp_driver_i2c
                                     esp_hw_support
                                     heap
                                     spi_flash
                                     lvgl
                                     app_update
                                     fatfs
                                     spiffs
                                     esp_adc
                                     esp_wifi
                                     freertos
                                     esp_system
                                     esp_rom
                       INCLUDE_DIRS "ui"
                                    ".."
                                    "../components/micropython"
                                    "../components/micropython/micropython"
                                    "../components/micropython/micropython/py"
                                    "../components/micropython/micropython/ports/esp32"
                                    "../components/micropython/micropython/ports/esp32/boards/ESP32_GENERIC"
                                    "../components/micropython/micropython/py/genhdr"
                                    "../components/micropython/modules/xiaomiao")

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
    -DMICROPY_PY_SYS_PATH=1
    -DMICROPY_PY_SYS_ATTR_DELEGATION=1
    -DMICROPY_HEAP_SIZE=65536
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-error=format-truncation
)

target_link_libraries(${COMPONENT_LIB} PRIVATE m)

# Generate MicroPython build headers at build time
set(GENHDR_DIR ${MICROPYTHON_DIR}/py/genhdr)
set(MKVERSION ${MICROPYTHON_DIR}/py/mkversionheader.py)

# Create genhdr directory
file(MAKE_DIRECTORY ${GENHDR_DIR})

# Custom command to generate qstrdefs.generated.h by scanning MicroPython source
# Use the generate_qstrdefs.py script when available, fallback to empty if not
set(QSTRGEN_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/generate_qstrdefs.py)
add_custom_command(OUTPUT ${GENHDR_DIR}/qstrdefs.generated.h
    COMMAND python3 ${QSTRGEN_SCRIPT} ${MICROPYTHON_DIR}/py ${GENHDR_DIR}/qstrdefs.generated.h
    COMMAND echo "--- qstrdefs.generated.h generated ---"
    COMMENT "Generating qstrdefs.generated.h by scanning MicroPython source"
    DEPENDS ${QSTRGEN_SCRIPT}
    VERBATIM
)

# Custom command to generate mpversion.h (mkversionheader.py may not exist in v1.22.0)
add_custom_command(OUTPUT ${GENHDR_DIR}/mpversion.h
    COMMAND ${CMAKE_COMMAND} -E echo "// Auto-generated mpversion" > ${GENHDR_DIR}/mpversion.h
    COMMAND ${CMAKE_COMMAND} -E echo "#define MICROPY_VERSION_MAJOR (1)" >> ${GENHDR_DIR}/mpversion.h
    COMMAND ${CMAKE_COMMAND} -E echo "#define MICROPY_VERSION_MINOR (22)" >> ${GENHDR_DIR}/mpversion.h
    COMMAND ${CMAKE_COMMAND} -E echo "#define MICROPY_VERSION_MICRO (0)" >> ${GENHDR_DIR}/mpversion.h
    COMMAND ${CMAKE_COMMAND} -E echo "#define MICROPY_VERSION_STRING \"v1.22.0\"" >> ${GENHDR_DIR}/mpversion.h
    COMMAND ${CMAKE_COMMAND} -E echo "#define MICROPY_GIT_TAG \"v1.22.0\"" >> ${GENHDR_DIR}/mpversion.h
    COMMAND ${CMAKE_COMMAND} -E echo "#define MICROPY_BUILD_DATE \"2024-01-01\"" >> ${GENHDR_DIR}/mpversion.h
    COMMENT "Generating mpversion.h"
    VERBATIM
)

# Custom command to generate root_pointers.h
add_custom_command(OUTPUT ${GENHDR_DIR}/root_pointers.h
    COMMAND ${CMAKE_COMMAND} -E echo "// Auto-generated root pointers" > ${GENHDR_DIR}/root_pointers.h
    COMMAND ${CMAKE_COMMAND} -E echo "mp_obj_list_t mp_sys_argv_obj;" >> ${GENHDR_DIR}/root_pointers.h
    COMMAND ${CMAKE_COMMAND} -E echo "mp_obj_t sys_mutable[MP_SYS_MUTABLE_NUM];" >> ${GENHDR_DIR}/root_pointers.h
    COMMAND ${CMAKE_COMMAND} -E echo "mp_obj_base_t * cur_exception;" >> ${GENHDR_DIR}/root_pointers.h
    COMMAND ${CMAKE_COMMAND} -E echo "mp_obj_t sys_exitfunc;" >> ${GENHDR_DIR}/root_pointers.h
    COMMENT "Generating root_pointers.h"
    VERBATIM
)

# Custom command to generate compiler.h
add_custom_command(OUTPUT ${GENHDR_DIR}/compiler.h
    COMMAND ${CMAKE_COMMAND} -E echo "// Auto-generated" > ${GENHDR_DIR}/compiler.h
    COMMAND ${CMAKE_COMMAND} -E echo "#define MICROPY_COMP_CONST (1)" >> ${GENHDR_DIR}/compiler.h
    COMMAND ${CMAKE_COMMAND} -E echo "#define MICROPY_COMP_DOUBLE_TUPLE (1)" >> ${GENHDR_DIR}/compiler.h
    COMMAND ${CMAKE_COMMAND} -E echo "#define MICROPY_COMP_TRIPLE_TUPLE (1)" >> ${GENHDR_DIR}/compiler.h
    COMMAND ${CMAKE_COMMAND} -E echo "#define MICROPY_COMP_RETURN_IF_EXPR (1)" >> ${GENHDR_DIR}/compiler.h
    COMMENT "Generating compiler.h"
    VERBATIM
)

# Custom command to generate nimgc.h
add_custom_command(OUTPUT ${GENHDR_DIR}/nimgc.h
    COMMAND ${CMAKE_COMMAND} -E echo "// Auto-generated" > ${GENHDR_DIR}/nimgc.h
    COMMENT "Generating nimgc.h"
    VERBATIM
)

# Custom command to generate moduledefs.h
# MICROPY_REGISTERED_MODULES and MICROPY_REGISTERED_EXTENSIBLE_MODULES are used
# in objmodule.c to define the built-in module arrays.
add_custom_command(OUTPUT ${GENHDR_DIR}/moduledefs.h
    COMMAND ${CMAKE_COMMAND} -E echo "// Auto-generated moduledefs" > ${GENHDR_DIR}/moduledefs.h
    COMMAND ${CMAKE_COMMAND} -E echo "#ifndef MODULEDEFS_H" >> ${GENHDR_DIR}/moduledefs.h
    COMMAND ${CMAKE_COMMAND} -E echo "#define MODULEDEFS_H" >> ${GENHDR_DIR}/moduledefs.h
    COMMAND ${CMAKE_COMMAND} -E echo "// Empty module list - no built-in modules registered" >> ${GENHDR_DIR}/moduledefs.h
    COMMAND ${CMAKE_COMMAND} -E echo "#define MICROPY_REGISTERED_MODULES" >> ${GENHDR_DIR}/moduledefs.h
    COMMAND ${CMAKE_COMMAND} -E echo "#define MICROPY_REGISTERED_EXTENSIBLE_MODULES" >> ${GENHDR_DIR}/moduledefs.h
    COMMAND ${CMAKE_COMMAND} -E echo "#endif // MODULEDEFS_H" >> ${GENHDR_DIR}/moduledefs.h
    COMMENT "Generating moduledefs.h"
    VERBATIM
)

# Add a custom target to generate all genhdr files
add_custom_target(micropython_genhdr ALL
    DEPENDS
        ${GENHDR_DIR}/qstrdefs.generated.h
        ${GENHDR_DIR}/mpversion.h
        ${GENHDR_DIR}/root_pointers.h
        ${GENHDR_DIR}/compiler.h
        ${GENHDR_DIR}/nimgc.h
        ${GENHDR_DIR}/moduledefs.h
)
add_dependencies(${COMPONENT_LIB} micropython_genhdr)