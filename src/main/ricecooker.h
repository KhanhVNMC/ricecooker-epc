#ifndef RICECOOKER_RICECOOKER_H
#define RICECOOKER_RICECOOKER_H
#include <stdint.h>

typedef enum : uint8_t {
    // universal states
    STATE_IDLE,
    STATE_MODE_SELECT,
    // executing tasks states
    STATE_COOKING,
    STATE_HOLD_WARM,
    STATE_DELAYED_COUNTDOWN,
    // config: manual
    STATE_CF_MANUAL_TEMP,
    STATE_CF_MANUAL_TIME,
    // config: delayed
    STATE_CF_DELAYED_TIME,
    STATE_CF_DELAYED_MODE_SELECT
} app_state_t;

typedef enum {
    MODE_RICE_NORMAL, // nau com bth
    MODE_FAST_RICE, // nau com nhanh
    MODE_PORRIDGE, // nau chao
    MODE_SOFT_SIMMER, // ninh/ham
    MODE_FLAN_DRE, // banh flan(dre) scarlet
    MODE_HOLD_WARM, // nac u noi com
    MODE_MANUAL_USER, // tu chinh
    MODE_DELAYED_EXEC, // delay
} cooking_mode_t;

#endif //RICECOOKER_RICECOOKER_H