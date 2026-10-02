#include "cookingprofile.h"

// 1. Nấu cơm bình thường (Ngâm 60°C -> Đun sôi 100°C cạn nước -> Ủ chín 95°C -> Giữ ấm)
static const temp_setpoint_t STAGE_NORMAL_SOAK_POINTS[] = {
    {0u, 25.0f}, {5u, 40.0f}, {15u, 40.0f}
};
static const temp_setpoint_t STAGE_NORMAL_BOIL_POINTS[] = {
    {0u, 40.0f}, {5u, 100.0f}, {20u, 100.0f}
};
static const temp_setpoint_t STAGE_NORMAL_COOK_POINTS[] = {
    {0u, 100.0f}, {5u, 80.0f}, {17u, 80.0f}
};
static const cooking_stage_t STAGES_RICE_NORMAL[] = {
    {
        .temp_setpoints = STAGE_NORMAL_SOAK_POINTS,
        .temp_setpoint_count = sizeof(STAGE_NORMAL_SOAK_POINTS) / sizeof(temp_setpoint_t),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 15u * 60u * 1000u, // 15 phút ngâm gạo
        .temp_reached_threshold = 0.0f
    },
    {
        .temp_setpoints = STAGE_NORMAL_BOIL_POINTS,
        .temp_setpoint_count = sizeof(STAGE_NORMAL_BOIL_POINTS) / sizeof(temp_setpoint_t),
        .trigger_type = TRIGGER_WATER_BOIL_DRY,
        .ceiling_duration_ms = 20u * 60u * 1000u, // timeout an toàn 20 phút
        .temp_reached_threshold = 105.0f // ngưỡng phát hiện cạn nước
    },
    {
        .temp_setpoints = STAGE_NORMAL_COOK_POINTS,
        .temp_setpoint_count = sizeof(STAGE_NORMAL_COOK_POINTS) / sizeof(temp_setpoint_t),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 17u * 60u * 1000u, // 17 phút hâm nóng
        .temp_reached_threshold = 0.0f
    }
};

static const cooking_profile_t PROFILE_RICE_NORMAL = {
    .stage_count    = sizeof(STAGES_RICE_NORMAL) / sizeof(cooking_stage_t),
    .stages         = STAGES_RICE_NORMAL,
    .auto_hold_warm = true
};

// 2. Nấu cơm nhanh (Đun sôi tối đa 100°C cạn nước -> Ủ nhanh 95°C -> Giữ ấm)
static const temp_setpoint_t STAGE_FAST_BOIL_POINTS[] = {
    {0u, 25.0f}, {5u, 100.0f}, {15u, 100.0f}
};
static const temp_setpoint_t STAGE_FAST_COOK_POINTS[] = {
    {0u, 100.0f}, {5u, 95.0f}, {20u, 95.0f}
};
static const cooking_stage_t STAGES_FAST_RICE[] = {
    {
        .temp_setpoints = STAGE_FAST_BOIL_POINTS,
        .temp_setpoint_count = sizeof(STAGE_FAST_BOIL_POINTS) / sizeof(temp_setpoint_t),
        .trigger_type = TRIGGER_WATER_BOIL_DRY,
        .ceiling_duration_ms = 15u * 60u * 1000u, // timeout an toàn 15 phút
        .temp_reached_threshold  = 105.0f
    },
    {
        .temp_setpoints = STAGE_FAST_COOK_POINTS,
        .temp_setpoint_count = sizeof(STAGE_FAST_COOK_POINTS) / sizeof(temp_setpoint_t),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 20u * 60u * 1000u, // 20 phút ủ nhanh
        .temp_reached_threshold  = 0.0f
    }
};

static const cooking_profile_t PROFILE_FAST_RICE = {
    .stage_count = sizeof(STAGES_FAST_RICE) / sizeof(cooking_stage_t),
    .stages = STAGES_FAST_RICE,
    .auto_hold_warm = true
};

