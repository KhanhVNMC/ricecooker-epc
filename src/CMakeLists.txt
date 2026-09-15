cmake_minimum_required(VERSION 3.22)
project(ricecooker_unix_fake LANGUAGES C)

file(GLOB_RECURSE RICECOOKER_CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c" # all files ending with .c recursively
)

# makes a static lib with the .c files
add_library(ricecooker_core STATIC ${RICECOOKER_CORE_SOURCES}
        ../impl/stm32c8/microtime.h)
target_compile_features(ricecooker_core PUBLIC c_std_23) # c23 shit

# so we dont have to #include "../../bullshit.h"
target_include_directories(ricecooker_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
