#ifndef RICECOOKER_COOKING_PROFILE_H
#define RICECOOKER_COOKING_PROFILE_H
#include <stdint.h>
#include "ricecooker.h"

#define COOKING_KEEP_WARM_TEMP            (65.0f)
// TODO: fien tune this once we have the cooker
#define WATER_BOIL_PLATEAU_MIN_TEMP       (98.0f)
#define WATER_BOIL_PLATEAU_MAX_ABS_SLOPE  (0.015f)
#define WATER_BOIL_DRY_MIN_SLOPE          (0.05f) // straight from my ass

typedef enum : uint8_t {
    TRIGGER_TIME_EXPIRED, // turn to next stage if timeout
    TRIGGER_POT_TEMP_REACHED, // reached desired temp
    TRIGGER_WATER_BOIL_DRY, // no water left
} stage_trigger_type_t;

typedef struct {
    uint32_t start_time_min;
    float start_temp_val;
} temp_setpoint_t;

typedef struct {
    stage_trigger_type_t trigger_type; // transitional stage condition
    uint32_t ceiling_duration_ms; // ceiling duration (ms), will pass to next stage regardless
    float temp_reached_threshold; // (only for TRIGGER_POT_TEMP_REACHED) temp before next
    const temp_setpoint_t *temp_setpoints; // for temp control
    uint8_t temp_setpoint_count;
} cooking_stage_t;

typedef struct {
    uint8_t stage_count;
    const cooking_stage_t *stages;
    bool auto_hold_warm;
} cooking_profile_t;

/**
 * Get cooking profile of each mode
 * If mode = MODE_MANUAL_USER or KEEP WARM, return nullptr
 */
const cooking_profile_t* cooking_get_profile(cooking_mode_t mode);

#endif //RICECOOKER_COOKING_PROFILE_H
