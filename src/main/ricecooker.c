#include <stdlib.h>

#include "ricecooker.h"
#include "include/rchal.h"
#include "include/xthreads.h"
#include "include/platform.h"
#include "include/mem.h"
#include "include/tm1637dv.h"
#include "include/heaterctl.h"

#define CONFIG_TIME_MINUTES_STEP    1
#define CONFIG_TEMP_STEP            5
#define DEFAULT_MANUAL_MODE_TIME    30
#define DEFAULT_MANUAL_MODE_TEMP    80
#define DEFAULT_DELAYED_MODE_TIME   15
#define MIN_CONFIG_TIME_MINS        1
#define MAX_CONFIG_TIME_MINS        (99u * 60 + 59u)
#define MIN_CONFIG_TEMP             60
#define MAX_CONFIG_TEMP             105

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

//// APPLICATION STATES / FLAGS /////
// state machines shit
static app_state_t app_state;
static cooking_mode_t cooking_mode;

static uint8_t  manual_mode_temp;
static uint16_t manual_mode_time_mins;

static bool     delayed_mode_active;
static uint16_t delayed_mode_time_mins;

static bool config_time_edit_minutes;

// timestamps
static uint32_t timestamp_cook_start;
static uint32_t timestamp_delayed_start;

// masks & shit
static bitmask8_t last_input_mask = 0;

//// APPLICATION FINITE STATE MACHINE /////
void begin_task_cooking(void);
void begin_task_delayed_countdown(void);
void on_cooking_pltick(void);
void on_cooking_complete(void);
void on_delayed_countdown_end(void);
void update_rc_display(void);
void on_platform_tick(void);

// util functions
static void config_adjust_time(uint16_t *time_mins_dest, bitmask8_t pressed);
static void app_display_countdown(uint32_t time_left_secs, bool blink);

