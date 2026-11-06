#ifndef TM1637DV_H
#define TM1637DV_H
#include "rchal.h"

enum : bitmask8_t {
    RC_SEG_A  = 1u << 0, // LSB (A)
    RC_SEG_B  = 1u << 1,
    RC_SEG_C  = 1u << 2,
    RC_SEG_D  = 1u << 3,
    RC_SEG_E  = 1u << 4,
    RC_SEG_F  = 1u << 5,
    RC_SEG_G  = 1u << 6,
    RC_SEG_DP = 1u << 7 // MSB (COLON/DOTPOINT)
};

typedef enum : uint8_t {
    CHAR_P = 10,
    CHAR_O = 11,
    CHAR_R = 12,
    CHAR_I = 13,
    CHAR_C = 14,
    CHAR_E = 15,
    CHAR_S = 16,
    CHAR_U = 17,
    CHAR_K = 18,
    CHAR_F = 19,
    CHAR_T = 20,
    CHAR_A = 21,
    CHAR_H = 22,
    CHAR_D = 23,
    CHAR_L = 24,
    CHAR_Y = 25,
    CHAR_N = 26,
    CHAR_G = 27,
    SCHAR_DEG = 28,
    CHAR_NULL = 0xFF,
} digit_t;

void display_set_enabled(bool on);
void display_set_brightness(uint8_t brightness);
void display_set_characters(const digit_t digits[4], bool colon);
void display_clear_cache(void);
void display_send_raw_segments(const bitmask8_t digit_bitmasks[4]);
void display_normalize_segments(void);
void display_scroll_tick(void);
void display_set_scrolling_chars(const digit_t *buf, uint8_t length, bool colon);

#endif //TM1637DV_H
