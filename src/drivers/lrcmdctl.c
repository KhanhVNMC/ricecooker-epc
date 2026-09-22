#include "include/lrcmdctl.h"
#include "include/ecclayer.h"
#include <string.h>

#define MAX_PACKET_SIZE     (128u)

static volatile bool packet_ready;
static uint8_t ib_cmdpkt[MAX_PACKET_SIZE];
static uint16_t buf_ptr;
static uint16_t cmdpkt_size;

static void clear_current() {
    packet_ready = false;
    buf_ptr = 0;
    cmdpkt_size = 0;
}

// this will assemble partial reads of a packet into a full datagram
// because a single datagram can be split into multiple rx_data reads
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
        if (buf_ptr >= 2 && cmdpkt_size == 0) {
            cmdpkt_size = ((uint16_t)ib_cmdpkt[0] << 8) | (uint16_t)ib_cmdpkt[1];
            // it must be at least 3 bytes because of size16 + type8
            if (cmdpkt_size < 3 || cmdpkt_size > MAX_PACKET_SIZE) {
                clear_current();
                return ECC_RX_REJECTED;
            }
        }
        // if fully read (bptr == size) latch the packet and stop
        // the transport layer from receiving any more
        if (cmdpkt_size != 0 && buf_ptr == cmdpkt_size) {
            packet_ready = true;
            return ECC_RX_COMPLETE; // LATCH CLOSE
        }
        // the semantic requires data to not be like this:
        // YES: [int16][int8][payload]              +1000 social credit
        // NO:  [int16][int8][payload][int16][int8] -1000 social credit, execution tmr
    }
    return ECC_RX_PARTIAL;
}

bool lrcmdctl_ob_send(const cmd_type_t cmd_type, const uint8_t *payload, const uint16_t payload_size) {
    if (!ecc_is_layer_ready()) {
        return false;
    }
    // this will just refuse to send if the payload is > 125B
    if (payload_size > MAX_PACKET_SIZE - 3u) {
        return false;
    }
    const uint16_t datagram_size = payload_size + 3u;
    uint8_t frame[datagram_size];
    // datagram: [size (2)][type (1)][payload]
    frame[0] = (uint8_t)(datagram_size >> 8u);
    frame[1] = (uint8_t)(datagram_size & 0xFFu);
    frame[2] = cmd_type;
    if (payload_size != 0) {
        memcpy(&frame[3], payload, payload_size);
    }
    return ecc_tx_data(frame, datagram_size);
}

bool lrcmdctl_ib_has_pending(void) {
    // only true if currently latched
    return packet_ready && ecc_is_layer_ready();
}

cmd_type_t lrcmdctl_ib_get_cmd_type(void) {
    return ib_cmdpkt[2];
}

uint16_t lrcmdctl_ib_payload_size(void) {
    return cmdpkt_size - 3u;
}

const uint8_t* lrcmdctl_ib_get_payload(void) {
    return ib_cmdpkt + 3u;
}

void lrcmdctl_ib_release(void) {
    // unlatch the packet and signals the transport layer
    // to ready up and receivesd more shit
    clear_current();
    ecc_next_read_ready();
}