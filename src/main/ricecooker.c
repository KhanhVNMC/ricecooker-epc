#include "tempstats.h"
#include "ricecooker.h"
#include "cookingprofile.h"
#include "melodies.h"
#include "include/cmath.h"
#include "include/rchal.h"
#include "include/xthreads.h"
#include "include/tm1637dv.h"
#include "include/heaterctl.h"
#include "include/buzzerctl.h"
#include "include/ecclayer.h"
#include "include/lrcmdctl.h"

#define APP_MAIN_INTERVAL_MS         (20u)
#define APP_BLINK_INTERVAL_MS        (500u)
#define APP_SCROLL_INTERVAL_MS       (500u)

#define APP_ANIM_FRAME_FAST_MS       (200u)
#define APP_ANIM_FRAME_NORMAL_MS     (250u)
#define APP_ANIM_FRAME_SOFT_MS       (300u)

#define APP_TEMP_SHOW_DURATION_MS    (3000u)
#define APP_DISPLAY_ALT_DELAYING_MS  (6000u) // ALT = alternating
#define APP_DISPLAY_ALT_MANUAL_MS    (5000u)

#define CONFIG_TIME_MINUTES_STEP     (1)
#define CONFIG_TEMP_STEP             (5)
#define DEFAULT_MANUAL_MODE_TIME     (30)
#define DEFAULT_MANUAL_MODE_TEMP     (80)
#define DEFAULT_DELAYED_MODE_TIME    (15)
#define MIN_CONFIG_TIME_MINS         (1)
#define MAX_CONFIG_TIME_MINS         (99u * 60 + 59u)
#define MIN_CONFIG_TEMP              (60)
#define MAX_CONFIG_TEMP              (105)

// digit consts
static constexpr digit_t BLANK[4] = {CHAR_NULL, CHAR_NULL, CHAR_NULL, CHAR_NULL};
static constexpr digit_t IDLE[4] = {CHAR_I, CHAR_D, CHAR_L, CHAR_E};
// extended (via scroll)
static constexpr digit_t EXT_HOLDING[] = {CHAR_H, CHAR_O, CHAR_L, CHAR_D, CHAR_I, CHAR_N, CHAR_G, CHAR_NULL};
static constexpr digit_t EXT_DELAYING[] = {CHAR_NULL, CHAR_NULL, CHAR_NULL, CHAR_D, CHAR_E, CHAR_L, CHAR_A, CHAR_Y, CHAR_I, CHAR_N, CHAR_G, CHAR_NULL};

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
// application layer clock (updated per tick)
static uint32_t app_tick_start_timestamp;

// state machines shit
static app_state_t    app_state;
static cooking_mode_t cooking_mode;

// manual mode configuration
static uint8_t  manual_mode_temp;
static uint16_t manual_mode_time_mins;
static bool     manual_mode_auto_warm;

// delayed mode configuration
static uint16_t delayed_mode_time_mins;
static bool     delayed_mode_configured; // true if ready to select mode

// timestamps
static uint32_t timestamp_cook_start;
static uint32_t timestamp_delayed_start;
static uint32_t timestamp_last_pot_temp_sample; // temp sample
static uint32_t timestamp_last_display_sc_tick; // for display scrolling
static uint32_t timestamp_pot_temp_display_start; // cosmetic

// for time cfg states, like manual or delay time
static bool config_time_edit_minutes;

// error detection policy & shit
static uint8_t system_error_code;

// physical button tracking & shit
static bitmask8_t last_input_mask = 0;
static bool phys_input_disabled = false;

void start_cooking_program(void); // honor configured delay
void start_task_cooking_now(void); // enter STATE_COOKING immediately
void reset_cooking_context(void);
void on_cooking_pltick(uint32_t current_time);
void on_cooking_complete(void);
void on_delayed_countdown_end(void);
void update_rc_display(void);
void on_platform_tick(void);
bool on_cmd_datagram_recv(ib_cmd_type_t cmd_type, uint16_t payload_size, const uint8_t *payload);

