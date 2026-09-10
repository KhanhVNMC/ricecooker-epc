#ifndef MEM_H
#define MEM_H
#include <stddef.h>

void *xmalloc(size_t size);
void xfree(void *ptr);

#endif //MEM_H
