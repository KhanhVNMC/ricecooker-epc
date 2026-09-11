#include <stdint.h>
#include <stdio.h>
#include "fake_tm1637.h"
#include "include/tm1637dv.h"

#define TM_RAM_SIZE 6
#pragma once

#define COLOR_ON    "\x1b[0m\x1b[1m\x1b[91m"
#define COLOR_OFF   "\x1b[1m\x1b[90m"
#define COLOR_RESET "\x1b[0m"

// fake TM1637 micro states
static uint8_t tm_ram[TM_RAM_SIZE] = {0};
static bool tm_transaction_open = false;
static uint8_t tm_transaction_byte = 0;
static bool tm_auto_increment = true;
static bool tm_data_transaction = false;
static bool tm_display_enabled = false;
static bool tm_transaction_dirty = false;
static uint8_t tm_address = 0;
static uint8_t tm_brightness = 0;

static const char *get_color(const uint8_t digit, const uint8_t segment) {
    return digit & segment ? COLOR_ON : COLOR_OFF;
}

static void printsegchars(uint8_t digits[4]) {
    printf(" %s⎽⎽⎽   %s⎽⎽⎽     %s⎽⎽⎽   %s⎽⎽⎽\n",
        get_color(digits[0], RC_SEG_A),
        get_color(digits[1], RC_SEG_A),
        get_color(digits[2], RC_SEG_A),
        get_color(digits[3], RC_SEG_A)
    );
    printf("%s│   %s│ %s│   %s│ %s. %s│   %s│ %s│   %s│\n",
        get_color(digits[0], RC_SEG_F),
        get_color(digits[0], RC_SEG_B),
        get_color(digits[1], RC_SEG_F),
        get_color(digits[1], RC_SEG_B),
        get_color(digits[1], RC_SEG_DP),
        get_color(digits[2], RC_SEG_F),
        get_color(digits[2], RC_SEG_B),
        get_color(digits[3], RC_SEG_F),
        get_color(digits[3], RC_SEG_B)
    );
    printf(" %s———   %s———     %s———   %s———\n",
        get_color(digits[0], RC_SEG_G),
        get_color(digits[1], RC_SEG_G),
        get_color(digits[2], RC_SEG_G),
        get_color(digits[3], RC_SEG_G)
    );
    printf("%s│   %s│ %s│   %s│ %s. %s│   %s│ %s│   %s│\n",
        get_color(digits[0], RC_SEG_E),
        get_color(digits[0], RC_SEG_C),
        get_color(digits[1], RC_SEG_E),
        get_color(digits[1], RC_SEG_C),
        get_color(digits[1], RC_SEG_DP),
        get_color(digits[2], RC_SEG_E),
        get_color(digits[2], RC_SEG_C),
        get_color(digits[3], RC_SEG_E),
        get_color(digits[3], RC_SEG_C)
    );
    printf(" %s‾‾‾   %s‾‾‾     %s‾‾‾   %s‾‾‾\n",
        get_color(digits[0], RC_SEG_D),
        get_color(digits[1], RC_SEG_D),
        get_color(digits[2], RC_SEG_D),
        get_color(digits[3], RC_SEG_D)
    );
    printf("%s", COLOR_RESET);
}

static void tm_render(void) {
    uint8_t digits[4] = {0};
    if (tm_display_enabled) {
        for (uint8_t i = 0; i < 4; ++i) {
            digits[i] = tm_ram[i];
        }
    }
    printsegchars(digits);
}

void tm_init() {
    for (uint8_t i = 0; i < TM_RAM_SIZE; ++i) {
        tm_ram[i] = 0;
    }
    tm_transaction_open = false;
    tm_transaction_byte = 0;
    tm_auto_increment = true;
    tm_data_transaction = false;
    tm_address = 0;
    tm_display_enabled = false;
    tm_brightness = 0;
    tm_transaction_dirty = false;
}

void rc_tm1637dp_start(void) {
    if (tm_transaction_open) {
        fprintf(stderr, "TM1637 ERROR: START inside transaction\n");
        return;
    }
    tm_transaction_open = true;
    tm_transaction_byte = 0;
    tm_data_transaction = false;
    tm_transaction_dirty = false;
}

void rc_tm1637dp_write_byte(const uint8_t byte) {
    if (!tm_transaction_open) {
        fprintf(stderr,"TM1637 ERROR: write_byte(0x%02X) outside transaction\n", byte);
        return;
    }
    // first byte determines what kind of transac this is
    if (tm_transaction_byte == 0) {
        // 01xxxxxx = data command
        if ((byte & 0xC0u) == 0x40u) {
            tm_auto_increment = !(byte & 0x04u);
        }
        // 1000xxxx = display control
        else if ((byte & 0xF0u) == 0x80u) {
            tm_display_enabled = byte & (1u << 3);
            tm_brightness = byte & 0b111u;
            tm_transaction_dirty = true;
        }
        // 11xxxxxx = address command
        else if ((byte & 0xC0u) == 0xC0u) {
            tm_address = byte & 0b111u;
            tm_data_transaction = true;
        } else {
            fprintf(stderr,"TM1637 ERROR: unknown command 0x%02X\n", byte);
        }
    }
    // bytes after an address command are display RAM data
    else if (tm_data_transaction > 0) {
        if (tm_address < TM_RAM_SIZE) {
            tm_ram[tm_address] = byte;
            tm_transaction_dirty = true;
        }
        if (tm_auto_increment) {
            ++tm_address;
        }
    } else {
        fprintf(stderr, "TM1637 ERROR: unexpected data byte 0x%02X\n", byte);
    }
    ++tm_transaction_byte;
}

void rc_tm1637dp_stop(void) {
    if (!tm_transaction_open) {
        fprintf(stderr, "TM1637 ERROR: STOP without START\n");
        return;
    }
    tm_transaction_open = false;
    if (tm_transaction_dirty) {
        tm_render();
    }
}