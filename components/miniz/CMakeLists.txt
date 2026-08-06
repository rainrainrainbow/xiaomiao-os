# miniz component - single-file deflate/inflate library
# Auto-downloaded from GitHub at CMake configure time

if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/miniz.h")
    message(STATUS "Downloading miniz from GitHub...")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/richgel999/miniz/master/miniz.h"
        "${CMAKE_CURRENT_LIST_DIR}/miniz.h"
        SHOW_PROGRESS)
    file(DOWNLOAD
        "https://raw.githubusercontent.com/richgel999/miniz/master/miniz.c"
        "${CMAKE_CURRENT_LIST_DIR}/miniz.c"
        SHOW_PROGRESS)
endif()

idf_component_register(SRCS "miniz.c"
                       INCLUDE_DIRS ".")