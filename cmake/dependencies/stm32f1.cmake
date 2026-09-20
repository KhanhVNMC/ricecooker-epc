# THIS FILE IS AI-GENERATED BECAUSE I DON'T WANT TO SPEND LIKE A DAY
# RAVAGING AROUND THE BUILD TOOLS
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

FetchContent_Declare(cmsis_core
    GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis-core.git
    GIT_TAG v5.4.0_cm3
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_Declare(cmsis_device_f1
    GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis-device-f1.git
    GIT_TAG v4.3.3
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_Declare(stm32f1_hal
    GIT_REPOSITORY https://github.com/STMicroelectronics/stm32f1xx-hal-driver.git
    GIT_TAG v1.1.8
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_Declare(
    stm32_usb_device
    GIT_REPOSITORY https://github.com/STMicroelectronics/stm32-mw-usb-device.git
    GIT_TAG v2.11.5
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(cmsis_core cmsis_device_f1 stm32f1_hal stm32_usb_device)

add_library(stm32_cmsis_core INTERFACE)
add_library(stm32::cmsis_core ALIAS stm32_cmsis_core)
target_include_directories(stm32_cmsis_core SYSTEM INTERFACE
    "${cmsis_core_SOURCE_DIR}/Include"
)

add_library(stm32_cmsis_device INTERFACE)
add_library(stm32::cmsis_device ALIAS stm32_cmsis_device)
target_link_libraries(stm32_cmsis_device INTERFACE stm32::cmsis_core)
target_include_directories(stm32_cmsis_device SYSTEM INTERFACE
    "${cmsis_device_f1_SOURCE_DIR}/Include"
)
target_compile_definitions(stm32_cmsis_device INTERFACE STM32F103xB)

add_library(stm32_hal STATIC
    "${stm32f1_hal_SOURCE_DIR}/Src/stm32f1xx_hal.c"
    "${stm32f1_hal_SOURCE_DIR}/Src/stm32f1xx_hal_cortex.c"
    "${stm32f1_hal_SOURCE_DIR}/Src/stm32f1xx_hal_dma.c"
    "${stm32f1_hal_SOURCE_DIR}/Src/stm32f1xx_hal_flash.c"
    "${stm32f1_hal_SOURCE_DIR}/Src/stm32f1xx_hal_flash_ex.c"
    "${stm32f1_hal_SOURCE_DIR}/Src/stm32f1xx_hal_gpio.c"
    "${stm32f1_hal_SOURCE_DIR}/Src/stm32f1xx_hal_rcc.c"
    "${stm32f1_hal_SOURCE_DIR}/Src/stm32f1xx_hal_rcc_ex.c"
    "${stm32f1_hal_SOURCE_DIR}/Src/stm32f1xx_hal_tim.c"
    "${stm32f1_hal_SOURCE_DIR}/Src/stm32f1xx_hal_tim_ex.c"
    "${stm32f1_hal_SOURCE_DIR}/Src/stm32f1xx_hal_adc.c"
    "${stm32f1_hal_SOURCE_DIR}/Src/stm32f1xx_hal_adc_ex.c"
    "${stm32f1_hal_SOURCE_DIR}/Src/stm32f1xx_hal_pcd.c"
    "${stm32f1_hal_SOURCE_DIR}/Src/stm32f1xx_hal_pcd_ex.c"
    "${stm32f1_hal_SOURCE_DIR}/Src/stm32f1xx_ll_usb.c"
)
add_library(stm32::hal ALIAS stm32_hal)

add_library(stm32_usb_device_lib STATIC
    "${stm32_usb_device_SOURCE_DIR}/Core/Src/usbd_core.c"
    "${stm32_usb_device_SOURCE_DIR}/Core/Src/usbd_ctlreq.c"
    "${stm32_usb_device_SOURCE_DIR}/Core/Src/usbd_ioreq.c"
    "${stm32_usb_device_SOURCE_DIR}/Class/CDC/Src/usbd_cdc.c"
)
add_library(stm32::usb_lib ALIAS stm32_usb_device_lib)
target_link_libraries(stm32_usb_device_lib PUBLIC stm32::hal)

target_link_libraries(stm32_hal PUBLIC stm32::cmsis_device)
target_include_directories(stm32_hal
    PUBLIC
        "${STM32_DEVICES_CONFIG_DIR}"
        "${stm32f1_hal_SOURCE_DIR}/Inc"
        "${stm32f1_hal_SOURCE_DIR}/Inc/Legacy"
)
target_include_directories(stm32_usb_device_lib
    PUBLIC
        "${stm32_usb_device_SOURCE_DIR}/Core/Inc"
        "${stm32_usb_device_SOURCE_DIR}/Class/CDC/Inc"
)
target_compile_definitions(stm32_hal PUBLIC USE_HAL_DRIVER STM32F103xB)

set(FREERTOS_PORT GCC_ARM_CM3 CACHE STRING "" FORCE)
set(FREERTOS_HEAP 4 CACHE STRING "" FORCE)
FetchContent_Declare(freertos_kernel
    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
    GIT_TAG V11.1.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(freertos_kernel)
target_link_libraries(freertos_config INTERFACE stm32::cmsis_device)
