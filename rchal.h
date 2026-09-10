#ifndef RICECOOKER_RCHAL_H
#define RICECOOKER_RCHAL_H
#include <stdint.h>

typedef enum {
    RC_SENSOR_POT,
    RC_SENSOR_HEATER,
    COUNT_RC_SENSORS
} rc_temp_sensor_t;

void rc_init_hal(void);
void rc_display_set_segments(uint8_t bitmask);
void rc_display_digit_set(uint8_t digit, bool on);

float rc_sensor_get_temp(rc_temp_sensor_t sensor);

void rc_heater_set(bool on);


uint32_t rc_time_current_millis(void);

#endif //RICECOOKER_RCHAL_H