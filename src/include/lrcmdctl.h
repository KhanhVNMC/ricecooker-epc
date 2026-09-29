#ifndef CMDCTL_H
#define CMDCTL_H
#include <stdint.h>

// INBOUND command types
typedef enum : uint8_t {
    CIB_DEBUG_ECHO = 0x0,
    CIB_DEBUG_SET_DLED = 0x1,

    CIB_CTL_RESET = 0x2, // equiv to pressing CANCEL
    CIB_CTL_SET_BTNS = 0x3, // "remotely" controlling the btns
    CIB_CTL_COOK_PRESET = 0x4, // jump straight to preset cookin
    CIB_CTL_COOK_MANUAL = 0x5, // setup manual and cook
    CIB_CTL_COOK_DELAYED = 0x6, // setup delayed and shit (can tolerate manual)
} ib_cmd_type_t;

// OUTBOUND response types
typedef enum : uint8_t {
    // OUTBOUND synchronous response types
    RSOB_GENERIC_ACK = 0x0,
    RSOB_GENERIC_NAK = 0x1, // negative ACK
    RSOB_DEBUG_ECHO = 0x2,

    // OUTBOUND asynchronous resp types (MSB set)
    RAOB_FAULTS_REPORT = 0x0 | (1 << 7),
    RAOB_SENSORS_REPORT = 0x1 | (1 << 7),
    RAOB_APP_STATE_REPORT = 0x2 | (1 << 7)
} ob_resp_type_t;

// INBOUND handling
bool lrcmdctl_ib_has_pending(void);
ib_cmd_type_t lrcmdctl_ib_get_cmd_type(void);
bool lrcmdctl_ib_has_payload(void);
uint16_t lrcmdctl_ib_payload_size(void);
const uint8_t* lrcmdctl_ib_get_payload(void);
void lrcmdctl_ib_release(void); // allow new packets to come

// OUTBOUND packet send
bool lrcmdctl_ob_send(
    ob_resp_type_t resp_type,
    const uint8_t *payload,
    uint16_t payload_size
);
bool lrcmdctl_ob_ack(bool positive);

#endif //CMDCTL_H