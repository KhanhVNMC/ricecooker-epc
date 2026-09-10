#include <stdio.h>
#include "headers/rchal.h"
#include <time.h>

static float sensor_values[COUNT_RC_SENSORS] = {
    [RC_SENSOR_POT] = 25.0f,
    [RC_SENSOR_HEATER] = 25.0f,
};

static bitmask8_t button_state = 0;
static bool heater_state = false;

static bool tm_transaction_open = false;

void rc_init_hal(void) {
    button_state = 0;
    heater_state = false;
    sensor_values[RC_SENSOR_POT] = 25.0f;
    sensor_values[RC_SENSOR_HEATER] = 25.0f;
    tm_transaction_open = false;
    printf("RC HAL (UNIX) INIT'ED\n");
}

void rc_tm1637dp_start(void) {
    tm_transaction_open = true;
    printf("TM1637 START\n");
}

void rc_tm1637dp_write_byte(uint8_t byte) {
    if (!tm_transaction_open) {
        fprintf(stderr,
            "TM1637 ERROR: write_byte(0x%02X) outside transaction\n", byte
        );
        return;
    }
    printf("TM1637 BYTE 0x%02X\n", byte);
}

void rc_tm1637dp_stop(void) {
    if (!tm_transaction_open) {
        fprintf(stderr,"TM1637 ERROR: STOP without START\n");
        return;
    }
    printf("TM1637 STOP\n");
    tm_transaction_open = false;
}

float rc_sensor_get_temp(rc_temp_sensor_t sensor) {
    if (sensor >= COUNT_RC_SENSORS) {
        return 0.0f;
    }
    return sensor_values[sensor];
}

bitmask8_t rc_buttons_get_state(void) {
    return button_state;
}

void rc_heater_set(bool on) {
    heater_state = on;
    printf("HEATER %s\n", on ? "ON" : "OFF");
}

uint32_t rc_time_current_millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(
        (uint64_t)ts.tv_sec * 1000u +
        (uint64_t)ts.tv_nsec / 1000000u
    );
}