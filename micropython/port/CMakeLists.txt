# MicroPython Port for XiaoMiao OS
#
# This provides either:
#   1. Full MicroPython integration (when MicroPython submodule is present)
#   2. Lite mode (log-only fallback for development without MP)

set(MP_PORT_SRCS
    mp_port_xiaomiao.c
)

idf_component_register(
    SRCS ${MP_PORT_SRCS}
    INCLUDE_DIRS "."
    REQUIRES driver freertos
)

# Check for full MicroPython source
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/../py/mpconfig.h)
    message(STATUS "Full MicroPython source detected - enabling WITH_MICROPYTHON")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE WITH_MICROPYTHON)
    
    # Add MicroPython core sources
    target_sources(${COMPONENT_LIB} PRIVATE
        ../py/mpstate.c
        ../py/mpprint.c
        ../py/mpz.c
        ../py/mphal.c
        ../py/nlr.c
        ../py/qstr.c
        ../py/stackctrl.c
        ../py/gc.c
        ../py/argcheck.c
        ../py/vm.c
        ../py/obj.c
        ../py/objarray.c
        ../py/objdict.c
        ../py/objexcept.c
        ../py/objfun.c
        ../py/objlist.c
        ../py/objmodule.c
        ../py/objstr.c
        ../py/objtuple.c
        ../py/objtype.c
        ../py/objint.c
        ../py/parse.c
        ../py/compile.c
        ../py/emitcommon.c
        ../py/emitbc.c
        ../py/formatfloat.c
        ../py/parsenum.c
        ../py/runtime.c
        ../py/runtime0.c
        ../py/runtime_utils.c
        ../py/builtinimport.c
        ../py/modbuiltins.c
        ../py/modgc.c
        ../py/modio.c
        ../py/modmath.c
        ../py/modmicropython.c
        ../py/modsys.c
        ../py/modtime.c
    )
    
    target_include_directories(${COMPONENT_LIB} PRIVATE
        ../py
        ../ports/esp32/include
    )
    
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        MICROPY_ENABLE_GC=1
        MICROPY_HEAP_SIZE=131072
        MICROPY_PY_USOCKET=0
        MICROPY_PY_NETWORK=0
        MICROPY_PY_MACHINE=1
        MICROPY_PY_THREAD=0
    )
else()
    message(STATUS "MicroPython source not found - building in LITE mode")
    message(STATUS "  MP code will be logged but not executed")
    message(STATUS "  To enable full MP: git submodule add https://github.com/micropython/micropython.git micropython/")
endif()
