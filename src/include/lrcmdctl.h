#ifndef CMDCTL_H
#define CMDCTL_H
#include <stdint.h>

typedef enum : uint8_t {
    CMD_ECHO = 0
} cmd_type_t;

// INBOUND handling
bool lrcmdctl_ib_has_pending(void);
cmd_type_t lrcmdctl_ib_get_cmd_type(void);
uint16_t lrcmdctl_ib_payload_size(void);
const uint8_t* lrcmdctl_ib_get_payload(void);
void lrcmdctl_ib_release(void); // allow new packets to come

// OUTBOUND packet send
bool lrcmdctl_ob_send(
    cmd_type_t cmd_type,
    const uint8_t *payload,
    uint16_t payload_size
);

#endif //CMDCTL_H