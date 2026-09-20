#include "include/lrcmdctl.h"
#include "include/ecclayer.h"
#include <string.h>

#define MAX_PACKET_SIZE     (128u)

static volatile bool packet_ready;
static uint8_t ib_cmdpkt[MAX_PACKET_SIZE];
static uint16_t buf_ptr;
static uint16_t size;

static void clear_current() {
    packet_ready = false;
    buf_ptr = 0;
    size = 0;
}

ecc_rx_result_t ecc_on_rx_data(const uint8_t *data, const uint32_t len) {
    if (len == 0) {
        return ECC_RX_PARTIAL;
    }
    for (uint32_t i = 0; i < len; i++) {
        if (buf_ptr >= MAX_PACKET_SIZE) {
            clear_current();
            return ECC_RX_REJECTED;
        }
        ib_cmdpkt[buf_ptr++] = data[i];
        if (buf_ptr >= 2 && size == 0) {
            size = ((uint16_t)ib_cmdpkt[0] << 8) | (uint16_t)ib_cmdpkt[1];
            if (size < 3 || size > MAX_PACKET_SIZE) {
                clear_current();
                return ECC_RX_REJECTED;
            }
        }
        if (size != 0 && buf_ptr == size) {
            packet_ready = true;
            return ECC_RX_COMPLETE;
        }
    }
    return ECC_RX_PARTIAL;
}

bool lrcmdctl_ob_send(const uint8_t cmd_type, const uint8_t *payload, const uint16_t payload_size) {
    if (payload_size > MAX_PACKET_SIZE - 3u) {
        return false;
    }
    const uint16_t datagram_size = payload_size + 3u;
    uint8_t frame[datagram_size];
    // datagram: [size (2)][type (1)][payload]
    frame[0] = (uint8_t)(datagram_size >> 8u);
    frame[1] = (uint8_t)(datagram_size & 0xFFu);
    frame[2] = cmd_type;
    memcpy(&frame[3], payload, payload_size);
    return ecc_tx_data(frame, datagram_size);
}

bool cmdctl_ib_has_pending(void) {
    return packet_ready && ecc_is_layer_ready();
}

uint8_t lrcmdctl_ib_get_cmd_type(void) {
    return ib_cmdpkt[2];
}

uint16_t lrcmdctl_ib_payload_size(void) {
    return size - 3u;
}

const uint8_t* lrcmdctl_ib_get_payload(void) {
    return ib_cmdpkt + 3u;
}

void lrcmdctl_ib_release(void) {
    clear_current();
    ecc_next_read_ready();
}