// util functions
static void set_app_state(const app_state_t new_state) { app_state = new_state; }
static void set_cooking_mode(const cooking_mode_t new_mode) { cooking_mode = new_mode; }
static btn_feedback_t config_adjust_time(uint16_t *time_mins_dest, bitmask8_t pressed);
static void app_display_countdown(bool blink, uint32_t time_left_secs);
static void app_display_temperature(uint16_t temp);
// 禁弾「過去を刻む時計」: time math is intentionally wraparound safe
static uint32_t app_current_time(void) { return app_tick_start_timestamp; }

void reset_app_state(const app_state_t reset_to) {
    // heater & buzzer
    heater_set_enabled(false);
    buzzer_stop_melody();

    // FSM reset
    set_app_state(reset_to);
    set_cooking_mode(MODE_RICE_NORMAL);
    if (reset_to != STATE_SYSTEM_ERROR) {
        system_error_code = 0; // no "errors"
        heater_clear_faults();
    } else {
        // for error state, keep playing this annoying fuck sound; looped
        melodies_play_one(MELODY_SYSTEM_ERROR, true);
    }

    // default values for delay and manual + more
    manual_mode_time_mins = DEFAULT_MANUAL_MODE_TIME;
    manual_mode_temp = DEFAULT_MANUAL_MODE_TEMP;
    manual_mode_auto_warm = true;
    delayed_mode_time_mins = DEFAULT_DELAYED_MODE_TIME;
    delayed_mode_configured = false;
    config_time_edit_minutes = true;
    tempstats_reset();
    reset_cooking_context();

    // just in case
    timestamp_cook_start = 0;
    timestamp_delayed_start = 0;
    timestamp_pot_temp_display_start = 0;
    timestamp_last_pot_temp_sample = 0;
    timestamp_last_display_sc_tick = 0;
}

