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
static bitmask8_t last_digit_masks[4];

static bool should_send_update(const bitmask8_t digit_bitmasks[4]) {
    if (!cache_valid) return true;
    for (uint8_t i = 0; i < 4; i++) {
        if (last_digit_masks[i] != digit_bitmasks[i]) {
            return true;
        }
    }
    return false;
}

static void digits_to_bitmasks(bitmask8_t *out, const digit_t digits[4], const bool colon) {
    for (uint8_t i = 0; i < 4; i++) {
        const digit_t digit = digits[i];
        bitmask8_t mask = digit == CHAR_NULL ? 0x00 : digit_to_dp_matrix[digit];
        if (colon) {
            mask |= RC_SEG_DP; // for the colon
        }
        out[i] = mask;
    }
}

void display_set_characters(const digit_t digits[4], const bool colon) {
    bitmask8_t out_masks[4] = {0};
    digits_to_bitmasks(out_masks, digits, colon);
    display_send_raw_segments(out_masks);
}

void display_clear_cache(void) {
    cache_valid = false;
}

void display_send_raw_segments(const bitmask8_t digit_bitmasks[4]) {
    // prevent bus spamming
    if (!should_send_update(digit_bitmasks)) {
        return;
    }

    // auto increment mode to not have to manually kt & send
    rc_tm1637dp_start();
    rc_tm1637dp_write_byte(TM_CMD_DATA_AUTO_INC);
    rc_tm1637dp_stop();

    // send the raw digits
    rc_tm1637dp_start();
    rc_tm1637dp_write_byte(TM_ADDRESS_START);
    for (uint8_t i = 0; i < 4; i++) {
        const bitmask8_t mask = digit_bitmasks[i];
        rc_tm1637dp_write_byte(mask);
        last_digit_masks[i] = mask;
    }
    rc_tm1637dp_stop();

    // cache is now valid
    cache_valid = true;
}

void display_normalize_segments(void) {
    display_clear_cache();
    display_set_enabled(false);
    display_send_raw_segments((bitmask8_t[4]) {0, 0, 0, 0});
}