void reset_app_state() {
    // FSM
    app_state = STATE_IDLE;
    cooking_mode = MODE_RICE_NORMAL;

    // default values for delay and manual
    manual_mode_time_mins = DEFAULT_MANUAL_MODE_TIME;
    manual_mode_temp = DEFAULT_MANUAL_MODE_TEMP;
    delayed_mode_time_mins = DEFAULT_DELAYED_MODE_TIME;
    delayed_mode_active = false;
    config_time_edit_minutes = true;

    // just in case
    timestamp_cook_start = 0;
    timestamp_delayed_start = 0;

    // default
    display_set_characters(IDLE, false);

    // heater
    heater_set_enabled(false);
}

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
            }
            break;
        }
        case STATE_CF_DELAYED_MODE_SELECT:
        case STATE_MODE_SELECT: {
            // press SELECT here either:
            if (pressed & RC_BTN_SELECT) {
                // if selected "DELAY", take to select delayed time
                if (cooking_mode == MODE_DELAYED_EXEC) {
                    app_state = STATE_CF_DELAYED_TIME;
                    cooking_mode = MODE_RICE_NORMAL; // ensure valid mode
                    config_time_edit_minutes = true;
                    break;
                }
                // if selected "MANUAL", take to set temp
                if (cooking_mode == MODE_MANUAL_USER) {
                    app_state = STATE_CF_MANUAL_TEMP;
                    break;
                }
                // if "DELAY" configured complete -> exec delay code
                if (delayed_mode_active) {
                    begin_task_delayed_countdown();
                    break;
                }
                // else exec normal cooking proc
                begin_task_cooking();
                break;
            }
            // in delayed mode, you cannot select delayed again
            const uint8_t mode_count = delayed_mode_active ? MODE_DELAYED_EXEC : COUNT_COOKING_MODES;
            // wraparounds for less clunky navigation
            if (pressed & RC_BTN_INCR) {
                cooking_mode = (cooking_mode + 1) % mode_count;
            } else if (pressed & RC_BTN_DECR) {
                cooking_mode = ((cooking_mode - 1) + mode_count) % mode_count;
            }
            break;
        }
        case STATE_CF_DELAYED_TIME: {
            if (pressed & RC_BTN_SELECT) {
                // delayed mode configured complete
                delayed_mode_active = true;
                cooking_mode = MODE_RICE_NORMAL; // default mode
                app_state = STATE_CF_DELAYED_MODE_SELECT;
                break;
            }
            if (pressed & RC_BTN_OPTIONS) {
                // swap between editing min/hour
                config_time_edit_minutes = !config_time_edit_minutes;
                break;
            }
            config_adjust_time(&delayed_mode_time_mins, pressed);
            break;
        }
        case STATE_CF_MANUAL_TIME: {
            if (pressed & RC_BTN_SELECT) {
                // if "DELAY" configured before -> exec delay code
                if (delayed_mode_active) {
                    begin_task_delayed_countdown();
                    break;
                }
                // else exec normal cooking proc
                begin_task_cooking();
                break;
            }
            if (pressed & RC_BTN_OPTIONS) {
                // swap between editing min/hour
                config_time_edit_minutes = !config_time_edit_minutes;
                break;
            }
            config_adjust_time(&manual_mode_time_mins, pressed);
            break;
        }
        case STATE_CF_MANUAL_TEMP: {
            if (pressed & RC_BTN_SELECT) {
                // after selecting temp, select time 4 manual
                app_state = STATE_CF_MANUAL_TIME;
                break;
            }
            if (pressed & RC_BTN_INCR) {
                if (manual_mode_temp + CONFIG_TEMP_STEP <= MAX_CONFIG_TEMP) {
                    manual_mode_temp += CONFIG_TEMP_STEP;
                }
                rc_buzzer_play();
            } else if (pressed & RC_BTN_DECR) {
                if (manual_mode_temp >= CONFIG_TEMP_STEP + MIN_CONFIG_TEMP) {
                    manual_mode_temp -= CONFIG_TEMP_STEP;
                }
                rc_buzzer_play();
            }
            break;
        }
        case STATE_COOKING: {
            break;
        }
        default: break; // prevent complaining
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
                ? mode_to_digits[cooking_mode]: BLANK,
                (app_state == STATE_CF_DELAYED_MODE_SELECT) // : if delayed
            );
            break;
        }
        case STATE_COOKING: {
            const uint32_t current_time = rc_time_current_millis();
            // SHOW TIME LEFT: for manual mode every 5s (from cooking start)
            if (cooking_mode == MODE_MANUAL_USER && ((current_time - timestamp_cook_start) / 5000u) & 1u) {
                const uint32_t duration_ms = manual_mode_time_mins * 60000u;
                app_display_countdown(
                    ((timestamp_cook_start + duration_ms) - current_time) / 1000u,
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
            if (((current_time - timestamp_delayed_start) / 5000u) & 1u) {
                const uint32_t duration_ms = delayed_mode_time_mins * 60000u;
                app_display_countdown(
                    ((timestamp_delayed_start + duration_ms) - current_time) / 1000u,
                    (current_time / 500u) & 1u
                );
                break;
            }
            // SHOW "DLAY" in between
            display_set_characters(mode_to_digits[MODE_DELAYED_EXEC], false);
            break;
        }
        case STATE_CF_MANUAL_TEMP: {
            if (manual_mode_temp <= 99) {
                // for temp <= 99: [99oC]
                display_set_characters((digit_t[4]) {
                    manual_mode_temp / 10, manual_mode_temp % 10,
                    SCHAR_DEG, CHAR_C
                } ,false);
            } else {
                // for temp > 99: [100o]
                display_set_characters((digit_t[4]) {
                    manual_mode_temp / 100,
                    (manual_mode_temp / 10) % 10,
                    manual_mode_temp % 10,
                    SCHAR_DEG
                } ,false);
            }
            break;
        }
        case STATE_CF_MANUAL_TIME:
        case STATE_CF_DELAYED_TIME: {
            const bool blink = (rc_time_current_millis() / 500u) & 1u;
            const uint32_t time_source_mins = app_state == STATE_CF_MANUAL_TIME
                ? manual_mode_time_mins : delayed_mode_time_mins
            ;
            const uint8_t display_hours = time_source_mins / 60;
            const uint8_t display_mins = time_source_mins % 60;
            // blink the editing one
            if (config_time_edit_minutes) {
                display_set_characters((digit_t[4]) {
                    display_hours / 10, display_hours % 10,
                    blink ? (display_mins / 10) : CHAR_NULL, blink ? (display_mins % 10) : CHAR_NULL
                }, true);
            } else {
                display_set_characters((digit_t[4]) {
                    blink ? (display_hours / 10) : CHAR_NULL, blink ? (display_hours % 10) : CHAR_NULL,
                    display_mins / 10, display_mins % 10,
                }, true);
            }
            break;
        }
        case STATE_HOLD_WARM: {
            display_set_characters(mode_to_digits[MODE_HOLD_WARM], false);
            break;
        }
        default: break; // prevent complaining
    }
}

