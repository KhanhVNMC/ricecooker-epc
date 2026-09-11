cmake_minimum_required(VERSION 3.29.6)
project(ricecooker C)

set(CMAKE_C_STANDARD 23)

add_executable(ricecooker
        src/main/test.c
        src/drivers/tm1637dv.c
        src/impl/unix/d_unix_rchal.c
        src/impl/unix/d_unix_mem.c
        src/impl/unix/d_unix_platform.c
        src/impl/unix/d_unix_xthreads.c
        src/impl/freertos/rtosimpl.c
        src/include/heaterctl.h
        src/drivers/heaterctl.c
        src/include/cmath.h
)

target_include_directories(ricecooker PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
