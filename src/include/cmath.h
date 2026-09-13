#ifndef RICECOOKER_CMATH_H
#define RICECOOKER_CMATH_H
#include <stdint.h>

static inline uint16_t dcf32u16(const float value, const uint16_t max) {
    if (!(value > 0.0f)) return 0u;
    if (value >= (float)max) return max;
    return (uint16_t) value;
}

static inline uint8_t u8_min(const uint8_t a, const uint8_t b) {
    return a < b ? a : b;
}

static inline uint8_t u8_max(const uint8_t a, const uint8_t b) {
    return a > b ? a : b;
}

static inline uint8_t u8_clamp(const uint8_t x, const uint8_t lo, const uint8_t hi) {
    return u8_min(hi, u8_max(x, lo));
}

static inline float fminf(const float a, const float b) {
    return a < b ? a : b;
}

static inline float fmaxf(const float a, const float b) {
    return a > b ? a : b;
}

static inline float fclampf(const float x, const float lo, const float hi) {
    return fminf(hi, fmaxf(x, lo));
}

#endif //RICECOOKER_CMATH_H