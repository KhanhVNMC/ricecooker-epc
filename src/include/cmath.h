#ifndef RICECOOKER_CMATH_H
#define RICECOOKER_CMATH_H
#include <stdint.h>
#include <string.h>

#define arrsize(array) (sizeof(array) / sizeof((array)[0]))

// https://en.wikipedia.org/wiki/Linear_interpolation
static inline float flerp(const float v0, const float v1, const float t) {
    return (1 - t) * v0 + t * v1;
}

union f32u32u {
    float f; uint32_t i;
}; // diabolical

static inline float fabs(const float x) {
    union f32u32u shit;
    shit.f = x;
    shit.i &= 0x7FFFFFFF;
    return shit.f;
}

static inline float fsquare(const float x) {
    return x * x;
}

static inline uint16_t dcf32u16(const float value, const uint16_t max) {
    if (!(value > 0.0f)) return 0u;
    if (value >= (float)max) return max;
    return (uint16_t) value;
}

static inline uint32_t ricf32u32(const float value) {
    union f32u32u shit;
    shit.f = value;
    return shit.i;
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

static inline float fmin(const float a, const float b) {
    return a < b ? a : b;
}

static inline float fmax(const float a, const float b) {
    return a > b ? a : b;
}

static inline float fclampf(const float x, const float lo, const float hi) {
    return fmin(hi, fmax(x, lo));
}

#endif //RICECOOKER_CMATH_H