#include "displaydriver.h"
#include "../rcplatform/rchal.h"
#include "../xthreads/xthreads.h"

static digit_t frontbuf_digits[4];
static digit_t backbuf_digits[4];

static digit_t* volatile current_display = frontbuf_digits;

void display_characters(const digit_t digits[4]) {
    digit_t *buf = (current_display == frontbuf_digits) ? backbuf_digits : frontbuf_digits;
    for (int i = 0; i < 4; ++i) {
        buf[i] = digits[i];
    }
    current_display = buf;
}

void t_display_update_thread(void*) {
    while (1) {
        const digit_t *display = current_display;
        for (int i = 0; i < 4; ++i) {
            const digit_t d = display[i];
            rc_display_digit_off_all();
            if (d != CHAR_NULL) {
                rc_display_set_segments((bitmask8_t) 0x0); // TODO, use d
                rc_display_digit_on(i);
            }
            xthreads_sleep(2);
        }
    }
}