#ifndef RICECOOKER_RCHAL_H
#define RICECOOKER_RCHAL_H
#include <stdint.h>

typedef uint8_t bitmask8_t;

typedef enum : uint8_t {
    RC_SENSOR_POT,
    RC_SENSOR_HEATER,
    COUNT_RC_SENSORS
} rc_temp_sensor_t;

enum : bitmask8_t {
    RC_BTN_SELECT  = 1u << 0, // chon
    RC_BTN_CANCEL  = 1u << 1, // huy
    RC_BTN_OPTIONS = 1u << 2, // che do
    RC_BTN_INCR    = 1u << 3, // increment
    RC_BTN_DECR    = 1u << 4  // decrement
};

void rc_init_hal(void);

void rc_tm1637dp_start(void);
void rc_tm1637dp_write_byte(uint8_t byte);
void rc_tm1637dp_stop(void);

void rc_buzzer_play(void);

float rc_sensor_get_temp(rc_temp_sensor_t sensor);

bitmask8_t rc_buttons_get_state(void);

void rc_heater_set(bool on);
bool rc_heater_is_on(void);

uint32_t rc_time_current_millis(void);

#endif //RICECOOKER_RCHAL_H