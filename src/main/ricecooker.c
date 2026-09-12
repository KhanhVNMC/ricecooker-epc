#include <stdlib.h>

#include "ricecooker.h"
#include "include/rchal.h"
#include "include/xthreads.h"
#include "include/platform.h"
#include "include/mem.h"
#include "include/tm1637dv.h"
#include "include/heaterctl.h"

// digit consts
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

// spinny icon
static constexpr bitmask8_t SPINNY_LEFT = RC_SEG_A | RC_SEG_F | RC_SEG_E | RC_SEG_D;
static constexpr bitmask8_t SPINNY_MID = RC_SEG_A | RC_SEG_D;
static constexpr bitmask8_t SPINNY_RIGHT = RC_SEG_A | RC_SEG_B | RC_SEG_C | RC_SEG_D;

static constexpr spinny_step_t ANIMATION_SPINNY_STEPS[13] = { // holy jank
    // - - -
    {0, RC_SEG_A},
    {1, RC_SEG_A},
    {2, RC_SEG_A},
    {3, RC_SEG_A},
    // | | v
    {3, RC_SEG_B},
    {3, RC_SEG_C},
    // _ _ _
    {3, RC_SEG_D},
    {2, RC_SEG_D},
    {1, RC_SEG_D},
    {0, RC_SEG_D},
    // | | ^
    {0, RC_SEG_E},
    {0, RC_SEG_F},
};

// state machines shit
static app_state_t app_state;
static cooking_mode_t cooking_mode;

static float    manual_mode_temp;
static uint16_t manual_mode_time_mins;

static uint16_t delayed_mode_time_mins;

// timestamps
static uint32_t cook_start_time;
static uint16_t delayed_mode_start_time;

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
                // TODO MAKE THIS SHIT WORK PROPERLY
                app_state = STATE_MODE_SELECT;
                cooking_mode = MODE_RICE_NORMAL;
                break;
            }
            break;
        }
        case STATE_MODE_SELECT: {
            if (pressed & RC_BTN_SELECT) {
                app_state = STATE_COOKING;
                cook_start_time = rc_time_current_millis();
                manual_mode_time_mins = 60 + 1;
                break;
            }
            // wraparounds for less clunky navigation
            if (pressed & RC_BTN_INCR) {
                cooking_mode = (cooking_mode + 1) % COUNT_COOKING_MODES;
            } else if (pressed & RC_BTN_DECR) {
                cooking_mode = ((cooking_mode - 1) + COUNT_COOKING_MODES) % COUNT_COOKING_MODES;
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

static inline void app_display_countdown(const uint16_t time_left_secs, const bool blink) {
    const uint8_t time_left_mins = time_left_secs / 60;

    const uint8_t display_hours = time_left_mins / 60;
    const uint8_t display_mins = time_left_mins % 60;
    const uint8_t display_seconds = time_left_secs % 60;

    if (display_hours > 0) {
        // HH:MM for >= 1 hour
        display_set_characters((digit_t[4]) {
            display_hours / 10, display_hours % 10,
            display_mins / 10, display_mins % 10
        }, blink);
    } else {
        // MM:SS for < 1 hour
        display_set_characters((digit_t[4]) {
            display_mins / 10, display_mins % 10,
            display_seconds / 10, display_seconds % 10
        }, blink);
    }
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
                (rc_time_current_millis() / 500u) & 1u // blink every 500ms
                ? mode_to_digits[cooking_mode]
                : BLANK, false
            );
            break;
        }
        case STATE_COOKING: {
            const uint32_t current_time = rc_time_current_millis();
            // SHOW TIME LEFT: for manual mode every 5s (from cooking start)
            if (cooking_mode == MODE_MANUAL_USER && ((current_time - cook_start_time) / 5000u) & 1u) {
                const uint32_t duration_ms = manual_mode_time_mins * 60000u;
                app_display_countdown(
                    ((cook_start_time + duration_ms) - current_time) / 1000u,
                    (current_time / 500u) & 1u
                );
                break;
            }
            // SHOW SPINNING ANIMATION for everything else
            uint8_t per_frame = 150u; // 250ms/frame for soft, 150 for normal and 50 for fast)
            if (cooking_mode == MODE_FAST_RICE) per_frame = 50u;
            else if (cooking_mode == MODE_SOFT_SIMMER) per_frame = 250u;

            // spinning animation (12 frames)
            const uint8_t frame = (current_time / per_frame) % 12u;
            const spinny_step_t step = ANIMATION_SPINNY_STEPS[frame];

            bitmask8_t segments[4] = {
                SPINNY_LEFT, SPINNY_MID, SPINNY_MID, SPINNY_RIGHT
            };
            segments[step.digit] ^= step.segment;
            display_send_raw_segments(segments);
            break;
        }
        case STATE_DELAYED_COUNTDOWN: {
            const uint32_t current_time = rc_time_current_millis();
            // SHOW TIME LEFT: for delayed mode every 5s (from delay start)
            if (((current_time - delayed_mode_start_time) / 5000u) & 1u) {
                const uint32_t duration_ms = delayed_mode_time_mins * 60000u;
                app_display_countdown(
                    ((delayed_mode_start_time + duration_ms) - current_time) / 1000u,
                    (current_time / 500u) & 1u
                );
                break;
            }
            // SHOW "DLAY" in between
            display_set_characters(mode_to_digits[MODE_DELAYED_EXEC], false);
            break;
        }
        case STATE_HOLD_WARM: {
            display_set_characters(mode_to_digits[MODE_HOLD_WARM], false);
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