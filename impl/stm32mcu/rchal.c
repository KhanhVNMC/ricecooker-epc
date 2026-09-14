#include "include/rchal.h"
#include "stm32f1xx_hal.h"

// TODO EVERYTHING (Except the timer ig)
void rc_init_rchal(void) {

}

void rc_tm1637dp_start(void) {
}

void rc_tm1637dp_write_byte(uint8_t byte) {

}

void rc_tm1637dp_stop(void) {
}

void rc_buzzer_play(void) {
}

float rc_sensor_get_temp(rc_temp_sensor_t sensor) {
    (void)sensor;
    return 0.0F;
}

bitmask8_t rc_buttons_get_state(void) {
    return 0U;
}

void rc_heater_set(bool on) {
}

bool rc_heater_is_on(void) {
}

uint32_t rc_time_current_millis(void) {
    return HAL_GetTick();
}

