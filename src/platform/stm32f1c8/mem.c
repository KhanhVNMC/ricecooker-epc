#include <stddef.h>
#include "FreeRTOS.h"
#include "include/mem.h"

// yes, thats it
void* xmalloc(const size_t size) {
    return pvPortMalloc(size);
}

void xfree(void *ptr) {
    vPortFree(ptr);
}
