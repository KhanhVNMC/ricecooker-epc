#include "cookingprofile.h"
#include "include/cmath.h"

// all setpoints here are FOOD temperatures; the pot is always hotter than the
// food, so every target here is REBASED before it reaches the heater ctler
// EXCEPT temp_reached_threshold, that one is POT, so not rebased

// 1. Nấu cơm bình thường (Ngâm -> Đun sôi tới cạn nước -> Ủ chín -> Giữ ấm)
static const temp_setpoint_t STAGE_NORMAL_SOAK_POINTS[] = {
    {0u, 45.0f},
    {3u, 55.0f},
    {7u, 70.0f}
};
static const temp_setpoint_t STAGE_NORMAL_BOIL_POINTS[] = {
    {0u, 105.f},
    {35u, COOKING_BOIL_DRIVE_TEMP},
};
static const temp_setpoint_t STAGE_NORMAL_COOK_POINTS[] = {
    {0u, 85.0f},
    {5u, 80.0f},
    {12u, 70.0f}
};
static const cooking_stage_t STAGES_RICE_NORMAL[] = {
    {
        .temp_setpoints = STAGE_NORMAL_SOAK_POINTS,
        .temp_setpoint_count = arrsize(STAGE_NORMAL_SOAK_POINTS),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 10u * 60u * 1000u, // 15 phút ngâm gạo
        .temp_reached_threshold = 0.0f
    },
    {
        .temp_setpoints = STAGE_NORMAL_BOIL_POINTS,
        .temp_setpoint_count = arrsize(STAGE_NORMAL_BOIL_POINTS),
        .trigger_type = TRIGGER_WATER_BOIL_DRY,
        .ceiling_duration_ms = 35u * 60u * 1000u, // timeout an toàn 35 phút
        .temp_reached_threshold = 0.0f // không dùng: cạn nước do has_water_boiled_away() quyết định
    },
    {
        .temp_setpoints = STAGE_NORMAL_COOK_POINTS,
        .temp_setpoint_count = arrsize(STAGE_NORMAL_COOK_POINTS),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 15u * 60u * 1000u, // 17 phút hâm nóng
        .temp_reached_threshold = 0.0f
    }
};

static const cooking_profile_t PROFILE_RICE_NORMAL = {
    .stage_count    = arrsize(STAGES_RICE_NORMAL),
    .stages         = STAGES_RICE_NORMAL,
    .auto_hold_warm = true
};

// 2. Nấu cơm nhanh (Đun sôi tới cạn nước -> Ủ nhanh -> Giữ ấm)
static const temp_setpoint_t STAGE_FAST_BOIL_POINTS[] = {
    {0u, 25.0f},
    {5u, COOKING_BOIL_DRIVE_TEMP},
    {30u, COOKING_BOIL_DRIVE_TEMP}
};
static const temp_setpoint_t STAGE_FAST_COOK_POINTS[] = {
    {0u, 90.0f},
    {5u, 85.0f},
    {20u, 85.0f}
};
static const cooking_stage_t STAGES_FAST_RICE[] = {
    {
        .temp_setpoints = STAGE_FAST_BOIL_POINTS,
        .temp_setpoint_count = arrsize(STAGE_FAST_BOIL_POINTS),
        .trigger_type = TRIGGER_WATER_BOIL_DRY,
        .ceiling_duration_ms = 30u * 60u * 1000u, // timeout an toàn 30 phút
        .temp_reached_threshold  = 0.0f
    },
    {
        .temp_setpoints = STAGE_FAST_COOK_POINTS,
        .temp_setpoint_count = arrsize(STAGE_FAST_COOK_POINTS),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 20u * 60u * 1000u, // 20 phút ủ nhanh
        .temp_reached_threshold  = 0.0f
    }
};

static const cooking_profile_t PROFILE_FAST_RICE = {
    .stage_count = arrsize(STAGES_FAST_RICE),
    .stages = STAGES_FAST_RICE,
    .auto_hold_warm = true
};

// 3. Nấu cháo (Đun sôi -> Sôi lăn tăn nhỏ lửa chống trào bọt -> Giữ ấm)
static const temp_setpoint_t STAGE_PORRIDGE_BOIL_POINTS[] = {
    {0u, 25.0f},
    {5u, COOKING_BOIL_DRIVE_TEMP},
    {20u, COOKING_BOIL_DRIVE_TEMP}
};
static const temp_setpoint_t STAGE_PORRIDGE_SIMMER_POINTS[] = {
    {0u, 95.0f},
    {5u, 92.0f},
    {40u, 92.0f}
};
static const temp_setpoint_t STAGE_PORRIDGE_FINISH_POINTS[] = {
    {0u, 92.0f},
    {3u, 88.0f},
    {8u, 88.0f}
};
static const cooking_stage_t STAGES_PORRIDGE[] = {
    {
        .temp_setpoints = STAGE_PORRIDGE_BOIL_POINTS,
        .temp_setpoint_count = arrsize(STAGE_PORRIDGE_BOIL_POINTS),
        .trigger_type = TRIGGER_POT_TEMP_REACHED,
        .ceiling_duration_ms  = 20u * 60u * 1000u, // timeout nếu quá 20 phút chưa sôi
        .temp_reached_threshold = COOKING_BOIL_REACHED_TEMP // đạt điểm sôi
    },
    {
        .temp_setpoints = STAGE_PORRIDGE_SIMMER_POINTS,
        .temp_setpoint_count = arrsize(STAGE_PORRIDGE_SIMMER_POINTS),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 40u * 60u * 1000u, // 40 phút ninh cháo liu riu không trào
        .temp_reached_threshold  = 0.0f
    },
    {
        .temp_setpoints = STAGE_PORRIDGE_FINISH_POINTS,
        .temp_setpoint_count = arrsize(STAGE_PORRIDGE_FINISH_POINTS),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 8u * 60u * 1000u, // timeout nếu quá 8 phút chưa sôi
        .temp_reached_threshold = 0.0f
    },

};

