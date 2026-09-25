#include <stddef.h>

#include "FreeRTOS.h"
#include "include/xthreads.h"
#include "include/platform.h"
#include "task.h"

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
        configMINIMAL_STACK_SIZE * 2, args,
        priority, &handle
    );
    return result == pdPASS ? (xthread_handle_t*)handle : NULL;
}

void xthreads_stop(const xthread_handle_t *handle) {
    if (handle != nullptr) {
        vTaskDelete((TaskHandle_t)handle);
    }
}

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

uint32_t xthreads_wait(const xthread_wait_mode_t wait_mode) {
    // if the TCB "counter" == 0 -> consider it blocked
    return ulTaskNotifyTake(wait_mode == XTHREAD_WAIT_ALL ? pdTRUE : pdFALSE, portMAX_DELAY);
}

// bounded wait
uint32_t xthreads_bwait(const xthread_wait_mode_t wait_mode, const uint32_t max_wait_ms) {
    // if the TCB "counter" == 0 -> consider it blocked
    return ulTaskNotifyTake(wait_mode == XTHREAD_WAIT_ALL ? pdTRUE : pdFALSE, pdMS_TO_TICKS(max_wait_ms));
}