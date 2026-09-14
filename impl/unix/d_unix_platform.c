#include "include/platform.h"

#define FAKE_UNIX_SHIT

int main() {
    platform_main();
    for (;;) {}
}

void platform_init_primitives(void) {}
void platform_start_xthreads_sched(void) {}