#ifndef CMDCTL_H
#define CMDCTL_H
#include <stdint.h>

// INBOUND command types
typedef enum : uint8_t {
    // INBOUND command types
    CIB_DEBUG_ECHO = 0x0,
    CIB_DEBUG_SET_DLED = 0x1,
} ib_cmd_type_t;

// OUTBOUND response types
typedef enum : uint8_t {
    // OUTBOUND synchronous response types
    RSOB_GENERIC_ACK = 0x0,
    RSOB_GENERIC_NAK = 0x1, // negative ACK
    RSOB_DEBUG_ECHO = 0x2,

    // OUTBOUND asynchronous resp types (MSB set)
    RAOB_SENSORS_REPORT = 0x0 | (1 << 7)
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