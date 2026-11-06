#ifndef RICECOOKER_TEMPSTATS_H
#define RICECOOKER_TEMPSTATS_H
#include <stdint.h>

#define SAMPLES_RETENTION_SIZE  (16)

void tempstats_reset(void);
bool tempstats_is_full(void);
uint8_t tempstats_sample_size(void);

void tempstats_collect_sample(float temp);
float tempstats_get_at(uint8_t i);

float tempstats_get_avg(void);
float tempstats_linreg_slope(void);

#endif //RICECOOKER_TEMPSTATS_H