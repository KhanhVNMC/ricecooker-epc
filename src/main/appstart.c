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

static void create_app_threads() {
    HEATER_UPDATE_THREAD = xthreads_new(task_heater_update_thread, nullptr, OBERSTER_FUHRER_DER_SS, "Adolf Heater");
    MAIN_APP_THREAD = xthreads_new(task_main_app_thread, nullptr, SS_OBERFUHRER, "Hermainn Goring"); // close enough
    BUZZER_UPDATE_THREAD = xthreads_new(task_buzzer_update_thread, nullptr, SS_STANDARTENFUHRER, "Joachim Buzzer");
}

void platform_main(void) {
    display_normalize_segments(); // prevent ghosting
    ricecooker_init(); // init application
    create_app_threads(); // create RTOS threads
    platform_start_xthreads_sched(); // init scheduler
}

void buzzer_play_beep(void) {
    xthreads_notify(BUZZER_UPDATE_THREAD);
}
