#include <stdio.h>
#include <time.h>

#include "fake_io.h"
#include "include/rchal.h"

static float sensor_values[COUNT_RC_SENSORS] = {
    [RC_SENSOR_POT] = 25.0f,
    [RC_SENSOR_HEATER] = 25.0f,
};

static bool heater_state = false;

void rc_init_rchal(void) {
    heater_state = false;
    sensor_values[RC_SENSOR_POT] = 25.0f;
    sensor_values[RC_SENSOR_HEATER] = 25.0f;
    tm_init();
    btns_init();
    printf("RC HAL (UNIX) INIT'ED\n");
    printf("Q=SELECT W=CANCEL E=OPTIONS R=INCR T=DECR\n");
}

void rc_buzzer_play(void) {

}

float rc_sensor_get_temp(const rc_temp_sensor_t sensor) {
    if (sensor >= COUNT_RC_SENSORS) {
        return 0.0f;
    }
    // what the fuck is this man
    // if (heater_state) {
    //     sensor_values[RC_SENSOR_HEATER] += 1;
    //     sensor_values[RC_SENSOR_POT] += 0.9f;
    // } else {
    //     if (sensor_values[RC_SENSOR_HEATER] >= 25) {
    //         sensor_values[RC_SENSOR_HEATER] -= 1.0f;
    //     }
    //     if (sensor_values[RC_SENSOR_POT] >= 25) {
    //         sensor_values[RC_SENSOR_POT] -= 0.5f;
    //     }
    // }
    return sensor_values[sensor];
}

void rc_heater_set(const bool on) {
    heater_state = on;
}

bool rc_heater_is_on(void) {
    return heater_state;
}

uint32_t rc_time_current_millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t) (
        (uint64_t) ts.tv_sec * 1000u +
        (uint64_t) ts.tv_nsec / 1000000u
    );
}