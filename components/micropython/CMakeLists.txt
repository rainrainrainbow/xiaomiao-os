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
    -DMICROPY_PY_SYS_PATH=1
    -DMICROPY_HEAP_SIZE=65536
    -Wno-unused-function
    -Wno-unused-variable
)

# Generate MicroPython build headers at build time
set(GENHDR_DIR ${MICROPYTHON_DIR}/py/genhdr)
set(MKQSTRDEFS ${MICROPYTHON_DIR}/py/makeqstrdefs.py)
set(MKVERSION ${MICROPYTHON_DIR}/py/mkversionheader.py)

# Create genhdr directory
file(MAKE_DIRECTORY ${GENHDR_DIR})

# Custom command to generate qstrdefs.generated.h
# Must contain valid Q() entries - empty qstr list is fine
add_custom_command(OUTPUT ${GENHDR_DIR}/qstrdefs.generated.h
    COMMAND ${CMAKE_COMMAND} -E echo "// Auto-generated qstrdefs" > ${GENHDR_DIR}/qstrdefs.generated.h
    COMMENT "Generating qstrdefs.generated.h"
    VERBATIM
)

# Custom command to generate mpversion.h
add_custom_command(OUTPUT ${GENHDR_DIR}/mpversion.h
    COMMAND python3 ${MKVERSION} ${GENHDR_DIR}/mpversion.h
    COMMENT "Generating mpversion.h"
    VERBATIM
)

# Custom command to generate root_pointers.h
add_custom_command(OUTPUT ${GENHDR_DIR}/root_pointers.h
    COMMAND ${CMAKE_COMMAND} -E echo "// Auto-generated" > ${GENHDR_DIR}/root_pointers.h
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
add_custom_command(OUTPUT ${GENHDR_DIR}/moduledefs.h
    COMMAND ${CMAKE_COMMAND} -E echo "// Auto-generated" > ${GENHDR_DIR}/moduledefs.h
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