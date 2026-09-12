#include <stdlib.h>

#include "ricecooker.h"
#include "include/rchal.h"
#include "include/xthreads.h"
#include "include/platform.h"
#include "include/mem.h"
#include "include/tm1637dv.h"
#include "include/heaterctl.h"

static constexpr digit_t BLANK[4] = {CHAR_NULL, CHAR_NULL, CHAR_NULL, CHAR_NULL};
static constexpr digit_t IDLE[4] = {CHAR_I, CHAR_D, CHAR_L, CHAR_E};
static constexpr digit_t ERRO[4] = {CHAR_E, CHAR_R, CHAR_R, CHAR_O};

static constexpr digit_t mode_to_digits[][4] = {
    [MODE_RICE_NORMAL] = {CHAR_R, CHAR_I, CHAR_C, CHAR_E},
    [MODE_FAST_RICE] = {CHAR_F, CHAR_A, CHAR_S, CHAR_T},
    [MODE_PORRIDGE] = {CHAR_P, CHAR_O, CHAR_R, CHAR_R},
    [MODE_SOFT_SIMMER] = {CHAR_S, CHAR_O, CHAR_F, CHAR_T},
    [MODE_FLAN_DRE] = {CHAR_F, CHAR_L, CHAR_A, CHAR_N},
    [MODE_HOLD_WARM] = {CHAR_H, CHAR_O, CHAR_L, CHAR_D},
    [MODE_MANUAL_USER] = {CHAR_U, CHAR_S, CHAR_E, CHAR_R},
    [MODE_DELAYED_EXEC] = {CHAR_D, CHAR_L, CHAR_A, CHAR_Y},
};

// state machines shit
static app_state_t app_state;
static cooking_mode_t cooking_mode;

// frontdecl functions
void reset_app_state(void);
void update_rc_display(void);

// masks & shit
static bitmask8_t last_input_mask = 0;

void app_on_btns_press(const bitmask8_t pressed) {
    if (pressed & RC_BTN_CANCEL) {
        reset_app_state();
        return;
    }
    switch (app_state) {
        case STATE_IDLE: {
            // pressing OPTIONS here take to mode select menu
            if (pressed & RC_BTN_OPTIONS) {
                app_state = STATE_MODE_SELECT;
                cooking_mode = MODE_RICE_NORMAL;
                break;
            }
            break;
        }
        case STATE_MODE_SELECT: {
            // wraparounds for less clunky navigation
            if (pressed & RC_BTN_INCR) {
                cooking_mode = (cooking_mode + 1) % COUNT_COOKING_MODES;
            } else if (pressed & RC_BTN_DECR) {
                cooking_mode = ((cooking_mode - 1) + COUNT_COOKING_MODES) % COUNT_COOKING_MODES;
            } else if (pressed & RC_BTN_SELECT) {

            }
            break;
        }
        case STATE_COOKING: {
            break;
        }
        case STATE_HOLD_WARM: {
            break;
        }
        case STATE_DELAYED_COUNTDOWN: {
            break;
        }
        case STATE_CF_MANUAL_TEMP: {
            break;
        }
        case STATE_CF_MANUAL_TIME: {
            break;
        }
        case STATE_CF_DELAYED_TIME: {
            break;
        }
        case STATE_CF_DELAYED_MODE_SELECT: {
            break;
        }
        default: break; // prevent complaining
    }
}

void reset_app_state() {
    app_state = STATE_IDLE;
    cooking_mode = MODE_RICE_NORMAL;
    display_set_characters(IDLE, false);
}

void update_rc_display(void) {
    switch (app_state) {
        case STATE_IDLE: {
            display_set_characters(IDLE, false);
            break;
        }
        case STATE_CF_DELAYED_MODE_SELECT:
        case STATE_MODE_SELECT: {
            display_set_characters(
                (rc_time_current_millis() % 1000u) < 500u // blink every 500ms
                ? mode_to_digits[cooking_mode]
                : BLANK, false
            );
            break;
        }
        case STATE_COOKING: {
            break;
        }
        case STATE_HOLD_WARM: {
            display_set_characters(mode_to_digits[MODE_HOLD_WARM], false);
            break;
        }
        case STATE_DELAYED_COUNTDOWN: {
            break;
        }
        case STATE_CF_MANUAL_TEMP: {
            break;
        }
        case STATE_CF_MANUAL_TIME: {
            break;
        }
        case STATE_CF_DELAYED_TIME: {
            break;
        }
        default: break; // prevent complaining
    }
}

void task_main_app() {
    while (1) {
        // input
        const bitmask8_t current_mask = rc_buttons_get_state();
        if (last_input_mask != current_mask) {
            // current && !last
            // only fire event on btn press (not release)
            const bitmask8_t changed = current_mask & ~last_input_mask;
            if (changed) {
                app_on_btns_press(changed);
            }
            last_input_mask = current_mask;
        }
        // output
        update_rc_display();
        xthreads_sleep(10);
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

    reset_app_state();
    task_main_app();
}