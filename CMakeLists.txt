cmake_minimum_required(VERSION 3.22)
# CLion auto-reload and a CLI configure can target the same preset directory.
# Serialize configuration before FetchContent can replace dependency checkouts.
file(LOCK "${CMAKE_BINARY_DIR}/configure.lock" GUARD PROCESS TIMEOUT 180)
project(ricecooker_host LANGUAGES C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(RICECOOKER_BUILD_STM32 "Build the STM32F103C8 firmware" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

if(RICECOOKER_BUILD_STM32)
    add_subdirectory(builder/stm32c8)
else()
    add_subdirectory(builder/unix)
endif()
