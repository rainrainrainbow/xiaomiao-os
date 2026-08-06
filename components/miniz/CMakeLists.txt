# miniz component - deflate/inflate library
# Auto-downloaded from GitHub at CMake configure time
# miniz.c is the single-file amalgamation containing all code

set(MINIZ_FILES
    miniz.h miniz.c miniz_export.h
    miniz_common.h miniz_tdef.h
    miniz_tinfl.h miniz_zip.h)

foreach(F ${MINIZ_FILES})
    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/${F}")
        message(STATUS "Downloading ${F} from miniz...")
        file(DOWNLOAD
            "https://raw.githubusercontent.com/richgel999/miniz/master/${F}"
            "${CMAKE_CURRENT_LIST_DIR}/${F}"
            SHOW_PROGRESS)
    endif()
endforeach()

idf_component_register(SRCS "miniz.c"
                       INCLUDE_DIRS "."
                       PRIV_INCLUDE_DIRS ".")
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=implicit-function-declaration
    -Wno-error=type-limits
    -Wno-error=unused-function
    -Wno-error=unused-variable)