#include <stdlib.h>

#include "include/rchal.h"
#include "include/xthreads.h"
#include "include/platform.h"
#include "include/mem.h"
#include "include/tm1637dv.h"
#include "include/heaterctl.h"

static constexpr digit_t IDLE[4] = {CHAR_I, CHAR_D, CHAR_L, CHAR_E};
static constexpr digit_t HOLD[4] = {CHAR_H, CHAR_O, CHAR_L, CHAR_D};
static constexpr digit_t RICE[4] = {CHAR_R, CHAR_I, CHAR_C, CHAR_E};
static constexpr digit_t SOFT[4] = {CHAR_S, CHAR_O, CHAR_F, CHAR_T};
static constexpr digit_t FAST[4] = {CHAR_F, CHAR_A, CHAR_S, CHAR_T};
static constexpr digit_t PORR[4] = {CHAR_P, CHAR_O, CHAR_R, CHAR_R};
static constexpr digit_t USER[4] = {CHAR_U, CHAR_S, CHAR_E, CHAR_R};
static constexpr digit_t DLAY[4] = {CHAR_D, CHAR_L, CHAR_A, CHAR_Y};
static constexpr digit_t ERRO[4] = {CHAR_E, CHAR_R, CHAR_R, CHAR_O};

static xthread_handle_t HEATER_UPDATE_THREAD;

void init_threads() {
    HEATER_UPDATE_THREAD = xthreads_new(task_heater_update, NULL);
}

void platform_main(void) {
    rc_init_hal();
    init_threads();

    display_set_enabled(true);
    display_set_characters((digit_t[4]) { 3, 6, CHAR_DEGREE, CHAR_C }, false);
    display_set_characters(IDLE, false);
    display_set_characters(RICE, false);
    display_set_characters(SOFT, false);
    display_set_characters(HOLD, false);
    display_set_characters(FAST, false);
    display_set_characters(PORR, false);
    display_set_characters(USER, false);
    display_set_characters(DLAY, false);
    display_set_characters(ERRO, false);

    while (1) {}
}