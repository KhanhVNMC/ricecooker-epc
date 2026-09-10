#include "headers/xthreads.h"
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

typedef struct {
    pthread_t thread;
    xthread_func_t func;
    void *args;
} unix_xthread_t;


static void *xthread_entry(void *arg) {
    const unix_xthread_t *thread = arg;
    thread->func(thread->args);
    return NULL;
}

xthread_handle_t xthreads_new(xthread_func_t func, void *args) {
    if (func == NULL) {
        return NULL;
    }
    unix_xthread_t *thread = malloc(sizeof(*thread));
    if (thread == NULL) {
        return NULL;
    }
    thread->func = func;
    thread->args = args;
    if (pthread_create(&thread->thread, nullptr, xthread_entry, thread) != 0) {
        free(thread);
        return NULL;
    }
    return thread;
}

void xthreads_stop(xthread_handle_t handle) {
    if (handle == NULL) return;
    unix_xthread_t *thread = handle;
    pthread_cancel(thread->thread);
    pthread_join(thread->thread, nullptr);
    free(thread);
}

void xthreads_yield(void) {
    sched_yield();
}

void xthreads_sleep(uint32_t ms) {
    struct timespec remaining = {
        .tv_sec  = ms / 1000u,
        .tv_nsec = (long)(ms % 1000u) * 1000000L
    };
    while (nanosleep(&remaining, &remaining) == -1) {
        if (errno != EINTR) break;
    }
}

void xthreads_shutdown(void) {
    pthread_exit(NULL);
}