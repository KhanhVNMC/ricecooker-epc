#include "tempstats.h"
#include "include/cmath.h"

static float   tmp_ringbuf[SAMPLES_RETENTION_SIZE];
static uint8_t rb_head;
static uint8_t rb_fill_size;

void tempstats_reset(void) {
    rb_head = 0;
    rb_fill_size = 0;
}

bool tempstats_is_full(void) {
    return rb_fill_size == SAMPLES_RETENTION_SIZE;
}

void tempstats_collect_sample(const float temp) {
    tmp_ringbuf[rb_head] = temp;
    rb_head = (rb_head + 1) % SAMPLES_RETENTION_SIZE;

    if (rb_fill_size < SAMPLES_RETENTION_SIZE) {
        rb_fill_size++;
    }
}

float tempstats_get_at(const uint8_t i) {
    if (rb_fill_size != SAMPLES_RETENTION_SIZE) {
        return tmp_ringbuf[i];
    }
    // this only works on filled buffer
    return tmp_ringbuf[(rb_head + i) % SAMPLES_RETENTION_SIZE];
}

float tempstats_get_avg(void) {
    if (rb_fill_size == 0) {
        return 0.f;
    }
    float sum = 0.f;
    for (uint8_t i = 0u; i < rb_fill_size; ++i) {
        sum += tempstats_get_at(i);
    }
    return sum / (float) rb_fill_size;
}

float tempstats_linreg_slope(void) {
    const uint8_t n = rb_fill_size;
    if (n < 2u) {
        return 0.0f; // doesnt work with like 1 sample (ofc)
    }
    const float avg_y = tempstats_get_avg();
    float avg_t = 0.f;
    for (uint8_t i = 0u; i < n; ++i) {
        avg_t += (float)i;
    }
    avg_t /= (float)n;

    // a = sigma(i=0 -> n, (t[i] - avg(t)) * (y[i] - avg(y)))
    // b = sigma(i=0 -> n, (t[i] - avg(t))^2)
    // m = a / b
    float numerator = 0, denominator = 0;
    for (uint8_t i = 0u; i < n; ++i) {
        numerator += ((float)i - avg_t) * (tempstats_get_at(i) - avg_y);
        denominator += fsquare((float)i - avg_t);
    }
    return numerator / denominator;
}
