cmake_minimum_required(VERSION 3.29.6)
project(ricecooker C)

set(CMAKE_C_STANDARD 23)

add_executable(ricecooker main.c
        src/rcplatform/rchal.h
        src/xthreads/xthreads.h
        src/service/test.c
        src/rcplatform/platform.h
)
