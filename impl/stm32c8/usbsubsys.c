#include "usbd_cdc.h"

// DUMMY static malloc and free, it just store in static memory
// this is NOT a real heap allocator, its just cdc
static USBD_CDC_HandleTypeDef cdc_state;
void* usbd_static_mem_dmalloc(const uint32_t size) {
    if (size > sizeof(cdc_state)) {
        return nullptr;
    }
    return &cdc_state;
}
void usbd_static_mem_dfree(void*) {}