// RULEs FOR BUTTON FEEDBACK:
// If the button led to a state/value change => BTN_ACCEPTED_*
// if the button intends to do state/value change but hits limits/rejection => BTN_REJECTED
// otherwise: BTN_IGNORED (nothing changed, you (and your actions) are worthless << self reflection btw)
btn_feedback_t app_on_btns_press(const bitmask8_t pressed) {
    if (pressed & RC_BTN_CANCEL) {
        const app_state_t last_state = app_state;
        reset_app_state(STATE_IDLE); // ignore errors for now
        return last_state == app_state ? BTN_IGNORED : BTN_ACCEPTED;
    }
    switch (app_state) {
        case STATE_IDLE: {
            // pressing OPTIONS here take to mode select menu
            if (pressed & RC_BTN_OPTIONS) {
                set_app_state(STATE_MODE_SELECT);
                set_cooking_mode(MODE_RICE_NORMAL);
                return BTN_ACCEPTED;
            }
            return BTN_IGNORED;
        }
        case STATE_CF_DELAYED_MODE_SELECT:
        case STATE_MODE_SELECT: {
            // press SELECT here either:
            if (pressed & RC_BTN_SELECT) {
                // if selected "DELAY", take to select delayed time
                if (cooking_mode == MODE_DELAYED_EXEC) {
                    set_app_state(STATE_CF_DELAYED_TIME);
                    set_cooking_mode(MODE_RICE_NORMAL); // ensure valid mode
                    config_time_edit_minutes = true;
                    return BTN_ACCEPTED;
                }
                // if selected "MANUAL", take to set temp
                if (cooking_mode == MODE_MANUAL_USER) {
                    set_app_state(STATE_CF_MANUAL_TEMP);
                    return BTN_ACCEPTED;
                }
                start_cooking_program(); // delayed/now is handled inside
                return BTN_ACCEPTED_SELF;
            }
            // in delayed mode, you cannot select delayed again
            const uint8_t mode_count = delayed_mode_configured ? MODE_DELAYED_EXEC : COUNT_COOKING_MODES;
            // wraparounds for less clunky navigation
            if (pressed & RC_BTN_INCR) {
                set_cooking_mode((cooking_mode + 1) % mode_count);
                return BTN_ACCEPTED;
            }
            if (pressed & RC_BTN_DECR) {
                set_cooking_mode(((cooking_mode - 1) + mode_count) % mode_count);
                return BTN_ACCEPTED;
            }
            return BTN_IGNORED;
        }
        case STATE_CF_MANUAL_TEMP: {
            if (pressed & RC_BTN_SELECT) {
                // after selecting temp, select time 4 manual
                set_app_state(STATE_CF_MANUAL_TIME);
                config_time_edit_minutes = true;
                return BTN_ACCEPTED;
            }
            if (pressed & RC_BTN_INCR) {
                if (manual_mode_temp + CONFIG_TEMP_STEP <= MAX_CONFIG_TEMP) {
                    manual_mode_temp += CONFIG_TEMP_STEP;
                    return BTN_ACCEPTED;
                }
                return BTN_REJECTED;
            }
            if (pressed & RC_BTN_DECR) {
                if (manual_mode_temp >= CONFIG_TEMP_STEP + MIN_CONFIG_TEMP) {
                    manual_mode_temp -= CONFIG_TEMP_STEP;
                    return BTN_ACCEPTED;
                }
                return BTN_REJECTED;
            }
            return BTN_IGNORED;
        }
        case STATE_CF_MANUAL_TIME: {
            if (pressed & RC_BTN_SELECT) {
                // after selecting time, choose to hold warm after or not
                set_app_state(STATE_CF_MANUAL_AUTO_WARM);
                return BTN_ACCEPTED;
            }
            if (pressed & RC_BTN_OPTIONS) {
                // swap between editing min/hour
                config_time_edit_minutes = !config_time_edit_minutes;
                return BTN_ACCEPTED;
            }
            return config_adjust_time(&manual_mode_time_mins, pressed);
        }
        case STATE_CF_MANUAL_AUTO_WARM: {
            if (pressed & RC_BTN_SELECT) {
                start_cooking_program(); // cook time
                return BTN_ACCEPTED_SELF;
            }
            if (pressed & RC_BTN_OPTIONS) {
                // swap between on/off auto warm
                manual_mode_auto_warm = !manual_mode_auto_warm;
                return BTN_ACCEPTED;
            }
            return BTN_IGNORED;
        }
        case STATE_CF_DELAYED_TIME: {
            if (pressed & RC_BTN_SELECT) {
                // delayed mode configured complete
                delayed_mode_configured = true;
                set_cooking_mode(MODE_RICE_NORMAL); // default mode
                set_app_state(STATE_CF_DELAYED_MODE_SELECT);
                return BTN_ACCEPTED;
            }
            if (pressed & RC_BTN_OPTIONS) {
                // swap between editing min/hour
                config_time_edit_minutes = !config_time_edit_minutes;
                return BTN_ACCEPTED;
            }
            return config_adjust_time(&delayed_mode_time_mins, pressed);
        }
        case STATE_COOKING: {
            if (pressed & RC_BTN_OPTIONS) {
                // this should display the temp screen for Xs (default 3)
                timestamp_pot_temp_display_start = app_current_time();
                return BTN_ACCEPTED;
            }
            return BTN_IGNORED;
        }
        default: break; // prevent complaining
    }
    return BTN_IGNORED;
}

