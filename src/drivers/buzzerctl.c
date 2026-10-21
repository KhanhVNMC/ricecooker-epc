#include "include/xthreads.h"
#include "include/rchal.h"
#include "include/buzzerctl.h"

void task_buzzer_update_thread(void *args) {
    while (1) {
        xthreads_wait(XTHREAD_WAIT_ONE); // this can play consecutively
        // play the buzzer for 100ms each
        // TODO DOREMI
        rc_buzzer_tone(261);
        xthreads_sleep(500);
        rc_buzzer_tone(293);
        xthreads_sleep(500);
        rc_buzzer_tone(329);
        xthreads_sleep(500);
        rc_buzzer_tone(349);
        xthreads_sleep(500);
        rc_buzzer_tone(392);
        xthreads_sleep(500);
        rc_buzzer_tone(0);
    }
}