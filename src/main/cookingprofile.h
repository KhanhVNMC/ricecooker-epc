#ifndef RICECOOKER_COOKING_PROFILE_H
#define RICECOOKER_COOKING_PROFILE_H
#include <stdint.h>
#include "ricecooker.h"

#define COOKING_KEEP_WARM_TEMP      (65.0f)

typedef enum : uint8_t {
    TRIGGER_TIME_EXPIRED, // turn to next stage if timeout
    TRIGGER_POT_TEMP_REACHED, // overheat
    TRIGGER_WATER_BOIL_DRY, // no water left
} stage_trigger_type_t;

typedef struct {
    uint32_t t_stage_in_mins;
    float temp;
} temp_point_t;

typedef struct {
    const temp_point_t *temp_points;
    uint8_t temp_point_count;
    stage_trigger_type_t trigger_type; // transitional stage condition
    uint32_t ceiling_duration_ms; // ceiling duration (ms), will pass to next stage regardless
    float temp_threshold;
} cooking_stage_t;

typedef struct {
    uint8_t stage_count;
    const cooking_stage_t *stages;
    bool auto_hold_warm;
} cooking_profile_t;

/**
 * Get cooking profile of each mode
 * If mode = MODE_MANUAL_USER, return nullptr
 */
const cooking_profile_t* cooking_get_profile(cooking_mode_t mode);

#endif //RICECOOKER_COOKING_PROFILE_H
