#ifndef USBHANDLE_H
#define USBHANDLE_H
#include <stdint.h>

typedef enum : uint8_t {
    ECC_RX_PARTIAL,
    ECC_RX_COMPLETE,
    ECC_RX_REJECTED
} ecc_rx_result_t;

bool ecc_is_layer_ready(void);
bool ecc_init_stack(void);
ecc_rx_result_t ecc_on_rx_data(const uint8_t *data, uint32_t len);
bool ecc_tx_data(const void *data, uint16_t length);

void ecc_next_read_ready(void);

#endif //USBHANDLE_H