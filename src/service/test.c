#include <stdlib.h>

#include "../rcplatform/rchal.h"
#include "../xthreads/xthreads.h"
#include "../rcplatform/platform.h"
#include "../rcplatform/mem.h"

void platform_main(void) {
    int *mall = xmalloc(1024);
    xfree(mall);
}