#include "include/tm1637dv.h"
#include "include/rchal.h"

#define TM_CMD_DATA_AUTO_INC        0x40
#define TM_CF_CONTROL_MASK          0x80
#define TM_ADDRESS_START            0xC0

#define DISPLAY_MAX_BRIGHTNESS      0b111

/**
 *  _A_
 * │F  │B .DP
 *  -G-
 * │E  │C .DP
 *  ‾D‾
 */
static constexpr bitmask8_t digit_to_dp_matrix[] = {
    /*0*/ RC_SEG_A | RC_SEG_B | RC_SEG_C | RC_SEG_D | RC_SEG_E | RC_SEG_F           ,
    /*1*/            RC_SEG_B | RC_SEG_C ,
    /*2*/ RC_SEG_A | RC_SEG_B |            RC_SEG_D | RC_SEG_E |            RC_SEG_G,
    /*3*/ RC_SEG_A | RC_SEG_B | RC_SEG_C | RC_SEG_D |                       RC_SEG_G,
    /*4*/            RC_SEG_B | RC_SEG_C |                       RC_SEG_F | RC_SEG_G,
    /*5*/ RC_SEG_A |            RC_SEG_C | RC_SEG_D |            RC_SEG_F | RC_SEG_G,
    /*6*/ RC_SEG_A |            RC_SEG_C | RC_SEG_D | RC_SEG_E | RC_SEG_F | RC_SEG_G,
    /*7*/ RC_SEG_A | RC_SEG_B | RC_SEG_C                                            ,
    /*8*/ RC_SEG_A | RC_SEG_B | RC_SEG_C | RC_SEG_D | RC_SEG_E | RC_SEG_F | RC_SEG_G,
    /*9*/ RC_SEG_A | RC_SEG_B | RC_SEG_C | RC_SEG_D |            RC_SEG_F | RC_SEG_G,
    /*P*/ RC_SEG_A | RC_SEG_B                       | RC_SEG_E | RC_SEG_F | RC_SEG_G,
    /*O*/                       RC_SEG_C | RC_SEG_D | RC_SEG_E |            RC_SEG_G,
    /*R*/ RC_SEG_A | RC_SEG_B |                       RC_SEG_E | RC_SEG_F           ,
    /*I*/            RC_SEG_B | RC_SEG_C                                            , // just 1
    /*C*/ RC_SEG_A |                       RC_SEG_D | RC_SEG_E | RC_SEG_F           ,
    /*E*/ RC_SEG_A |                       RC_SEG_D | RC_SEG_E | RC_SEG_F | RC_SEG_G,
    /*S*/ RC_SEG_A |            RC_SEG_C | RC_SEG_D |            RC_SEG_F | RC_SEG_G, // just the 5
    /*U*/            RC_SEG_B | RC_SEG_C | RC_SEG_D | RC_SEG_E | RC_SEG_F           ,
    /*K*/ RC_SEG_A |            RC_SEG_C |            RC_SEG_E | RC_SEG_F | RC_SEG_G,
    /*F*/ RC_SEG_A |                                  RC_SEG_E | RC_SEG_F | RC_SEG_G,
    /*T*/                                  RC_SEG_D | RC_SEG_E | RC_SEG_F | RC_SEG_G,
    /*A*/ RC_SEG_A | RC_SEG_B | RC_SEG_C |            RC_SEG_E | RC_SEG_F | RC_SEG_G,
    /*H*/            RC_SEG_B | RC_SEG_C |            RC_SEG_E | RC_SEG_F | RC_SEG_G,
    /*D*/            RC_SEG_B | RC_SEG_C | RC_SEG_D | RC_SEG_E |            RC_SEG_G,
    /*L*/                                  RC_SEG_D | RC_SEG_E | RC_SEG_F           ,
    /*Y*/            RC_SEG_B | RC_SEG_C | RC_SEG_D |            RC_SEG_F | RC_SEG_G,
    /*N*/ RC_SEG_A | RC_SEG_B | RC_SEG_C |            RC_SEG_E | RC_SEG_F           ,
    /*°*/ RC_SEG_A | RC_SEG_B |                                  RC_SEG_F | RC_SEG_G,
};

void display_set_enabled(const bool on) {
    rc_tm1637dp_start();
    rc_tm1637dp_write_byte(TM_CF_CONTROL_MASK |
        (on ? (1u << 3) : 0u) | (on ? DISPLAY_MAX_BRIGHTNESS : 0u) // max when on
    );
    rc_tm1637dp_stop();
}

void display_set_brightness(const uint8_t brightness) {
    rc_tm1637dp_start();
    rc_tm1637dp_write_byte(TM_CF_CONTROL_MASK |
        (1u << 3) | (brightness & DISPLAY_MAX_BRIGHTNESS) // always on when set
    );
    rc_tm1637dp_stop();
}

// cache
static bool cache_valid = false;
static digit_t last_digits[4] = {CHAR_NULL, CHAR_NULL, CHAR_NULL, CHAR_NULL};
static bool last_colon = false;

void display_set_characters(const digit_t digits[4], const bool colon) {
    if (cache_valid) {
        // prevent bus spamming
        bool changed = colon != last_colon;
        for (uint8_t i = 0; i < 4; i++) {
            if (last_digits[i] != digits[i]) {
                changed = true;
                break;
            }
        }
        if (!changed) return;
    }

    // auto increment mode to not have to manually kt & send
    rc_tm1637dp_start();
    rc_tm1637dp_write_byte(TM_CMD_DATA_AUTO_INC);
    rc_tm1637dp_stop();

    // send the digits
    rc_tm1637dp_start();
    rc_tm1637dp_write_byte(TM_ADDRESS_START);
    for (uint8_t i = 0; i < 4; i++) {
        const digit_t digit = digits[i];
        bitmask8_t mask = digit == CHAR_NULL ? 0x00 : digit_to_dp_matrix[digit];
        if (i == 1 && colon) {
            mask |= RC_SEG_DP; // for the colon
        }
        rc_tm1637dp_write_byte(mask);
    }
    rc_tm1637dp_stop();

    // update cached
    for (uint8_t i = 0; i < 4; i++) {
        last_digits[i] = digits[i];
    }
    last_colon = colon;
    cache_valid = true;
}

void display_clear_cache(void) {
    cache_valid = true;
}

void display_send_raw_segments(const bitmask8_t digit_bitmasks[4]) {
    // auto increment mode to not have to manually kt & send
    rc_tm1637dp_start();
    rc_tm1637dp_write_byte(TM_CMD_DATA_AUTO_INC);
    rc_tm1637dp_stop();

    // send the raw digits
    rc_tm1637dp_start();
    rc_tm1637dp_write_byte(TM_ADDRESS_START);
    for (uint8_t i = 0; i < 4; i++) {
        rc_tm1637dp_write_byte(digit_bitmasks[i]);
    }
    rc_tm1637dp_stop();

    // ensure the next set chars wont be stale
    display_clear_cache();
}

void display_normalize_segments(void) {
    display_set_enabled(false);
    display_send_raw_segments((bitmask8_t[4]) {0, 0, 0, 0});
}