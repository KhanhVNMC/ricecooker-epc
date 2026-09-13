#include "include/heaterctl.h"
#include "include/cmath.h"
#include "include/xthreads.h"

#define HEATER_SAFETY_CEILING           (220) // deg C
#define HEATER_UNDERHEAT_TRIGGER_PERIOD (5600)
#define HEATER_EXPECTED_ELEM_TEMP_RISE  (30)
#define HEATER_EXPECTED_POT_TEMP_RISE   (15)

#define THERMISTOR_MIN_TEMP             (-10.f)
#define THERMISTOR_MAX_TEMP             (250.f)
#define THERMISTOR_HEADROOM             (4.f)
#define HEATERCTL_INTERVAL              (50) // ms

static bitmask8_t heater_ctl_faults = 0;

static float heater_pot_temp_target = 0;
static bool heater_element_enabled = false;

static uint32_t heater_on_timestamp = 0;
static float heater_on_elem_tmp = 0;
static float heater_on_pot_tmp = 0;

enum : uint8_t {
    HEATER_OVERHEAT_FAULT = 1 << 0,
    HEATER_UNDERHEAT_FAULT = 1 << 1,
};

void heater_set_target_pot_temp(const float temp) {
    critical heater_pot_temp_target = fclampf(temp, THERMISTOR_MIN_TEMP, THERMISTOR_MAX_TEMP);
}

void heater_set_enabled(const bool enabled) {
    critical heater_element_enabled = enabled;
}

float heater_get_target_pot_temp(void) {
    critical_ret (heater_pot_temp_target);
}

bool heater_is_enabled(void) {
    critical_ret (heater_element_enabled);
}

bitmask8_t heater_get_faults(void) {
    critical_ret (heater_ctl_faults);
}

void heater_clear_faults(void) {
    critical heater_ctl_faults = 0x0;
}

void task_heater_update_thread(void *args) {
    while (1) {
        xthreads_csenter(); // critical section
        const bool local_heater_enabled = heater_element_enabled;
        const float local_pot_target_temp = heater_pot_temp_target;
        const bitmask8_t local_heater_ctl_faults = heater_ctl_faults;
        xthreads_csexit();

        if (!local_heater_enabled || local_heater_ctl_faults != 0u) {
            rc_heater_set(false);
            xthreads_sleep(HEATERCTL_INTERVAL);
            continue;
        }

        const float heater_tmp = rc_sensor_get_temp(RC_SENSOR_HEATER);
        const float pot_tmp = rc_sensor_get_temp(RC_SENSOR_POT);
        const uint32_t current_time = rc_time_current_millis();
        const bool heater_currently_on = rc_heater_is_on();

        const bool heater_should_start = pot_tmp < local_pot_target_temp - THERMISTOR_HEADROOM;
        const bool heater_should_stop = pot_tmp > local_pot_target_temp + THERMISTOR_HEADROOM;

        // overheat protection
        if (heater_tmp >= HEATER_SAFETY_CEILING) {
            rc_heater_set(false); // emergency off
            critical {
                heater_ctl_faults |= HEATER_OVERHEAT_FAULT;
                heater_element_enabled = false; // just to be sure
            }
            goto proc_end;
        }

        // underheat protection
        if (!heater_should_stop && heater_currently_on
            && current_time - heater_on_timestamp > HEATER_UNDERHEAT_TRIGGER_PERIOD
        ) {
            const float tmp_heater_rise = heater_tmp - heater_on_elem_tmp;
            const float tmp_pot_rise = pot_tmp - heater_on_pot_tmp;
            // failed to raise properly (on both sensors) after X seconds
            if (tmp_heater_rise < HEATER_EXPECTED_ELEM_TEMP_RISE
                && tmp_pot_rise < HEATER_EXPECTED_POT_TEMP_RISE
            ) {
                rc_heater_set(false); // emergency off
                critical {
                    heater_ctl_faults |= HEATER_UNDERHEAT_FAULT;
                    heater_element_enabled = false; // just to be sure
                }
                goto proc_end;
            }
            // rearm the underheat protection fault
            heater_on_timestamp = current_time;
            heater_on_elem_tmp = heater_tmp;
            heater_on_pot_tmp = pot_tmp;
        }

        // headroom to prevent rapid relay clicking
        if (heater_should_start) {
            if (!heater_currently_on) {
                // for underheat protection fault
                heater_on_timestamp = current_time;
                heater_on_elem_tmp = heater_tmp;
                heater_on_pot_tmp = pot_tmp;
            }
            rc_heater_set(true);
        } else if (heater_should_stop) {
            rc_heater_set(false);
        }
        proc_end: xthreads_sleep(HEATERCTL_INTERVAL);
    }
}