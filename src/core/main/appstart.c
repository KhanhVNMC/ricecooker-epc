#include "ricecooker.h"
#include "include/heaterctl.h"
#include "include/buzzerctl.h"
#include "include/xthreads.h"
#include "include/platform.h"
#include "include/tm1637dv.h"

// threads
static xthread_handle_t *MAIN_APP_THREAD;
static xthread_handle_t *HEATER_UPDATE_THREAD;
static xthread_handle_t *BUZZER_UPDATE_THREAD;

static void app_threads_init(void) {
    // [name]::[identifier_digits]
    HEATER_UPDATE_THREAD = xthreads_new(
        task_heater_update_thread, nullptr,
        XTHREADS_CHANCELLOR, "Heater::8945"
    ); // most critical thread
    MAIN_APP_THREAD = xthreads_new(
        task_main_app_thread, nullptr,
        XTHREADS_PRIO_HIGH, "Main::9346"
    ); // 2nd most critical thread
    BUZZER_UPDATE_THREAD = xthreads_new(
        task_buzzer_update_thread, nullptr,
        XTHREADS_PRIO_MEDIUM, "Peeper::1576"
    ); // beep beep

    if (!HEATER_UPDATE_THREAD || !MAIN_APP_THREAD || !BUZZER_UPDATE_THREAD) {
        platform_on_error(XTHREADS_CREATION);
    }
}

void platform_main(void) {
    display_normalize_segments(); // prevent ghosting
    buzzer_sys_init(); // init buzzer system (mutex)
    ricecooker_init(); // init application
    app_threads_init(); // create RTOS threads
    platform_xscheduler_start(); // init scheduler
}