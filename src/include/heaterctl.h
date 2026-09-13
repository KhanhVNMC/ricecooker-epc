#ifndef RICECOOKER_HEATERCTL_H
#define RICECOOKER_HEATERCTL_H
#include "include/rchal.h"

typedef enum : bitmask8_t {
    HEATER_OVERHEAT_FAULT = 1 << 0,
    HEATER_UNDERHEAT_FAULT = 1 << 1,
} heater_faults_t;

void heater_set_target_pot_temp(float temp);
void heater_set_enabled(bool enabled);

float heater_get_target_pot_temp(void);
bool heater_is_enabled(void);

heater_faults_t heater_get_faults(void);
void heater_clear_faults(void);

void task_heater_update_thread(void *args);

#endif //RICECOOKER_HEATERCTL_H