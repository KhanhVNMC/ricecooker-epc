#ifndef XTHREADS_H
#define XTHREADS_H
#include <stdint.h>

#define critical for (bool _once = (xthreads_csenter(), true); _once; xthreads_csexit(), _once = false)
#define critical_ret(x) xthreads_csenter(); auto _ret = x; xthreads_csexit(); return _ret

typedef void (*xthread_func_t)(void *args);
typedef void *xthread_handle_t;

// from outside the thread
xthread_handle_t xthreads_new(xthread_func_t func, void *args);
void xthreads_stop(xthread_handle_t handle);

// from inside the thread
void xthreads_yield(void);
void xthreads_sleep(uint32_t ms);
void xthreads_shutdown(void);
void xthreads_csenter(void);
void xthreads_csexit(void);

#endif //XTHREADS_H