// 3. Nấu cháo (Đun sôi 100°C -> Sôi lăn tăn nhỏ lửa 88°C chống trào bọt -> Giữ ấm)
static const temp_setpoint_t STAGE_PORRIDGE_BOIL_POINTS[] = {
    {0u, 25.0f}, {5u, 100.0f}, {15u, 100.0f}
};
static const temp_setpoint_t STAGE_PORRIDGE_SIMMER_POINTS[] = {
    {0u, 100.0f}, {5u, 97.0f}, {40u, 97.0f}
};
static const temp_setpoint_t STAGE_PORRIDGE_FINISH_POINTS[] = {
    {0u, 97.0f}, {3u, 90.0f}, {8u, 90.0f}
};
static const cooking_stage_t STAGES_PORRIDGE[] = {
    {
        .temp_setpoints = STAGE_PORRIDGE_BOIL_POINTS,
        .temp_setpoint_count = sizeof(STAGE_PORRIDGE_BOIL_POINTS) / sizeof(temp_setpoint_t),
        .trigger_type = TRIGGER_POT_TEMP_REACHED,
        .ceiling_duration_ms  = 15u * 60u * 1000u, // timeout nếu quá 15 phút chưa sôi
        .temp_reached_threshold = 100.f // đạt điểm sôi
    },
    {
        .temp_setpoints = STAGE_PORRIDGE_SIMMER_POINTS,
        .temp_setpoint_count = sizeof(STAGE_PORRIDGE_SIMMER_POINTS) / sizeof(temp_setpoint_t),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 40u * 60u * 1000u, // 40 phút ninh cháo liu riu không trào
        .temp_reached_threshold  = 0.0f
    },
    {
        .temp_setpoints = STAGE_PORRIDGE_FINISH_POINTS,
        .temp_setpoint_count = sizeof(STAGE_PORRIDGE_FINISH_POINTS) / sizeof(temp_setpoint_t),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 8u * 60u * 1000u, // timeout nếu quá 8 phút chưa sôi
        .temp_reached_threshold = 0.0f
    },

};

static const cooking_profile_t PROFILE_PORRIDGE = {
    .stage_count    = sizeof(STAGES_PORRIDGE) / sizeof(cooking_stage_t),
    .stages         = STAGES_PORRIDGE,
    .auto_hold_warm = true
};

// 4. Ninh / Hầm mềm 
static const temp_setpoint_t STAGE_SOFT_BOIL_POINTS[] = {
    {0u, 25.0f}, {10u, 100.0f}, {20u, 100.0f}
};
static const temp_setpoint_t STAGE_SOFT_COOLDOWN_POINTS[] = {
    {0u, 100.0f}, {5u, 75.0f}
};
static const temp_setpoint_t STAGE_SOFT_SIMMER_POINTS[] = {
    {0u, 75.0f}, {10u, 95.0f}, {120u, 95.0f}
};
static const cooking_stage_t STAGES_SOFT_SIMMER[] = {
    {
        .temp_setpoints = STAGE_SOFT_BOIL_POINTS,
        .temp_setpoint_count = sizeof(STAGE_SOFT_BOIL_POINTS) / sizeof(temp_setpoint_t),
        .trigger_type = TRIGGER_POT_TEMP_REACHED,
        .ceiling_duration_ms = 20u * 60u * 1000u,
        .temp_reached_threshold = 105.0f
    },
    {
        .temp_setpoints = STAGE_SOFT_COOLDOWN_POINTS,
        .temp_setpoint_count = sizeof(STAGE_SOFT_COOLDOWN_POINTS) / sizeof(temp_setpoint_t),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 5u * 60u * 1000u, // 5 phút hạ nhiệt
        .temp_reached_threshold = 0.0f
    },
    {
        .temp_setpoints = STAGE_SOFT_SIMMER_POINTS,
        .temp_setpoint_count = sizeof(STAGE_SOFT_SIMMER_POINTS) / sizeof(temp_setpoint_t),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms  = 2u * 60u * 60u * 1000u, // 120 phút hầm mềm
        .temp_reached_threshold  = 0.0f
    }
};

static const cooking_profile_t PROFILE_SOFT_SIMMER = {
    .stage_count    = sizeof(STAGES_SOFT_SIMMER) / sizeof(cooking_stage_t),
    .stages         = STAGES_SOFT_SIMMER,
    .auto_hold_warm = true
};

// 5. Làm bánh Flan
static const temp_setpoint_t STAGE_FLAN_STEAM_POINTS[] = {
    {0u, 25.0f}, {5u, 110.0f}, {10u, 110.0f}
};
static const temp_setpoint_t STAGE_FLAN_BAKE_POINTS[] = {
    {0u, 110.0f}, {10u, 140.0f}, {40u, 140.0f}
};
static const temp_setpoint_t STAGE_FLAN_COOLDOWN_POINTS[] = {
    {0u, 140.0f}, {10u, 80.0f}
};
static const cooking_stage_t STAGES_FLAN_DRE[] = {
    {
        .temp_setpoints = STAGE_FLAN_STEAM_POINTS,
        .temp_setpoint_count = sizeof(STAGE_FLAN_STEAM_POINTS) / sizeof(temp_setpoint_t),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 10u * 60u * 1000u, // 10 phút hấp bánh
        .temp_reached_threshold  = 0.0f
    },
    {
        .temp_setpoints = STAGE_FLAN_BAKE_POINTS,
        .temp_setpoint_count = sizeof(STAGE_FLAN_BAKE_POINTS) / sizeof(temp_setpoint_t),
        .trigger_type = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms = 40u * 60u * 1000u,
        .temp_reached_threshold  = 0.0f
    },
    {
        .temp_setpoints = STAGE_FLAN_COOLDOWN_POINTS,
        .temp_setpoint_count = sizeof(STAGE_FLAN_COOLDOWN_POINTS) / sizeof(temp_setpoint_t),
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
