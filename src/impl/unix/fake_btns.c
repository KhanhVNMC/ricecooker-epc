#include "fake_io.h"
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "include/rchal.h"

static struct termios old_termios;
static bool terminal_initialized = false;

static void terminal_shutdown(void) {
    if (terminal_initialized) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
    }
}

void btns_init(void) {
    if (!isatty(STDIN_FILENO)) {
        return;
    }
    struct termios termios;
    tcgetattr(STDIN_FILENO, &old_termios);
    termios = old_termios;
    termios.c_lflag &= ~(ICANON | ECHO);
    termios.c_cc[VMIN] = 0;
    termios.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &termios);
    terminal_initialized = true;
    atexit(terminal_shutdown);
}

bitmask8_t rc_buttons_get_state(void) {
    char input;
    bitmask8_t button_state = 0;
    if (read(STDIN_FILENO, &input, 1) != 1) {
        return 0;
    }
    switch (input) {
        case 'q': case 'Q': {
            button_state = RC_BTN_SELECT;
            break;
        }
        case 'w': case 'W': {
            button_state = RC_BTN_CANCEL;
            break;
        }
        case 'e': case 'E': {
            button_state = RC_BTN_OPTIONS;
            break;
        }
        case 'r': case 'R': {
            button_state = RC_BTN_INCR;
            break;
        }
        case 't': case 'T': {
            button_state = RC_BTN_DECR;
            break;
        }
        default: break;
    }
    return button_state;
}