#include <stddef.h>

#include "FreeRTOS.h"
#include "include/xthreads.h"
#include "include/platform.h"
#include "task.h"

void platform_start_xthreads_sched(void) {
    vTaskStartScheduler();
    platform_on_error(SCHEDULER_INIT);
}

xthread_handle_t *xthreads_new(const xthread_func_t func, const char *name, void *args) {
    TaskHandle_t handle = nullptr;
    const BaseType_t result = xTaskCreate( // this creates freertos thread
        func, name,
        configMINIMAL_STACK_SIZE * 2, args,
        tskIDLE_PRIORITY + 1U, &handle
    );
    return result == pdPASS ? (xthread_handle_t*)handle : NULL;
}

void xthreads_stop(const xthread_handle_t *handle) {
    vTaskDelete((TaskHandle_t)handle);
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

