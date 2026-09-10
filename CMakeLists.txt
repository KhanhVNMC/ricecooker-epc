cmake_minimum_required(VERSION 4.1)
project(ricecooker C)

set(CMAKE_C_STANDARD 23)

add_executable(ricecooker main.c
        rchal.h)
