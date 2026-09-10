#ifndef DISPLAYUTILS_H
#define DISPLAYUTILS_H
#include <stdint.h>

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
    CHAR_NULL = 0xFF,
} digit_t;

void display_characters(const digit_t digits[]);

#endif //DISPLAYUTILS_H