void update_rc_display(void) {
    const uint32_t current_time = app_current_time();
    const bool blink = (current_time / APP_BLINK_INTERVAL_MS) & 1u; // flip flop every interval

    switch (app_state) {
        case STATE_IDLE: {
            display_set_characters(IDLE, false);
            break;
        }
        case STATE_SYSTEM_ERROR: {
            // display blinking [ER:<code>]
            display_set_characters(blink ?
                (digit_t[4]) {CHAR_E, CHAR_R, system_error_code / 10, system_error_code % 10}
                : BLANK, blink
            );
            break;
        }
        case STATE_CF_DELAYED_MODE_SELECT:
        case STATE_MODE_SELECT: {
            display_set_characters(
                blink ? mode_to_digits[cooking_mode] : BLANK,
                (app_state != STATE_MODE_SELECT) // ":" if delayed
            );
            break;
        }
        case STATE_COOKING: {
            // SHOW POT TEMPERATURE: if the user pressed OPTIONS during cooking
            if (current_time - timestamp_pot_temp_display_start < APP_TEMP_SHOW_DURATION_MS) {
                app_display_temperature(
                    dcf32u16(heater_read_cached_sensor(RC_SENSOR_POT) + 0.5f, 250)
                );
                break;
            }

            // SHOW TIME LEFT: for manual mode every X sec (from cooking start)
            if (cooking_mode == MODE_MANUAL_USER && ((current_time - timestamp_cook_start) / APP_DISPLAY_ALT_MANUAL_MS) & 1u) {
                const uint32_t duration_ms = manual_mode_time_mins * 60000u;
                app_display_countdown(blink,
                    ((timestamp_cook_start + duration_ms) - current_time) / 1000u
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
            // SHOW TIME LEFT: for delayed mode every X sec (from delay start)
            if (((current_time - timestamp_delayed_start) / APP_DISPLAY_ALT_DELAYING_MS) & 1u) {
                const uint32_t duration_ms = delayed_mode_time_mins * 60000u;
                app_display_countdown(blink,
                    ((timestamp_delayed_start + duration_ms) - current_time) / 1000u
                );
                break;
            }
            // SHOW "DELAYING" (scrollin') in between
            display_set_scrolling_chars(EXT_DELAYING, sizeof(EXT_DELAYING), false);
            break;
        }
        case STATE_CF_MANUAL_TEMP: {
            app_display_temperature(manual_mode_temp);
            break;
        }
        case STATE_CF_MANUAL_TIME:
        case STATE_CF_DELAYED_TIME: {
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
        case STATE_CF_MANUAL_AUTO_WARM: {
            // display [HL:<ON/OF>] (only <ON/OF> blinks)
            display_set_characters((digit_t[4]) {CHAR_H, CHAR_L,
                !blink ? CHAR_NULL : 0, // the 0 looks prettier
                !blink ? CHAR_NULL : (manual_mode_auto_warm ? CHAR_N : CHAR_F)
            }, true);
            break;
        }
        case STATE_HOLD_WARM: {
            display_set_scrolling_chars(EXT_HOLDING, sizeof(EXT_HOLDING), false);
            break;
        }
        default: break; // prevent complaining
    }
}

static btn_feedback_t config_adjust_time(uint16_t *time_mins_dest, const bitmask8_t pressed) {
    const uint16_t to_change = (config_time_edit_minutes ? CONFIG_TIME_MINUTES_STEP : 60);
    if (pressed & RC_BTN_INCR) {
        if (*time_mins_dest + to_change <= MAX_CONFIG_TIME_MINS) {
            *time_mins_dest += to_change;
            return BTN_ACCEPTED;
        }
        return BTN_REJECTED;
    }
    if (pressed & RC_BTN_DECR) {
        if (*time_mins_dest >= to_change + MIN_CONFIG_TIME_MINS) {
            *time_mins_dest -= to_change;
            return BTN_ACCEPTED;
        }
        return BTN_REJECTED;
    }
    return BTN_IGNORED;
}

static inline void app_display_countdown(const bool blink, const uint32_t time_left_secs) {
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
        app_tick_start_timestamp = rc_time_current_millis();
        on_platform_tick();
        xthreads_sleep(APP_MAIN_INTERVAL_MS);
    }
}

void start_cooking_program(void) {
    // IF "DELAY" configured before -> start countdown
    if (delayed_mode_configured) {
        timestamp_delayed_start = app_current_time();
        melodies_play_one(MELODY_DELAYED_START, false); // delayed start jingle
        set_app_state(STATE_DELAYED_COUNTDOWN);
        return;
    }
    melodies_play_one(MELODY_COOK_START, false); // immediate start jingle
    // ELSE exec normal cooking proc
    start_task_cooking_now();
}

void on_delayed_countdown_end(void) {
    start_task_cooking_now();
}

static void faults_report_to_ecc(const uint8_t metadata) {
    // report to ECC layer for diagnostic
    if (ecc_is_layer_ready()) {
        const uint8_t report[] = { system_error_code, metadata }; // [syserr code][extra metadata]
        lrcmdctl_ob_send(RAOB_FAULTS_REPORT, report, sizeof(report));
    }
}

static void cooking_watchdog_tick(const uint32_t current_time) {
    // watchdog for heater faults (error code 33 -> 63)
    const heater_faults_t hfaults = heater_get_faults();
    if (hfaults != 0) {
        system_error_code = 0b100000; // base (32) heater error code (or with the mask)
        // element overheat: code 33 (bi = by itself) pot overheat: code 34 (bi);
        // heater underheat: code 36 (bi); pot underheat: code 40 (bi)
        // poor contact: code 48 (bi)
        system_error_code |= hfaults;
        reset_app_state(STATE_SYSTEM_ERROR);
        faults_report_to_ecc(hfaults);
        return;
    }
    // collect pot temp every second
    if (current_time - timestamp_last_pot_temp_sample >= 1000u) {
        const float pot_temp = heater_read_cached_sensor(RC_SENSOR_POT);
        const float element_temp = heater_read_cached_sensor(RC_SENSOR_HEATER);

        // for temp statistics
        tempstats_collect_sample(pot_temp);
        timestamp_last_pot_temp_sample = current_time;

        // report to ecc layer
        if (ecc_is_layer_ready()) {
            const uint32_t raw_pottmp = ricf32u32(pot_temp); // float to raw bits
            const uint32_t raw_elementtmp = ricf32u32(element_temp);
            const uint8_t report[] = {
                UNPACK_U32_BE(current_time), // time for graph
                UNPACK_U32_BE(raw_pottmp), UNPACK_U32_BE(raw_elementtmp), // sensors
                heater_is_enabled(), // heater control plane enabled
                rc_heater_is_on() // physical heating element ON
            };
            // best effort send
            lrcmdctl_ob_send_async(RAOB_HEATER_REPORT, report, sizeof(report));
        }
    }
}

void on_platform_tick(void) {
    const uint32_t current_time = app_current_time();
    // trigger the job at the end of the delay tunnel
    if (app_state == STATE_DELAYED_COUNTDOWN) {
        const uint32_t dl_duration_ms = delayed_mode_time_mins * 60000u;
        if (current_time - timestamp_delayed_start >= dl_duration_ms) {
            delayed_mode_configured = false;
            on_delayed_countdown_end();
        }
    }
    // platform tick while cooking
    if (app_state == STATE_COOKING || app_state == STATE_HOLD_WARM) {
        cooking_watchdog_tick(current_time);
        // watchdog could perform a reset to ERROR, so guard against it
        if (app_state == STATE_COOKING || app_state == STATE_HOLD_WARM) {
            on_cooking_pltick(current_time);
        }
    }
    // process control datagrams
    if (lrcmdctl_ib_has_pending()) {
        on_cmd_datagram_recv(
            lrcmdctl_ib_get_cmd_type(),
            lrcmdctl_ib_payload_size(),
            lrcmdctl_ib_get_payload()
        );
        lrcmdctl_ib_release();
    }
    // physical input handling
    if (!phys_input_disabled) {
        const bitmask8_t current_inp_mask = rc_buttons_get_state();
        if (last_input_mask != current_inp_mask) {
            // current && !last
            // only fire event on btn press (not release)
            const bitmask8_t changed = current_inp_mask & ~last_input_mask;
            if (changed) {
                const btn_feedback_t feedback = app_on_btns_press(changed);
                switch (feedback) {
                    case BTN_ACCEPTED: melodies_play_one(MELODY_BTN_ACCEPTED, false); break;
                    case BTN_REJECTED: melodies_play_one(MELODY_BTN_REJECTED, false); break;
                    case BTN_ACCEPTED_SELF: case BTN_IGNORED: break; // dont play any sound
                }
            }
            last_input_mask = current_inp_mask;
        }
    }
    // update output
    update_rc_display();
    // scroll tick states are mutable from update_rc_display, so call this after
    if (current_time - timestamp_last_display_sc_tick >= APP_SCROLL_INTERVAL_MS) {
        display_scroll_tick();
        timestamp_last_display_sc_tick = current_time;
    }
}

//// APPLICATION COOKING/COOKING PROFILE LOGIC /////
// thermal cooking profile tracking
static const cooking_profile_t *active_profile = nullptr;
static uint8_t active_stage_idx = 0;
static uint32_t timestamp_stage_start = 0;

// keep track of the current idex of SETPOINT WITHIN a stage
static uint8_t stage_setpoint_index = 0;

// this prevent the first initial heatup to be treated as boil dry
static bool boil_plateau_seen = false;

void reset_cooking_context(void) {
    active_profile = nullptr;
    active_stage_idx = 0;
    timestamp_stage_start = 0;
    stage_setpoint_index = 0;
    boil_plateau_seen = false;
}

static bool has_water_boiled_away(void) {
    if (!tempstats_is_full()) {
        return false;
    }
    // statistics from the pot, not the heater
    const float avg_temp = tempstats_get_avg();
    const float linreg_slope = tempstats_linreg_slope();

    if (!boil_plateau_seen) {
        // boiling plateau has a specific characteristic (needs experiment)
        if (avg_temp >= WATER_BOIL_PLATEAU_MIN_TEMP
          && f32abs(linreg_slope) <= WATER_BOIL_PLATEAU_MAX_ABS_SLOPE
        ) {
            boil_plateau_seen = true;
        }
        return false;
    }

    // with the plateau observed, lr slope with this characteristic
    // COULD mean water boiled away, or sth exploded
    return linreg_slope >= WATER_BOIL_DRY_MIN_SLOPE;
}

static float monotonic_get_target_temp(const uint32_t stage_elapsed_mins) {
    const cooking_stage_t *stage = &active_profile->stages[active_stage_idx];
    const temp_setpoint_t *graph = stage->temp_setpoints;

    // move current to the next setpoint index when its time
    if (stage_setpoint_index + 1 < stage->temp_setpoint_count &&
        stage_elapsed_mins >= graph[stage_setpoint_index + 1].start_time_min
    ) {
        stage_setpoint_index++;
    }

    const temp_setpoint_t current = graph[stage_setpoint_index];
    if (stage_setpoint_index + 1 >= stage->temp_setpoint_count) {
        // no more next setpoint, keep returning current
        return current.start_temp_val;
    }

    const temp_setpoint_t next = graph[stage_setpoint_index + 1];
    const float delta = ((float)stage_elapsed_mins - (float)current.start_time_min) /
        ((float)next.start_time_min - (float)current.start_time_min)
    ;
    return flerp(current.start_temp_val, next.start_temp_val, delta);
}

void start_task_cooking_now(void) {
    // holding warm is technically NOT a cooking mode, it is a system state!
    if (cooking_mode == MODE_HOLD_WARM) {
        set_app_state(STATE_HOLD_WARM);
        heater_set_target_pot_temp(COOKING_KEEP_WARM_TEMP);
        heater_set_enabled(true);
        return;
    }

    timestamp_cook_start = app_current_time();
    set_app_state(STATE_COOKING);

    // manual mode have no profile and static temp
    if (cooking_mode == MODE_MANUAL_USER) {
        active_profile = nullptr;
        heater_set_target_pot_temp(manual_mode_temp);
        heater_set_enabled(true);
        return;
    }

    // load cooking profile from preset
    reset_cooking_context();
    active_profile = cooking_get_profile(cooking_mode);

    if (active_profile == nullptr || active_profile->stage_count == 0u || active_profile->stages == nullptr) {
        system_error_code = 80; // code 80: profile error
        faults_report_to_ecc(cooking_mode); // hm
        reset_app_state(STATE_SYSTEM_ERROR);
        return;
    }

    // setup the initial stage
    timestamp_stage_start = app_current_time();
    heater_set_target_pot_temp(monotonic_get_target_temp(0u));
    heater_set_enabled(true);
}

void on_cooking_pltick(const uint32_t current_time) {
    // hold warm dont have a ticking job
    if (app_state == STATE_HOLD_WARM) {
        return;
    }

    // manual mode timeout (static time)
    if (cooking_mode == MODE_MANUAL_USER) {
        const uint32_t man_duration_ms = (uint32_t)manual_mode_time_mins * 60000u;
        if (current_time - timestamp_cook_start >= man_duration_ms) {
            on_cooking_complete();
        }
        return;
    }

    const cooking_stage_t *current_stage = &active_profile->stages[active_stage_idx];
    const uint32_t stage_elapsed_ms = current_time - timestamp_stage_start;
    const float actual_pot_temp = heater_read_cached_sensor(RC_SENSOR_POT);

    // this only meaningfully change every min, but, better safe than sorry
    heater_set_target_pot_temp(monotonic_get_target_temp(
        stage_elapsed_ms / 60000u
    ));

    bool advance_stage = false;
    switch (current_stage->trigger_type) {
        case TRIGGER_TIME_EXPIRED: {
            if (stage_elapsed_ms >= current_stage->ceiling_duration_ms) {
                advance_stage = true;
            }
            break;
        }
        case TRIGGER_POT_TEMP_REACHED: { // or timeout
            if (actual_pot_temp >= current_stage->temp_reached_threshold ||
                stage_elapsed_ms >= current_stage->ceiling_duration_ms
            ) {
                advance_stage = true;
            }
            break;
        }
        case TRIGGER_WATER_BOIL_DRY: { // or timeout
            if (stage_elapsed_ms >= current_stage->ceiling_duration_ms ||
                has_water_boiled_away()
            ) {
                advance_stage = true;
            }
            break;
        }
    }

    if (advance_stage) {
        // if next stage overflows, done with the cooking prog
        if (++active_stage_idx >= active_profile->stage_count) {
            on_cooking_complete();
            return;
        }
        // setup the next stage
        timestamp_stage_start = current_time;
        stage_setpoint_index = 0;
        boil_plateau_seen = false;
    }
}

void on_cooking_complete(void) {
    const cooking_mode_t last_cook_mode = cooking_mode;
    bool auto_hold_warm = manual_mode_auto_warm;
    if (active_profile != nullptr) {
        auto_hold_warm = active_profile->auto_hold_warm;
    }

    if (auto_hold_warm) {
        set_cooking_mode(MODE_HOLD_WARM);
        start_task_cooking_now();
    } else {
        reset_app_state(STATE_IDLE);
    }

    // play the completion jingle
    switch (last_cook_mode) {
        case MODE_MANUAL_USER: melodies_play_one(MELODY_COMPLETION_ELISE, false); break;
        case MODE_FLAN_DRE: melodies_play_one(MELODY_COMPLETION_OWEN, false); break;
        default: melodies_play_one(MELODY_COMPLETION_DEFAULT, false); break;
    }
}

//// APPLICATION EXTERNAL CONTROL LOGIC /////
static inline bool reset_and_configure_manual_mode(
    const uint8_t set_temp, const uint16_t set_time_mins, const bool set_auto_warm
) {
    if (set_temp < MIN_CONFIG_TEMP || set_temp > MAX_CONFIG_TEMP) {
        // keep temp in range
        return false;
    }
    if (set_time_mins < MIN_CONFIG_TIME_MINS || set_time_mins > MAX_CONFIG_TIME_MINS) {
        // keep time in range
        return false;
    }
    reset_app_state(STATE_IDLE); // reset to a safe state first
    manual_mode_temp = set_temp;
    manual_mode_time_mins = set_time_mins;
    manual_mode_auto_warm = set_auto_warm;
    return true;
}

bool on_cmd_datagram_recv(const ib_cmd_type_t cmd_type, const uint16_t payload_size, const uint8_t *payload) {
    switch (cmd_type) {
        case CIB_DEBUG_ECHO: {
            return lrcmdctl_ob_send(RSOB_DEBUG_ECHO, payload, payload_size);
        }
        case CIB_DEBUG_SET_DLED: {
            if (payload_size != 1) goto resp_nak;
            rc_dbg_led_set(payload[0]);
            goto resp_ack;
        }
        case CIB_CTL_RESET: {
            if (payload_size != 0) goto resp_nak;
            reset_app_state(STATE_IDLE);
            goto resp_ack;
        }
        case CIB_CTL_SYSCONFIG: {
            // [configuration bitfield: 2]
            if (payload_size != 1) goto resp_nak;
            phys_input_disabled = (payload[0] & 0b1) != 0; // LSB = 1 == disable
            buzzer_mute_device((payload[0] & 0b10) != 0); // 2nd bit set = muted
            goto resp_ack;
        }
        case CIB_CTL_PRESS_BTNS: {
            // [button mask: 1]
            if (payload_size != 1) goto resp_nak;
            const bitmask8_t pressed_button = payload[0];
            app_on_btns_press(pressed_button);
            goto resp_ack;
        }
        case CIB_CTL_COOK_PRESET: {
            // [cooking preset: 1]
            if (payload_size != 1) goto resp_nak;
            const cooking_mode_t cook_preset = payload[0];

            if (cook_preset >= MODE_MANUAL_USER) {
                // do NOT accept MANUAL and DELAYED, they are not presets
                goto resp_nak;
            }
            reset_app_state(STATE_IDLE); // reset to a safe state first
            set_cooking_mode(cook_preset);
            start_cooking_program();
            goto resp_ack;
        }
        case CIB_CTL_COOK_MANUAL: {
            // [manual_temp: 1][manual_time: 2][auto_warm: 1]
            if (payload_size != 4) goto resp_nak;
            const uint8_t set_temp = payload[0];
            const uint16_t set_time_mins = (payload[1] << 8) | payload[2];
            const bool set_auto_warm = payload[3] & 1;

            if (reset_and_configure_manual_mode(set_temp, set_time_mins, set_auto_warm)) {
                set_cooking_mode(MODE_MANUAL_USER);
                start_cooking_program();
                goto resp_ack;
            }
            goto resp_nak;
        }
        case CIB_CTL_COOK_DELAYED: {
            // [cooking mode: 1][delayed_time: 2]|<for MANUAL>[manual_temp: 1][manual_time: 2][auto_warm: 1]
            if (payload_size < 3) goto resp_nak;
            const cooking_mode_t cook_mode = payload[0];
            const uint16_t delay_time_mins = (payload[1] << 8) | payload[2];
            if (delay_time_mins < MIN_CONFIG_TIME_MINS || delay_time_mins > MAX_CONFIG_TIME_MINS) {
                // keep time in range
                goto resp_nak;
            }

            if (cook_mode >= MODE_DELAYED_EXEC) {
                goto resp_nak; // DELAYED is not a program per-se
            }
            // delayed manual requires temp, time and auto warm config
            if (cook_mode == MODE_MANUAL_USER) {
                if (payload_size != 4 /*see CIB_CTL_COOK_MANUAL*/ + 3) goto resp_nak;
                const uint8_t set_temp = payload[3];
                const uint16_t set_time_mins = (payload[4] << 8) | payload[5];
                const bool set_auto_warm = payload[6] & 1;

                if (reset_and_configure_manual_mode(set_temp, set_time_mins, set_auto_warm)) {
                    delayed_mode_time_mins = delay_time_mins;
                    delayed_mode_configured = true;
                    set_cooking_mode(MODE_MANUAL_USER);
                    start_cooking_program();
                    goto resp_ack;
                }
                goto resp_nak;
            }
            // normal delayed with preset
            if (payload_size != 3) goto resp_nak; // for preset, only accept [time][preset]
            reset_app_state(STATE_IDLE); // reset to a safe state first
            delayed_mode_time_mins = delay_time_mins;
            set_cooking_mode(cook_mode);
            delayed_mode_configured = true;
            start_cooking_program();
            goto resp_ack;
        }
        case CIB_GET_APP_STATE_REPORT: {
            if (payload_size != 0) goto resp_nak;
            const uint32_t ts_avg_bits = ricf32u32(tempstats_get_avg());
            const uint32_t ts_linreg_bits = ricf32u32(tempstats_linreg_slope());
            const uint8_t report[] = {
                heater_get_faults(), system_error_code, // should be 0, 0 if healthy
                app_state, cooking_mode, // state & cooking mode
                phys_input_disabled, last_input_mask, // physical input info
                UNPACK_U32_BE(app_current_time()), // app clock
                UNPACK_U32_BE(timestamp_cook_start), // nonsensical if state != cooking
                UNPACK_U32_BE(timestamp_delayed_start), // nonsensical if state != delayed
                // manual configuration (could be stale)
                manual_mode_temp, UNPACK_U16_BE(manual_mode_time_mins), manual_mode_auto_warm,
                // delayed config (could be stale too)
                delayed_mode_configured, UNPACK_U16_BE(delayed_mode_time_mins),
                // detailed cooking data (could be nonsensical)
                active_stage_idx, stage_setpoint_index, UNPACK_U32_BE(timestamp_stage_start),
                boil_plateau_seen, tempstats_sample_size(), // should be enough
                UNPACK_U32_BE(ts_avg_bits), UNPACK_U32_BE(ts_linreg_bits)
            };
            return lrcmdctl_ob_send(RSOB_APP_STATE_REPORT, report, sizeof(report));
        }
        // unknown command/corrupted commands
        default: goto resp_nak;
    }
    resp_nak: return lrcmdctl_ob_ack(false);
    resp_ack: return lrcmdctl_ob_ack(true);
}

///// INIT SHIT /////
void ricecooker_init(void) {
    display_set_enabled(true); // turn the screen on
    reset_app_state(STATE_IDLE); // initial boot
}