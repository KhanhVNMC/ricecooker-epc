#ifndef XTHREADS_H
#define XTHREADS_H
#include <stdint.h>

#define BWAIT_TIMED_OUT (0u)

#define critical for (bool _once = (xthreads_csenter(), true); _once; xthreads_csexit(), _once = false)
#define critical_ret(x) xthreads_csenter(); auto _ret = x; xthreads_csexit(); return _ret

typedef void (*xthread_func_t)(void *args);
typedef void xthread_handle_t;
typedef void xthread_mutex_t;

// https://en.wikipedia.org/wiki/Ranks_and_insignia_of_the_Waffen-SS
typedef enum : uint8_t {
    SS_STURMSCHARFUHRER = 0,
    SS_STANDARTENFUHRER = 1,
    SS_OBERFUHRER = 2,
    SS_GRUPPENFUHRER = 3,
    OBERSTER_FUHRER_DER_SS = 4,
} thread_priority_t;

typedef enum : uint8_t {
    XTHREAD_WAIT_ONE,
    XTHREAD_WAIT_ALL,
} xthread_wait_mode_t;

// from outside the thread
xthread_handle_t* xthreads_new(
    xthread_func_t func, void *args,
    thread_priority_t priority, const char *name
);
void xthreads_stop(const xthread_handle_t *handle);
void xthreads_notify(const xthread_handle_t *handle);

// from inside the thread
xthread_handle_t* xthreads_current(void);
void xthreads_yield(void);
void xthreads_sleep(uint32_t ms);
void xthreads_shutdown(void);
void xthreads_csenter(void);
void xthreads_csexit(void);
void xthreads_wait(xthread_wait_mode_t wait_mode);
bool xthreads_bwait(xthread_wait_mode_t wait_mode, uint32_t max_wait_ms);
xthread_mutex_t* xthreads_mutex_new(void);
bool xthreads_mutex_acquire(const xthread_mutex_t *mutex);
void xthreads_mutex_release(const xthread_mutex_t *mutex);

#endif //XTHREADS_H
