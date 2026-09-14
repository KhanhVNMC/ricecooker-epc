cmake_minimum_required(VERSION 3.22)
project(ricecooker_host LANGUAGES C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(RICECOOKER_BUILD_STM32 "Build the STM32F103C8 firmware instead of the Unix harness" OFF)

if(RICECOOKER_BUILD_STM32)
    add_subdirectory(firmware/stm32c8)
else()
    add_subdirectory(src) # build the ricecooker_core
    find_package(Threads REQUIRED)

    add_executable(ricecooker
        impl/unix/d_unix_rchal.c
        impl/unix/d_unix_mem.c
        impl/unix/d_unix_platform.c
        impl/unix/d_unix_xthreads.c
        impl/unix/fake_io.h
        impl/unix/fake_tm1637.c
        impl/unix/fake_btns.c
    )

    target_link_libraries(ricecooker
        PRIVATE ricecooker_core
        Threads::Threads
    )
endif()
