#include <stdio.h>
#include <stdlib.h>
#include "headers/mem.h"

void *xmalloc(size_t size) {
    void *mal = malloc(size);
    printf("MALLOCED (%lu) @ %p\n", size, mal);
    return mal;
}
void xfree(void *ptr) {
    printf("FREED @ %p\n", ptr);
    free(ptr);
}