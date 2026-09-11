#include <stdlib.h>

#include "include/rchal.h"
#include "include/xthreads.h"
#include "include/platform.h"
#include "include/mem.h"
#include "include/tm1637dv.h"
#include "include/heaterctl.h"

static xthread_handle_t HEATER_UPDATE_THREAD;

void init_threads() {
    HEATER_UPDATE_THREAD = xthreads_new(task_heater_update, NULL);
}

void platform_main(void) {
    rc_init_hal();
    init_threads();
    heater_set_enabled(true);
    heater_set_target_temp(100);

    while (1) {}
}