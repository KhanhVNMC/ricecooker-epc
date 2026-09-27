#include "include/lrcmdctl.h"
#include "include/ecclayer.h"
#include <string.h>

static volatile bool datagram_ready;
static uint8_t  ib_cmdpkt[ECC_MAX_DATAGRAM_SIZE];
static uint16_t buf_ptr;
static uint16_t cmdpkt_size;

static void clear_current() {
    datagram_ready = false;
    buf_ptr = 0;
    cmdpkt_size = 0;
}

// this will assemble partial reads of USB RX into a full datagram
// because a single datagram can be split into multiple rx_data reads
ecc_rx_result_t ecc_on_rx_data(const uint8_t *data, const uint32_t len) {
    if (len == 0) {
        return ECC_RX_PARTIAL;
    }
    // prevent it from blowing past the buffer
    if (buf_ptr + len > ECC_MAX_DATAGRAM_SIZE) {
        clear_current();
        return ECC_RX_REJECTED;
    }
    for (uint32_t i = 0; i < len; i++) {
        ib_cmdpkt[buf_ptr++] = data[i];
        if (buf_ptr >= 2 && cmdpkt_size == 0) {
            cmdpkt_size = ((uint16_t)ib_cmdpkt[0] << 8) | (uint16_t)ib_cmdpkt[1];
            // it must be at least 3 bytes because of size16 + type8
            if (cmdpkt_size < 3 || cmdpkt_size > ECC_MAX_DATAGRAM_SIZE) {
                clear_current();
                return ECC_RX_REJECTED;
            }
        }
        // if fully read (bptr == size) latch the datagram and stop
        // the transport layer from receiving any more
        if (cmdpkt_size != 0 && buf_ptr == cmdpkt_size) {
            datagram_ready = true;
            return ECC_RX_COMPLETE; // LATCH CLOSE
        }
        // the semantic requires data to not be like this:
        // YES: [int16][int8][payload]              +1000 social credit
        // NO:  [int16][int8][payload][int16][int8] -1000 social credit, execution tmr
    }
    return ECC_RX_PARTIAL;
}

bool lrcmdctl_ob_send(const ob_resp_type_t resp_type, const uint8_t *payload, const uint16_t payload_size) {
    if (!ecc_is_layer_ready()) {
        return false;
    }
    // this will just refuse to send if the payload is > 125 bytes
    if (payload_size > ECC_MAX_DATAGRAM_SIZE - 3u) {
        return false;
    }
    const uint16_t datagram_size = payload_size + 3u;
    uint8_t frame[datagram_size];
    // datagram: [size (2)][resp type (1)][payload]
    frame[0] = (uint8_t)(datagram_size >> 8u);
    frame[1] = (uint8_t)(datagram_size & 0xFFu);
    frame[2] = resp_type;
    if (payload_size != 0) {
        memcpy(&frame[3], payload, payload_size);
    }
    return ecc_tx_data(frame, datagram_size);
}

bool lrcmdctl_ob_ack(const bool positive) {
    return lrcmdctl_ob_send(positive ? RSOB_GENERIC_ACK : RSOB_GENERIC_NAK, nullptr, 0);
}

bool lrcmdctl_ib_has_pending(void) {
    // only true if currently latched
    return datagram_ready && ecc_is_layer_ready();
}

ib_cmd_type_t lrcmdctl_ib_get_cmd_type(void) {
    return ib_cmdpkt[2];
}

uint16_t lrcmdctl_ib_payload_size(void) {
    return cmdpkt_size - 3u;
}

bool lrcmdctl_ib_has_payload(void) {
    return lrcmdctl_ib_payload_size() != 0;
}

const uint8_t* lrcmdctl_ib_get_payload(void) {
    return ib_cmdpkt + 3u;
}

void lrcmdctl_ib_release(void) {
    // unlatch the datagram and signals the transport layer
    // to ready up and receive more shit
    clear_current();
    ecc_next_read_ready();
}