#include "ricecooker.h"
#include "include/heaterctl.h"
#include "include/xthreads.h"
#include "include/platform.h"
#include "include/tm1637dv.h"

// threads
static xthread_handle_t *MAIN_APP_THREAD;
static xthread_handle_t *HEATER_UPDATE_THREAD;

void create_app_threads() {
    HEATER_UPDATE_THREAD = xthreads_new(task_heater_update_thread, "Adolf Heater", nullptr);
    MAIN_APP_THREAD = xthreads_new(task_main_app_thread, "HerMAINn Goring", nullptr);
}

void platform_main(void) {
    platform_init_primitives(); // init GPIO and shit
    rc_init_rchal(); // init RCHAL
    display_normalize_segments();

    display_set_characters((digit_t[4]) {1, 2, 3, 4}, false);
    display_set_enabled(true);
    const uint32_t intro_start = rc_time_current_millis();
    while (rc_time_current_millis() - intro_start < 1000u) {
        /* Wait one second on the HAL clock without the scheduler. */
    }

    // Force the instruction with the same input/output register; CLZ(8) is 28.
    uint32_t clz_result = 1u << 3;
    __asm__ volatile ("clz %0, %0" : "+r" (clz_result));
    display_set_characters((digit_t[4]) {
        (clz_result / 1000u) % 10u,
        (clz_result / 100u) % 10u,
        (clz_result / 10u) % 10u,
        clz_result % 10u
    }, false);

    for (;;) {}
    // ricecooker_init(); // init rice
    //
    // create_app_threads(); // create RTOS threads
    // platform_start_xthreads_sched(); // init scheduler
}
