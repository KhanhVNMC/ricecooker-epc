#include "../headers/tm1637dv.h"
#include "../headers/rchal.h"

#define TM_CMD_DATA_AUTO_INC        0x40
#define TM_CF_CONTROL_MASK          0x80
#define TM_ADDRESS_START            0xC0

#define DISPLAY_MAX_BRIGHTNESS      0b111

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
    /*R*/                                             RC_SEG_E |            RC_SEG_G,
    /*I*/                       RC_SEG_C                                            , // im serious
    /*C*/ RC_SEG_A |                       RC_SEG_D | RC_SEG_E | RC_SEG_F           ,
    /*E*/ RC_SEG_A |                       RC_SEG_D | RC_SEG_E | RC_SEG_F | RC_SEG_G,
    /*S*/ RC_SEG_A |            RC_SEG_C | RC_SEG_D |            RC_SEG_F | RC_SEG_G, // just the 5
    /*U*/            RC_SEG_B | RC_SEG_C | RC_SEG_D | RC_SEG_E | RC_SEG_F           ,
    /*K*/ RC_SEG_A |            RC_SEG_C |            RC_SEG_E | RC_SEG_F | RC_SEG_G,
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

void display_set_characters(const digit_t digits[4], const bool colon) {
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
}
