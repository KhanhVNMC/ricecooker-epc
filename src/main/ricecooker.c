#include <stdlib.h>

#include "ricecooker.h"
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
static constexpr digit_t FLAN[4] = {CHAR_F, CHAR_L, CHAR_A, CHAR_N};
static constexpr digit_t USER[4] = {CHAR_U, CHAR_S, CHAR_E, CHAR_R};
static constexpr digit_t DLAY[4] = {CHAR_D, CHAR_L, CHAR_A, CHAR_Y};
static constexpr digit_t ERRO[4] = {CHAR_E, CHAR_R, CHAR_R, CHAR_O};

// state machines
static app_state_t app_state = STATE_IDLE;
static cooking_mode_t cooking_mode = MODE_RICE_NORMAL;

// masks & shit
static bitmask8_t last_input_mask = 0;

void app_on_btns_press(const bitmask8_t pressed) {
    if (pressed & RC_BTN_SELECT) {
        display_set_characters(FLAN, false);
    }

    if (pressed & RC_BTN_CANCEL) {
        display_set_characters(IDLE, false);
    }
}

void task_main_app() {
    while (1) {
        const bitmask8_t current_mask = rc_buttons_get_state();
        if (last_input_mask != current_mask) {
            // current && !last
            // only fire event on btn press (not release)
            if (current_mask & ~last_input_mask) {
                app_on_btns_press(current_mask);
            }
            last_input_mask = current_mask;
        }
    }
}

// threads
static xthread_handle_t HEATER_UPDATE_THREAD;

void init_threads() {
    HEATER_UPDATE_THREAD = xthreads_new(task_heater_update, NULL);
}

void platform_main(void) {
    rc_init_hal();
    init_threads();

    display_set_enabled(true);
    task_main_app();
}