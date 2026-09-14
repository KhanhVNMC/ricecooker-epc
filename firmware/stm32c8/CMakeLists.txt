# THIS FILE IS AI-GENERATED BECAUSE I DON'T WANT TO SPEND LIKE A DAY
# RAVAGING AROUND THE BUILD TOOLS
cmake_minimum_required(VERSION 3.22)
project(ricecooker_stm32c8 LANGUAGES C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(RICECOOKER_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(STM32_PLATFORM_DIR "${RICECOOKER_ROOT}/impl/stm32mcu")
set(STM32_DEVICES_CONFIG_DIR "${STM32_PLATFORM_DIR}/dcfg")

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE "${STM32_DEVICES_CONFIG_DIR}")
target_compile_definitions(freertos_config INTERFACE STM32F103xB)

include("${RICECOOKER_ROOT}/cmake/dependencies/stm32f1.cmake")
add_subdirectory("${RICECOOKER_ROOT}/src" "${CMAKE_CURRENT_BINARY_DIR}/ricecooker_core")

file(GLOB_RECURSE STM32_PLATFORM_SOURCES CONFIGURE_DEPENDS
    "${STM32_PLATFORM_DIR}/*.c"
)

add_executable(ricecooker_stm32c8
    ${STM32_PLATFORM_SOURCES}
    "${cmsis_device_f1_SOURCE_DIR}/Source/Templates/system_stm32f1xx.c"
    "${cmsis_device_f1_SOURCE_DIR}/Source/Templates/gcc/startup_stm32f103xb.s"
)
set_target_properties(ricecooker_stm32c8 PROPERTIES
    OUTPUT_NAME ricecooker
    SUFFIX .elf
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
target_compile_features(ricecooker_stm32c8 PRIVATE c_std_23)
target_compile_options(ricecooker_stm32c8 PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-Og;-g3>
    $<$<CONFIG:Release>:-Os>
)
target_compile_definitions(ricecooker_stm32c8 PRIVATE
    USE_HAL_DRIVER
    STM32F103xB
)
target_include_directories(ricecooker_stm32c8 PRIVATE "${STM32_PLATFORM_DIR}")
target_link_libraries(ricecooker_stm32c8 PRIVATE
    ricecooker_core
    stm32::hal
    freertos_kernel
)

set(STM32_LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/STM32F103C8_FLASH.ld")
target_link_options(ricecooker_stm32c8 PRIVATE
    "-T${STM32_LINKER_SCRIPT}"
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    "-Wl,-Map=${CMAKE_BINARY_DIR}/ricecooker.map"
    -Wl,--print-memory-usage
)
set_property(TARGET ricecooker_stm32c8 APPEND PROPERTY LINK_DEPENDS "${STM32_LINKER_SCRIPT}")

add_custom_command(TARGET ricecooker_stm32c8 POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:ricecooker_stm32c8>" "${CMAKE_BINARY_DIR}/ricecooker.bin"
    COMMAND "${CMAKE_OBJCOPY}" -O ihex "$<TARGET_FILE:ricecooker_stm32c8>" "${CMAKE_BINARY_DIR}/ricecooker.hex"
    COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:ricecooker_stm32c8>"
)
