#ifndef PLATFORM_H
#define PLATFORM_H

// bootstrap
void platform_init_primitives(void);
void platform_start_xthreads_sched(void);

// entrypoint
void platform_main(void);

#endif //PLATFORM_H
