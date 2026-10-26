#include <stddef.h>

#include "FreeRTOS.h"
#include "include/xthreads.h"
#include "include/platform.h"
#include "task.h"
#include "semphr.h"

void platform_start_xthreads_sched(void) {
    vTaskStartScheduler();
    platform_on_error(SCHEDULER_INIT);
}

xthread_handle_t *xthreads_new(
    const xthread_func_t func, void *args,
    const thread_priority_t priority, const char *name
) {
    TaskHandle_t handle = nullptr;
    const BaseType_t result = xTaskCreate( // this creates freertos thread
        func, name,
        configMINIMAL_STACK_SIZE * 3, args,
        priority, &handle
    );
    return result == pdPASS ? (xthread_handle_t*)handle : NULL;
}

void xthreads_stop(const xthread_handle_t *handle) {
    if (handle != nullptr) {
        vTaskDelete((TaskHandle_t)handle);
    }
}

xthread_handle_t* xthreads_current(void) {
    // inside irq handler (ipsr == 0 -> normal code)
    if (__get_IPSR() != 0) {
        return nullptr;
    }
    return xTaskGetCurrentTaskHandle();
}

void xthreads_yield(void) {
    taskYIELD();
}

void xthreads_sleep(const uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void xthreads_shutdown(void) {
    vTaskDelete(nullptr);
}

void xthreads_csenter(void) {
    taskENTER_CRITICAL();
}

void xthreads_csexit(void) {
    taskEXIT_CRITICAL();
}

// this increments the TCB "counter", which allow
// xthreads_wait to unblock
void xthreads_notify(const xthread_handle_t *handle) {
    if (handle != nullptr) {
        // notify from IRQ hits different
        if (xthreads_current() == nullptr) {
            BaseType_t woken_hp_thread = pdFALSE; // woken higher priority thread
            vTaskNotifyGiveFromISR((TaskHandle_t)handle, &woken_hp_thread);
            portYIELD_FROM_ISR(woken_hp_thread);
            return;
        }
        xTaskNotifyGive((TaskHandle_t)handle);
    }
}

// this returns the value before the wait consumes the TCB counter
// wait mode WAIT_ALL set the counter to 0, while WAIT_ONE just subtract 1
static uint32_t xthreads_wait_generic(const xthread_wait_mode_t wait_mode, const uint32_t max_wait_ticks) {
    // if the TCB "counter" == 0 -> consider it blocked
    return ulTaskNotifyTake(wait_mode == XTHREAD_WAIT_ALL ? pdTRUE : pdFALSE, max_wait_ticks);
}

// unbounded wait
void xthreads_wait(const xthread_wait_mode_t wait_mode) {
    xthreads_wait_generic(wait_mode, portMAX_DELAY);
}

// bounded wait
// returns false if timed out
bool xthreads_bwait(const xthread_wait_mode_t wait_mode, const uint32_t max_wait_ms) {
    return xthreads_wait_generic(wait_mode, pdMS_TO_TICKS(max_wait_ms)) != BWAIT_TIMED_OUT;
}

xthread_mutex_t* xthreads_mutex_new(void) {
    return xSemaphoreCreateMutex();
}

bool xthreads_mutex_acquire(const xthread_mutex_t *mutex) {
    if (mutex == nullptr) {
        return false;
    }
    if (__get_IPSR() != 0) {
        return false; // please dont do this
    }
    return xSemaphoreTake((const SemaphoreHandle_t) mutex, portMAX_DELAY) == pdTRUE;
}

void xthreads_mutex_release(const xthread_mutex_t *mutex) {
    if (mutex == nullptr) {
        return;
    }
    if (__get_IPSR() != 0) {
        return; // please dont do this
    }
    xSemaphoreGive(mutex);
}