static const cooking_profile_t PROFILE_PORRIDGE = {
    .stage_count    = arrsize(STAGES_PORRIDGE),
    .stages         = STAGES_PORRIDGE,
    .auto_hold_warm = true
};

// 4. Ninh / Hầm mềm 
static const temp_setpoint_t STAGE_SOFT_BOIL_POINTS[] = {
    {0u, 25.0f},
    {10u, COOKING_BOIL_DRIVE_TEMP},
    {25u, COOKING_BOIL_DRIVE_TEMP}
};
static const temp_setpoint_t STAGE_SOFT_COOLDOWN_POINTS[] = {
    {0u, 95.0f},
    {5u, 75.0f}
};
static const temp_setpoint_t STAGE_SOFT_SIMMER_POINTS[] = {
    {0u, 75.0f},
    {10u, 92.0f},
    {120u, 92.0f}
};
static const cooking_stage_t STAGES_SOFT_SIMMER[] = {
    {
        .temp_setpoints = STAGE_SOFT_BOIL_POINTS,
        .temp_setpoint_count = arrsize(STAGE_SOFT_BOIL_POINTS),
        .trigger_type = TRIGGER_POT_TEMP_REACHED,
        .ceiling_duration_ms = 25u * 60u * 1000u,
        .temp_reached_threshold = COOKING_BOIL_REACHED_TEMP
    },
    {
        .temp_setpoints = STAGE_SOFT_COOLDOWN_POINTS,
        .temp_setpoint_count = arrsize(STAGE_SOFT_COOLDOWN_POINTS),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 5u * 60u * 1000u, // 5 phút hạ nhiệt
        .temp_reached_threshold = 0.0f
    },
    {
        .temp_setpoints = STAGE_SOFT_SIMMER_POINTS,
        .temp_setpoint_count = arrsize(STAGE_SOFT_SIMMER_POINTS),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms  = 2u * 60u * 60u * 1000u, // 120 phút hầm mềm
        .temp_reached_threshold  = 0.0f
    }
};

static const cooking_profile_t PROFILE_SOFT_SIMMER = {
    .stage_count    = arrsize(STAGES_SOFT_SIMMER),
    .stages         = STAGES_SOFT_SIMMER,
    .auto_hold_warm = true
};

// 5. Làm bánh Flan
static const temp_setpoint_t STAGE_FLAN_STEAM_POINTS[] = {
    {0u, 25.0f}, {5u, 87.0f}, {10u, 87.0f}
};
static const temp_setpoint_t STAGE_FLAN_BAKE_POINTS[] = {
    {0u, 87.0f}, {10u, 112.0f}, {40u, 112.0f}
};
static const temp_setpoint_t STAGE_FLAN_COOLDOWN_POINTS[] = {
    {0u, 112.0f}, {10u, 65.0f}
};
static const cooking_stage_t STAGES_FLAN_DRE[] = {
    {
        .temp_setpoints = STAGE_FLAN_STEAM_POINTS,
        .temp_setpoint_count = arrsize(STAGE_FLAN_STEAM_POINTS),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 10u * 60u * 1000u, // 10 phút hấp bánh
        .temp_reached_threshold  = 0.0f
    },
    {
        .temp_setpoints = STAGE_FLAN_BAKE_POINTS,
        .temp_setpoint_count = arrsize(STAGE_FLAN_BAKE_POINTS),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 40u * 60u * 1000u,
        .temp_reached_threshold  = 0.0f
    },
    {
        .temp_setpoints = STAGE_FLAN_COOLDOWN_POINTS,
        .temp_setpoint_count = arrsize(STAGE_FLAN_COOLDOWN_POINTS),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 10u * 60u * 1000u,
        .temp_reached_threshold = 120.0f
    }
};

static const cooking_profile_t PROFILE_FLAN_DRE = {
    .stage_count    = sizeof(STAGES_FLAN_DRE) / sizeof(cooking_stage_t),
    .stages         = STAGES_FLAN_DRE,
    .auto_hold_warm = false // Không giữ ấm bánh flan sau khi hấp
};

const cooking_profile_t* cooking_get_profile(const cooking_mode_t mode) {
    switch (mode) {
        case MODE_RICE_NORMAL: return &PROFILE_RICE_NORMAL;
        case MODE_FAST_RICE:   return &PROFILE_FAST_RICE;
        case MODE_PORRIDGE:    return &PROFILE_PORRIDGE;
        case MODE_SOFT_SIMMER: return &PROFILE_SOFT_SIMMER;
        case MODE_FLAN_DRE:    return &PROFILE_FLAN_DRE;
        default:               return nullptr;
    }
}

inline float cooking_rebase_temp(const float temp) {
    // 0 at room temp, 1 at boiling and above
    const float scale =
        temp >= REBASE_WATER_BOIL_TEMP ? 1.0f : temp <= REBASE_ROOM_TEMP ? 0.0f :
        (temp - REBASE_ROOM_TEMP) / (REBASE_WATER_BOIL_TEMP - REBASE_ROOM_TEMP) // basically lerp
    ;
    return temp + REBASE_BOIL_TEMP_OFFSET * scale;
}
