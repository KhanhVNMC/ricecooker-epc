#ifndef MICROTIME_H
#define MICROTIME_H
#include <stdint.h>

void mt_us_timer_init(void);
uint16_t mt_time_cnt_micros(void);
void mt_delay_blocking_us(uint16_t delay);

#endif //MICROTIME_H