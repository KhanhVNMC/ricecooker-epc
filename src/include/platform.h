#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>

typedef enum : uint8_t {
    HAL_INIT_OSCILLATOR,
    HAL_INIT_SYSCLK,
    HAL_INIT_US_SLEEP,
    HAL_INIT_GPIO_ANALOG,
    HAL_INIT_PWM_GEN,
    XTHREADS_CREATION,
    APPLICATION_INIT,
    SCHEDULER_INIT,
} platform_error_t;

// bootstrap
void platform_xscheduler_start(void);

// entrypoint
void platform_main(void);

// error handling
void platform_on_error(platform_error_t err);

#endif //PLATFORM_H
