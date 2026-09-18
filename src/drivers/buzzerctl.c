#include "include/xthreads.h"
#include "include/rchal.h"
#include "include/buzzerctl.h"

void task_buzzer_update_thread(void *args) {
    while (1) {
        xthreads_wait(XTHREAD_WAIT_ONE); // this can play consecutively
        // play the buzzer for 100ms each
        rc_buzzer_set(true);
        xthreads_sleep(100);
        rc_buzzer_set(false);
    }
}