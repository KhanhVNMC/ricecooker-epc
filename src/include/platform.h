#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>

typedef enum : uint8_t {
    HAL_INIT_OSCILLATOR,
    HAL_INIT_SYSCLK,
    HAL_INIT_US_SLEEP,
    APPLICATION_INIT,
    SCHEDULER_INIT,
} platform_error_t;

// bootstrap
void platform_init_primitives(void);
void platform_start_xthreads_sched(void);

// entrypoint
void platform_main(void);

// error handling
void platform_on_error(platform_error_t err);

#endif //PLATFORM_H
