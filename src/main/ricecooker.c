#include "tempstats.h"
#include "ricecooker.h"
#include "cookingprofile.h"
#include "include/cmath.h"
#include "include/rchal.h"
#include "include/xthreads.h"
#include "include/tm1637dv.h"
#include "include/heaterctl.h"
#include "include/buzzerctl.h"
#include "include/lrcmdctl.h"

#define APP_MAIN_INTERVAL_MS        (10)
#define APP_BLINK_INTERVAL_MS       (500u)
#define APP_ANIM_FRAME_FAST_MS      (200u)
#define APP_ANIM_FRAME_NORMAL_MS    (250u)
#define APP_ANIM_FRAME_SOFT_MS      (300u)

#define CONFIG_TIME_MINUTES_STEP    (1)
#define CONFIG_TEMP_STEP            (5)
#define DEFAULT_MANUAL_MODE_TIME    (30)
#define DEFAULT_MANUAL_MODE_TEMP    (80)
#define DEFAULT_DELAYED_MODE_TIME   (15)
#define MIN_CONFIG_TIME_MINS        (1)
#define MAX_CONFIG_TIME_MINS        (99u * 60 + 59u)
#define MIN_CONFIG_TEMP             (60)
#define MAX_CONFIG_TEMP             (105)

// digit consts
static constexpr digit_t BLANK[4] = {CHAR_NULL, CHAR_NULL, CHAR_NULL, CHAR_NULL};
static constexpr digit_t IDLE[4] = {CHAR_I, CHAR_D, CHAR_L, CHAR_E};

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

