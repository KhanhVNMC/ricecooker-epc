#ifndef RICECOOKER_HEATERCTL_H
#define RICECOOKER_HEATERCTL_H
#include "include/rchal.h"

void heater_set_target_temp(float temp);
void heater_set_enabled(bool enabled);

float heater_get_target_temp(void);
bool heater_is_enabled(void);

void task_heater_update_thread(void *args);

#endif //RICECOOKER_HEATERCTL_H