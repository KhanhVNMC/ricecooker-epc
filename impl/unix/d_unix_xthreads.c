#include "include/xthreads.h"
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

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

static void *xthread_entry(void *arg) {
    const unix_xthread_t *thread = arg;
    thread->func(thread->args);
    return NULL;
}

xthread_handle_t *xthreads_new(
    const xthread_func_t func, void *args,
    const thread_priority_t, const char*
) {
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

void xthreads_stop(const xthread_handle_t *handle) {
    if (handle == NULL) return;
    const unix_xthread_t *thread = (unix_xthread_t*)handle;
    pthread_cancel(thread->thread);
    pthread_join(thread->thread, nullptr);
    free((void*)thread);
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

void xthreads_csenter(void) {
    pthread_mutex_lock(&mtx);
}

void xthreads_csexit(void) {
    pthread_mutex_unlock(&mtx);
}

static pthread_mutex_t notify_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t notify_cv = PTHREAD_COND_INITIALIZER;
static bool notified = false;

void xthreads_notify(const xthread_handle_t *handle) {
    (void)handle;
    pthread_mutex_lock(&notify_mtx);
    notified = true;
    pthread_cond_signal(&notify_cv);
    pthread_mutex_unlock(&notify_mtx);
}

void xthreads_wait(const xthread_wait_mode_t) {
    pthread_mutex_lock(&notify_mtx);
    while (!notified) {
        pthread_cond_wait(&notify_cv, &notify_mtx);
    }
    notified = false;
    pthread_mutex_unlock(&notify_mtx);
}