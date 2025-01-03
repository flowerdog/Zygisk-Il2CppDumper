cmake_minimum_required(VERSION 3.4.1)

add_library(
        zygisk_il2cpp_dumper
        SHARED
        main.cpp
        il2cpp_dump.cpp
        hack.cpp
        log.cpp
        config_parser.cpp
        xdl/xdl.c
        xdl/xdl_iterate.c
        xdl/xdl_linker.c
        xdl/xdl_lzma.c
        xdl/xdl_util.c
)

target_include_directories(
        zygisk_il2cpp_dumper
        PRIVATE
        xdl/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

find_library(
        log-lib
        log
)

target_link_libraries(
        zygisk_il2cpp_dumper
        ${log-lib}
)
