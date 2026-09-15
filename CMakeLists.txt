cmake_minimum_required(VERSION 3.22)
project(ricecooker_host LANGUAGES C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(RICECOOKER_BUILD_STM32 "Build the STM32F103C8 firmware" OFF)

if(RICECOOKER_BUILD_STM32)
    add_subdirectory(builder/stm32c8)
else()
    add_subdirectory(builder/unix)
endif()