static void config_adjust_time(uint16_t *time_mins_dest, const bitmask8_t pressed) {
    const uint16_t to_change = (config_time_edit_minutes ? CONFIG_TIME_MINUTES_STEP : 60);
    if (pressed & RC_BTN_INCR) {
        if (*time_mins_dest + to_change <= MAX_CONFIG_TIME_MINS) {
            *time_mins_dest += to_change;
        }
        // todo: beep
    } else if (pressed & RC_BTN_DECR) {
        if (*time_mins_dest >= to_change + MIN_CONFIG_TIME_MINS) {
            *time_mins_dest -= to_change;
        }
        // todo: play beep?
    }
}

static inline void app_display_countdown(const uint32_t time_left_secs, const bool blink) {
    const uint32_t time_left_mins = time_left_secs / 60;

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

void task_main_app_thread(void *args) {
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
        on_platform_tick();
        xthreads_sleep(10);
    }
}

///// BUSINESS LOGIC LAYER /////
void begin_task_delayed_countdown(void) {
    timestamp_delayed_start = rc_time_current_millis();
    app_state = STATE_DELAYED_COUNTDOWN;
}

void on_delayed_countdown_end(void) {
    begin_task_cooking();
}

void on_platform_tick(void) {
    const uint32_t current_time = rc_time_current_millis();
    // trigger the job at the end of the delay tunnel
    if (app_state == STATE_DELAYED_COUNTDOWN) {
        const uint32_t dl_duration_ms = delayed_mode_time_mins * 60000u;
        if (current_time - timestamp_delayed_start >= dl_duration_ms) {
            delayed_mode_active = false;
            on_delayed_countdown_end();
        }
    }
    if (app_state == STATE_COOKING || app_state == STATE_HOLD_WARM) {
        on_cooking_pltick();
    }
    // update output
    update_rc_display();
}

// TODO MAKE THIS WORK PROPERLY
void begin_task_cooking(void) {
    timestamp_cook_start = rc_time_current_millis();
    app_state = STATE_COOKING;
}

// TODO MAKE THIS WORK PROPERLY
void on_cooking_pltick(void) {

}

// TODO MAKE THIS WORK PROPERLY
void on_cooking_complete(void) {

}

///// PLATFORM CONTROL SHIT /////
// threads
static xthread_handle_t HEATER_UPDATE_THREAD;
static xthread_handle_t MAIN_APP_THREAD;

void create_app_threads() {
    HEATER_UPDATE_THREAD = xthreads_new(task_heater_update_thread, NULL);
    MAIN_APP_THREAD = xthreads_new(task_main_app_thread, NULL);
}

void platform_main(void) {
    rc_init_hal(); // init HAL and RCHAL
    platform_init_primitives(); // init GPIO and shit

    display_set_enabled(true); // turn the screen on
    reset_app_state(); // initial boot

    create_app_threads(); // create RTOS threads
    platform_start_xthreads_sched(); // init scheduler
}