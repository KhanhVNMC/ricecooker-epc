#include "include/heaterctl.h"
#include "include/cmath.h"
#include "include/platform.h"
#include "include/xthreads.h"

#define HEATER_ELEMENT_MAX_SAFETY_CEILING   (320.f)
#define HEATER_POT_MAX_SAFETY_CEILING       (160.f) // deg C
#define GLOBAL_MIN_SAFETY_CEILING           (0.f) // NEVER happen if the sensors arent cooked

#define WD_ELEMENT_RISE_THRESHOLD           (15.f) // what the f is this name
#define WD_POT_DROP_THRESHOLD               (15.f) // still

#define ALLOWED_POT_MIN_TEMP                (10.f)
#define ALLOWED_POT_MAX_TEMP                (200.f)
#define THERMISTOR_HEADROOM                 (4.f)

#define HEATER_WATCHDOG_INTERVAL_MS         (5000)
#define HEATERCTL_INTERVAL_MS               (25u) // ms

static heater_faults_t heater_ctl_faults = 0;

static float heater_pot_temp_target = 0;
static bool heater_element_enabled = false;

static float cached_sensors_data[COUNT_RC_SENSORS];

static uint32_t heater_wd_last_timestamp = 0;
static float heater_wd_last_elem_tmp = 0;
static float heater_wd_last_pot_tmp = 0;

void heater_set_target_pot_temp(const float temp) {
    critical heater_pot_temp_target = fclampf(temp, ALLOWED_POT_MIN_TEMP, ALLOWED_POT_MAX_TEMP);
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

heater_faults_t heater_get_faults(void) {
    critical_ret (heater_ctl_faults);
}

void heater_clear_faults(void) {
    critical heater_ctl_faults = 0x0;
}

float heater_read_cached_sensor(const rc_temp_sensor_t sensor) {
    return cached_sensors_data[sensor];
}

void task_heater_update_thread(void *args) {
    while (1) {
        // Pet watchdog trong thread heater (mỗi 25ms)
        platform_watchdog_pet();

        // cache here so other threads can read w/o contention
        cached_sensors_data[RC_SENSOR_POT] = rc_sensor_get_temp(RC_SENSOR_POT);
        cached_sensors_data[RC_SENSOR_HEATER] = rc_sensor_get_temp(RC_SENSOR_HEATER);

        xthreads_csenter(); // critical section
        const bool local_heater_enabled = heater_element_enabled;
        const float local_pot_target_temp = heater_pot_temp_target;
        const bitmask8_t local_heater_ctl_faults = heater_ctl_faults;
        xthreads_csexit();

        if (!local_heater_enabled || local_heater_ctl_faults != 0u) {
            rc_heater_set(false);
            critical heater_element_enabled = false;
            xthreads_sleep(HEATERCTL_INTERVAL_MS);
            continue;
        }

        const float heater_tmp = cached_sensors_data[RC_SENSOR_HEATER];
        const float pot_tmp = cached_sensors_data[RC_SENSOR_POT];
        const uint32_t current_time = rc_time_current_millis();
        const bool heater_currently_on = rc_heater_is_on();

        const bool heater_should_start = pot_tmp < local_pot_target_temp - THERMISTOR_HEADROOM;
        const bool heater_should_stop = pot_tmp > local_pot_target_temp + THERMISTOR_HEADROOM;

        // overheat protection (element)
        const bool element_overheat = heater_tmp >= HEATER_ELEMENT_MAX_SAFETY_CEILING;
        const bool pot_overheat = pot_tmp >= HEATER_POT_MAX_SAFETY_CEILING;

        if (element_overheat || pot_overheat) {
            rc_heater_set(false); // emergency off
            critical {
                if (element_overheat) heater_ctl_faults |= HEATER_ELEMENT_OVERHEAT_FAULT;
                if (pot_overheat)     heater_ctl_faults |= HEATER_POT_OVERHEAT_FAULT;
                heater_element_enabled = false; // just to be sure
            }
            goto proc_end;
        }

        // severe "underheat" (could be rouge sensor) protection
        const bool element_underheat = heater_tmp <= GLOBAL_MIN_SAFETY_CEILING;
        const bool pot_underheat = pot_tmp <= GLOBAL_MIN_SAFETY_CEILING;

        if (element_underheat || pot_underheat) {
            rc_heater_set(false); // emergency off
            critical {
                if (element_underheat) heater_ctl_faults |= HEATER_ELEMENT_UNDERHEAT_FAULT;
                if (pot_underheat)     heater_ctl_faults |= HEATER_POT_UNDERHEAT_FAULT;
                heater_element_enabled = false; // just to be sure
            }
            goto proc_end;
        }

        // watchdog periodic protection
        if (heater_currently_on && current_time - heater_wd_last_timestamp >= HEATER_WATCHDOG_INTERVAL_MS) {
            const float heater_delta = heater_tmp - heater_wd_last_elem_tmp;
            const float pot_delta = pot_tmp - heater_wd_last_pot_tmp;

            if (!heater_should_stop) {
                // sometimes i want to fucking kill myself
                // this is just retarded
                if (heater_delta > WD_ELEMENT_RISE_THRESHOLD
                    && pot_delta < -WD_POT_DROP_THRESHOLD
                ) {
                    rc_heater_set(false); // emergency off
                    critical {
                        heater_ctl_faults |= HEATER_POOR_CONTACT_FAULT;
                        heater_element_enabled = false; // just to be sure
                    }
                    goto proc_end;
                }
            }

            // record the last stats
            heater_wd_last_timestamp = current_time;
            heater_wd_last_elem_tmp = heater_tmp;
            heater_wd_last_pot_tmp = pot_tmp;
        }

        // headroom to prevent rapid relay clicking
        if (heater_should_start) {
            // OFF -> ON state transition
            if (!heater_currently_on) {
                heater_wd_last_timestamp = current_time;
                heater_wd_last_elem_tmp = heater_tmp;
                heater_wd_last_pot_tmp = pot_tmp;
            }
            rc_heater_set(true);
        } else if (heater_should_stop) {
            rc_heater_set(false);
        }
        proc_end: xthreads_sleep(HEATERCTL_INTERVAL_MS);
    }
}