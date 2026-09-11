#include "include/heaterctl.h"
#include "include/cmath.h"
#include "include/xthreads.h"

#define THERMISTOR_MIN_TEMP   (-5.f)
#define THERMISTOR_MAX_TEMP   (110.f)
#define THERMISTOR_HEADROOM   (4.f)
#define HEATERCTL_INTERVAL    (100) // ms

static float heater_temp_target = 0;
static bool heater_element_enabled = false;

void heater_set_target_temp(const float temp) {
    xthreads_csenter();
    heater_temp_target = fclampf(temp, THERMISTOR_MIN_TEMP, THERMISTOR_MAX_TEMP);
    xthreads_csexit();
}

void heater_set_enabled(const bool enabled) {
    xthreads_csenter();
    heater_element_enabled = enabled;
    xthreads_csexit();
}

void task_heater_update(void *args) {
    while (1) {
        xthreads_csenter(); // critical section
        const bool local_heater_enabled = heater_element_enabled;
        const float local_heater_target_temp = heater_temp_target;
        xthreads_csexit();

        if (!local_heater_enabled) {
            rc_heater_set(false);
            xthreads_sleep(HEATERCTL_INTERVAL);
            continue;
        }
        const float heater_tmp = rc_sensor_get_temp(RC_SENSOR_HEATER);
        // headroom to prevent rapid relay clicking
        if (heater_tmp < local_heater_target_temp - THERMISTOR_HEADROOM) {
            rc_heater_set(true);
        } else if (heater_tmp > local_heater_target_temp + THERMISTOR_HEADROOM) {
            rc_heater_set(false);
        }
        xthreads_sleep(HEATERCTL_INTERVAL);
    }
}

float heater_get_target_temp(void) {
    return heater_temp_target;
}

bool heater_is_enabled(void) {
    return heater_element_enabled;
}