static constexpr spinny_step_t ANIMATION_SPINNY_STEPS[12] = { // holy jank
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
static app_state_t    app_state;
static cooking_mode_t cooking_mode;

static uint8_t  manual_mode_temp;
static uint16_t manual_mode_time_mins;

static bool     delayed_mode_active;
static uint16_t delayed_mode_time_mins;

static bool config_time_edit_minutes;

// error detection policy & shit
static uint8_t system_error_code;

// timestamps
static uint32_t timestamp_cook_start;
static uint32_t timestamp_delayed_start;
static uint32_t timestamp_pot_temp_display_start; // cosmetic
static uint32_t timestamp_tempstats_last_sample; // temp sample

// thermal cooking profile tracking
static const cooking_profile_t *active_profile = nullptr;
static uint8_t  active_stage_idx = 0;
static uint32_t timestamp_stage_start = 0;

// masks & shit
static bitmask8_t last_input_mask = 0;

//// APPLICATION FINITE STATE MACHINE /////
void begin_task_cooking(void);
void begin_task_delayed_countdown(void);
void on_cooking_pltick(uint32_t current_time);
void on_cooking_complete(void);
void on_delayed_countdown_end(void);
void update_rc_display(void);
void on_platform_tick(void);
void on_cmd_datagram_recv(cmd_type_t cmd_type, uint16_t payload_size, const uint8_t *payload);

// util functions
static void config_adjust_time(uint16_t *time_mins_dest, bitmask8_t pressed);
static void app_display_countdown(uint32_t time_left_secs, bool blink);
static void app_display_temperature(uint16_t temp);

void reset_app_state(const app_state_t reset_to) {
    // heater
    heater_set_enabled(false);

    // FSM reset
    app_state = reset_to;
    cooking_mode = MODE_RICE_NORMAL;
    if (reset_to != STATE_SYSTEM_ERROR) {
        system_error_code = 0; // no "errors"
        heater_clear_faults();
    }

    // default values for delay and manual + more
    manual_mode_time_mins = DEFAULT_MANUAL_MODE_TIME;
    manual_mode_temp = DEFAULT_MANUAL_MODE_TEMP;
    delayed_mode_time_mins = DEFAULT_DELAYED_MODE_TIME;
    delayed_mode_active = false;
    config_time_edit_minutes = true;
    tempstats_reset();

    // just in case
    timestamp_cook_start = 0;
    timestamp_delayed_start = 0;
    timestamp_pot_temp_display_start = 0;
    timestamp_tempstats_last_sample = 0;
    active_profile = nullptr;
    active_stage_idx = 0;
    timestamp_stage_start = 0;
}

void app_on_btns_press(const bitmask8_t pressed) {
    if (pressed & RC_BTN_CANCEL) {
        reset_app_state(STATE_IDLE); // ignore errors for now
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
        case STATE_CF_MANUAL_TEMP: {
            if (pressed & RC_BTN_SELECT) {
                // after selecting temp, select time 4 manual
                app_state = STATE_CF_MANUAL_TIME;
                config_time_edit_minutes = true;
                break;
            }
            if (pressed & RC_BTN_INCR) {
                if (manual_mode_temp + CONFIG_TEMP_STEP <= MAX_CONFIG_TEMP) {
                    manual_mode_temp += CONFIG_TEMP_STEP;
                }
            } else if (pressed & RC_BTN_DECR) {
                if (manual_mode_temp >= CONFIG_TEMP_STEP + MIN_CONFIG_TEMP) {
                    manual_mode_temp -= CONFIG_TEMP_STEP;
                }
            }
            break;
        }
        case STATE_CF_MANUAL_TIME: {
            if (pressed & RC_BTN_SELECT) {
                // if "DELAY" configured before -> exec delay code
                if (delayed_mode_active) {
                    begin_task_delayed_countdown();
                    break;
                }
                // else exec normal cooking proc with flag set
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
        case STATE_COOKING: {
            if (pressed & RC_BTN_OPTIONS) {
                // this should display the temp screen for 3s
                timestamp_pot_temp_display_start = rc_time_current_millis();
            }
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
        case STATE_SYSTEM_ERROR: {
            const bool blink = (rc_time_current_millis() / APP_BLINK_INTERVAL_MS) & 1u;
            // display blinking [ERR:<code>]
            display_set_characters(blink ?
                (digit_t[4]) {CHAR_E, CHAR_R, system_error_code / 10, system_error_code % 10}
                : BLANK, blink
            );
            break;
        }
        case STATE_CF_DELAYED_MODE_SELECT:
        case STATE_MODE_SELECT: {
            display_set_characters(
                (rc_time_current_millis() / APP_BLINK_INTERVAL_MS) & 1u // blink every Xms
                ? mode_to_digits[cooking_mode] : BLANK,
                (app_state != STATE_MODE_SELECT) // : if delayed
            );
            break;
        }
        case STATE_COOKING: {
            const uint32_t current_time = rc_time_current_millis();
            // SHOW POT TEMPERATURE: if the user pressed OPTIONS during cooking
            if (current_time - timestamp_pot_temp_display_start < 3000u) {
                app_display_temperature(
                    dcf32u16(rc_sensor_get_temp(RC_SENSOR_POT) + 0.5f, 250)
                );
                break;
            }

            // SHOW TIME LEFT: for manual mode every 5s (from cooking start)
            if (cooking_mode == MODE_MANUAL_USER && ((current_time - timestamp_cook_start) / 5000u) & 1u) {
                const uint32_t duration_ms = manual_mode_time_mins * 60000u;
                app_display_countdown(
                    ((timestamp_cook_start + duration_ms) - current_time) / 1000u,
                    (current_time / APP_BLINK_INTERVAL_MS) & 1u
                );
                break;
            }

            // SHOW SPINNING ANIMATION for everything else
            uint16_t per_frame = APP_ANIM_FRAME_NORMAL_MS;
            if (cooking_mode == MODE_FAST_RICE) per_frame = APP_ANIM_FRAME_FAST_MS;
            else if (cooking_mode == MODE_SOFT_SIMMER) per_frame = APP_ANIM_FRAME_SOFT_MS;

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
                    (current_time / APP_BLINK_INTERVAL_MS) & 1u
                );
                break;
            }
            // SHOW "DLAY" in between
            display_set_characters(mode_to_digits[MODE_DELAYED_EXEC], false);
            break;
        }
        case STATE_CF_MANUAL_TEMP: {
            app_display_temperature(manual_mode_temp);
            break;
        }
        case STATE_CF_MANUAL_TIME:
        case STATE_CF_DELAYED_TIME: {
            const bool blink = (rc_time_current_millis() / APP_BLINK_INTERVAL_MS) & 1u;
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
    } else if (pressed & RC_BTN_DECR) {
        if (*time_mins_dest >= to_change + MIN_CONFIG_TIME_MINS) {
            *time_mins_dest -= to_change;
        }
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

static inline void app_display_temperature(const uint16_t temp) {
    // for temp <= 99: [99oC]
    if (temp <= 99u) {
        display_set_characters((digit_t[4]) {
            temp / 10u, temp % 10u,
            SCHAR_DEG, CHAR_C
        }, false);
    } else {
        // for temp > 99: [100o]
        display_set_characters((digit_t[4]) {
            temp / 100u,
            (temp / 10u) % 10u,
            temp % 10u,
            SCHAR_DEG
        }, false);
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
                buzzer_play_beep();
                app_on_btns_press(changed);
            }
            last_input_mask = current_mask;
        }
        on_platform_tick();
        xthreads_sleep(APP_MAIN_INTERVAL_MS);
    }
}

void begin_task_delayed_countdown(void) {
    timestamp_delayed_start = rc_time_current_millis();
    app_state = STATE_DELAYED_COUNTDOWN;
}

static void watchdog_platform_tick(const uint32_t current_time) {
    // watchdog for heater faults (error code 1x)
    const heater_faults_t hfaults = heater_get_faults();
    if (hfaults != 0) {
        system_error_code = 20; // base heater error code
        if (hfaults & HEATER_ELEMENT_OVERHEAT_FAULT) system_error_code += 2; // code 12
        if (hfaults & HEATER_POT_OVERHEAT_FAULT) system_error_code += 3; // code 13 (if both then 15)
        reset_app_state(STATE_SYSTEM_ERROR);
        return;
    }
    // collect pot temp every second
    if (current_time - timestamp_tempstats_last_sample >= 1000u) {
        tempstats_collect_sample(rc_sensor_get_temp(RC_SENSOR_POT));
        timestamp_tempstats_last_sample = current_time;
    }
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
    // platform tick while cooking
    if (app_state == STATE_COOKING || app_state == STATE_HOLD_WARM) {
        watchdog_platform_tick(current_time);
        on_cooking_pltick(current_time);
    }
    // update output
    update_rc_display();
    // process control datagrams
    if (lrcmdctl_ib_has_pending()) {
        on_cmd_datagram_recv(
            lrcmdctl_ib_get_cmd_type(),
            lrcmdctl_ib_payload_size(),
            lrcmdctl_ib_get_payload()
        );
        lrcmdctl_ib_release();
    }
}

void on_cmd_datagram_recv(const cmd_type_t cmd_type, const uint16_t payload_size, const uint8_t *payload) {

}

// TODO MAKE THIS WORK PROPERLY
void begin_task_cooking(void) {
    timestamp_cook_start = rc_time_current_millis();
    timestamp_stage_start = timestamp_cook_start;
    active_stage_idx = 0;
    app_state = STATE_COOKING;

    if (cooking_mode == MODE_MANUAL_USER) {
        active_profile = nullptr;
        heater_set_target_pot_temp((float)manual_mode_temp);
        heater_set_enabled(true);
    } else {
        active_profile = cooking_get_profile(cooking_mode);
        if (active_profile != nullptr && active_profile->stage_count > 0) {
            heater_set_target_pot_temp(active_profile->stages[0].target_pot_temp);
            heater_set_enabled(true);
        } else {
            // TODO THIS IS JUST TEST CODE
            // Fall back
            heater_set_target_pot_temp(100.f);
            heater_set_enabled(true);
        }
    }
}

// TODO MAKE THIS WORK PROPERLY
void on_cooking_pltick(const uint32_t current_time) {
    // TODO impl here
    if (app_state == STATE_HOLD_WARM) {
        heater_set_target_pot_temp(65.0f);
        heater_set_enabled(true);
        return;
    }

    if (cooking_mode == MODE_MANUAL_USER) {
        const uint32_t duration_ms = (uint32_t)manual_mode_time_mins * 60u * 1000u;
        if (current_time - timestamp_cook_start >= duration_ms) {
            on_cooking_complete();
        }
        return;
    }

    if (active_profile == nullptr || active_profile->stage_count == 0) {
        return;
    }

    const cooking_stage_t *curr_stage = &active_profile->stages[active_stage_idx];
    const uint32_t stage_elapsed_ms = current_time - timestamp_stage_start;
    const float pot_temp = rc_sensor_get_temp(RC_SENSOR_POT);
    bool advance_stage = false;

    switch (curr_stage->trigger_type) {
        case TRIGGER_TIME_EXPIRED: {
            if (stage_elapsed_ms >= curr_stage->ceiling_duration_ms) {
                advance_stage = true;
            }
            break;
        }
        case TRIGGER_POT_TEMP_REACHED: {
            if (pot_temp >= curr_stage->temp_threshold || stage_elapsed_ms >= curr_stage->ceiling_duration_ms) {
                advance_stage = true;
            }
            break;
        }
        case TRIGGER_WATER_BOIL_DRY: {
            // high temperature (have no water left)
            // high slope during 5 mins
            // timeout
            const float slope = tempstats_linreg_slope();
            const bool min_boil_time_met = stage_elapsed_ms >= 5u * 60u * 1000u;
            if (pot_temp >= curr_stage->temp_threshold ||
                (min_boil_time_met && slope >= 0.25f && pot_temp >= 100.0f) ||
                stage_elapsed_ms >= curr_stage->ceiling_duration_ms) {
                advance_stage = true;
            }
            break;
        }
    }

    if (advance_stage) {
        active_stage_idx++;
        if (active_stage_idx < active_profile->stage_count) {
            // the next stage
            timestamp_stage_start = current_time;
            heater_set_target_pot_temp(active_profile->stages[active_stage_idx].target_pot_temp);
        } else {
            // Done in cooking
            on_cooking_complete();
        }
    }
}

// TODO MAKE THIS WORK PROPERLY
void on_cooking_complete(void) {
    buzzer_play_beep();

    bool auto_warm = true;
    if (active_profile != nullptr) {
        auto_warm = active_profile->auto_hold_warm;
    } else if (cooking_mode == MODE_FLAN_DRE) {
        auto_warm = false;
    }

    if (auto_warm) {
        app_state = STATE_HOLD_WARM;
        cooking_mode = MODE_HOLD_WARM;
        heater_set_target_pot_temp(65.0f);
        heater_set_enabled(true);
    } else {
        reset_app_state(STATE_IDLE);
    }
}

///// INIT SHIT /////
void ricecooker_init(void) {
    display_set_enabled(true); // turn the screen on
    reset_app_state(STATE_IDLE); // initial boot
}