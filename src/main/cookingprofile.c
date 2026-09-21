#include "cookingprofile.h"

// 1. Nấu cơm bình thường (Ngâm 60°C -> Đun sôi 100°C cạn nước -> Ủ chín 95°C -> Giữ ấm)
static const cooking_stage_t STAGES_RICE_NORMAL[] = {
    {
        .target_pot_temp = 60.0f,
        .trigger_type    = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms     = 8u * 60u * 1000u, // 8 phút ngâm gạo
        .temp_threshold  = 0.0f
    },
    {
        .target_pot_temp = 100.0f,
        .trigger_type    = TRIGGER_WATER_BOIL_DRY,
        .ceiling_duration_ms     = 40u * 60u * 1000u, // timeout an toàn 40 phút
        .temp_threshold  = 105.0f // ngưỡng phát hiện cạn nước
    },
    {
        .target_pot_temp = 95.0f,
        .trigger_type    = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms     = 12u * 60u * 1000u, // 12 phút ủ chín
        .temp_threshold  = 0.0f
    }
};

static const cooking_profile_t PROFILE_RICE_NORMAL = {
    .stage_count    = sizeof(STAGES_RICE_NORMAL) / sizeof(cooking_stage_t),
    .stages         = STAGES_RICE_NORMAL,
    .auto_hold_warm = true
};

// 2. Nấu cơm nhanh (Đun sôi tối đa 100°C cạn nước -> Ủ nhanh 95°C -> Giữ ấm)
static const cooking_stage_t STAGES_FAST_RICE[] = {
    {
        .target_pot_temp = 100.0f,
        .trigger_type    = TRIGGER_WATER_BOIL_DRY,
        .ceiling_duration_ms     = 30u * 60u * 1000u, // timeout an toàn 30 phút
        .temp_threshold  = 105.0f
    },
    {
        .target_pot_temp = 95.0f,
        .trigger_type    = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms     = 8u * 60u * 1000u, // 8 phút ủ nhanh
        .temp_threshold  = 0.0f
    }
};

static const cooking_profile_t PROFILE_FAST_RICE = {
    .stage_count    = sizeof(STAGES_FAST_RICE) / sizeof(cooking_stage_t),
    .stages         = STAGES_FAST_RICE,
    .auto_hold_warm = true
};

// 3. Nấu cháo (Đun sôi 100°C -> Sôi lăn tăn nhỏ lửa 88°C chống trào bọt -> Giữ ấm)
static const cooking_stage_t STAGES_PORRIDGE[] = {
    {
        .target_pot_temp = 100.0f,
        .trigger_type    = TRIGGER_POT_TEMP_REACHED,
        .ceiling_duration_ms     = 15u * 60u * 1000u, // timeout nếu quá 15 phút chưa sôi
        .temp_threshold  = 98.0f // gần đạt điểm sôi
    },
    {
        .target_pot_temp = 88.0f,
        .trigger_type    = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms     = 45u * 60u * 1000u, // 45 phút ninh cháo liu riu không trào
        .temp_threshold  = 0.0f
    }
};

static const cooking_profile_t PROFILE_PORRIDGE = {
    .stage_count    = sizeof(STAGES_PORRIDGE) / sizeof(cooking_stage_t),
    .stages         = STAGES_PORRIDGE,
    .auto_hold_warm = true
};

// 4. Ninh / Hầm mềm (Đun tới 95°C -> Hầm liu riu 88°C trong 60 phút -> Giữ ấm)
static const cooking_stage_t STAGES_SOFT_SIMMER[] = {
    {
        .target_pot_temp = 95.0f,
        .trigger_type    = TRIGGER_POT_TEMP_REACHED,
        .ceiling_duration_ms     = 15u * 60u * 1000u,
        .temp_threshold  = 93.0f
    },
    {
        .target_pot_temp = 88.0f,
        .trigger_type    = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms     = 60u * 60u * 1000u, // 60 phút hầm mềm
        .temp_threshold  = 0.0f
    }
};

static const cooking_profile_t PROFILE_SOFT_SIMMER = {
    .stage_count    = sizeof(STAGES_SOFT_SIMMER) / sizeof(cooking_stage_t),
    .stages         = STAGES_SOFT_SIMMER,
    .auto_hold_warm = true
};

// 5. Làm bánh Flan (Hấp cách thủy 83°C ổn định trong 25 phút để mặt bánh mịn, không rỗ -> Tắt)
static const cooking_stage_t STAGES_FLAN_DRE[] = {
    {
        .target_pot_temp = 83.0f,
        .trigger_type    = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms     = 25u * 60u * 1000u, // 25 phút hấp bánh
        .temp_threshold  = 0.0f
    }
};

static const cooking_profile_t PROFILE_FLAN_DRE = {
    .stage_count    = sizeof(STAGES_FLAN_DRE) / sizeof(cooking_stage_t),
    .stages         = STAGES_FLAN_DRE,
    .auto_hold_warm = false // Không giữ ấm bánh flan sau khi hấp
};

// 6. Giữ ấm (Duy trì 65°C liên tục)
static const cooking_stage_t STAGES_HOLD_WARM[] = {
    {
        .target_pot_temp = 65.0f,
        .trigger_type    = TRIGGER_TIME_EXPIRED,
        .ceiling_duration_ms     = UINT32_MAX,
        .temp_threshold  = 0.0f
    }
};

static const cooking_profile_t PROFILE_HOLD_WARM = {
    .stage_count    = sizeof(STAGES_HOLD_WARM) / sizeof(cooking_stage_t),
    .stages         = STAGES_HOLD_WARM,
    .auto_hold_warm = true
};

const cooking_profile_t* cooking_get_profile(const cooking_mode_t mode) {
    switch (mode) {
        case MODE_RICE_NORMAL: return &PROFILE_RICE_NORMAL;
        case MODE_FAST_RICE:   return &PROFILE_FAST_RICE;
        case MODE_PORRIDGE:    return &PROFILE_PORRIDGE;
        case MODE_SOFT_SIMMER: return &PROFILE_SOFT_SIMMER;
        case MODE_FLAN_DRE:    return &PROFILE_FLAN_DRE;
        case MODE_HOLD_WARM:   return &PROFILE_HOLD_WARM;
        default:               return nullptr;
    }
}
