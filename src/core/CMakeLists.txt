cmake_minimum_required(VERSION 3.22)
project(ricecooker_unix_fake LANGUAGES C)

file(GLOB_RECURSE RICECOOKER_CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c" # all files ending with .c recursively
)

# makes a static lib with the .c files
add_library(ricecooker_core STATIC ${RICECOOKER_CORE_SOURCES})
target_compile_features(ricecooker_core PUBLIC c_std_23) # c23 shit

if(COMPILED_FOR_PROTEUS_8_17)
    # proteus 8.17 have a stupid isa violating bug: CLZ(0) is 0...
    # ARM isa contract defined CLZ(0) = 32 (common sense)
    target_compile_definitions(ricecooker_core PUBLIC COMPILED_FOR_PROTEUS_8_17) # PUBLIC so it propagates upwards
endif()

# so we dont have to #include "../../bullshit.h"
target_include_directories(ricecooker_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
