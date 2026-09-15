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

    ricecooker_init(); // init rice

    create_app_threads(); // create RTOS threads
    platform_start_xthreads_sched(); // init scheduler
}