#include <stdlib.h>

#include "headers/rchal.h"
#include "headers/xthreads.h"
#include "headers/platform.h"
#include "headers/mem.h"
#include "headers/tm1637dv.h"

void platform_main(void) {
    rc_init_hal();
    display_set_characters((digit_t[4]){ 0, 1, 2, CHAR_P }, false);
}