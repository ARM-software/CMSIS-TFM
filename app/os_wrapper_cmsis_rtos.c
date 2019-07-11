/*
 * Copyright (c) 2017-2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test/suites/sst/non_secure/os_wrapper.h"

#include <string.h>
#include "cmsis_os2.h"

/* This is an example OS abstraction layer for CMSIS-RTOS for non-secure test
 * environment
 */

uint32_t os_wrapper_thread_new(const char *name, int32_t stack_size,
                               os_wrapper_thread_func func, void *arg,
                               uint32_t priority)
{
    osThreadAttr_t task_attribs = {.tz_module = 1};
    osThreadId_t thread_id;

    /* By default, the thread starts as osThreadDetached */
    if (stack_size != OS_WRAPPER_DEFAULT_STACK_SIZE) {
        task_attribs.stack_size = stack_size;
    }
    task_attribs.name = name;
    task_attribs.priority = (osPriority_t) priority;

    thread_id = osThreadNew(func, arg, &task_attribs);
    if (thread_id == NULL) {
        return OS_WRAPPER_ERROR;
    }

    return (uint32_t)thread_id;
}


uint32_t os_wrapper_semaphore_create(uint32_t max_count, uint32_t initial_count,
                                     const char *name)
{
    osSemaphoreAttr_t sema_attrib = {0};
    osSemaphoreId_t semaphore;

    sema_attrib.name = name;

    semaphore = osSemaphoreNew(max_count, initial_count, &sema_attrib);
    if (semaphore == NULL) {
        return OS_WRAPPER_ERROR;
    }

    return (uint32_t)semaphore;
}

uint32_t os_wrapper_semaphore_acquire(uint32_t semaphore_id, uint32_t timeout)
{
    osStatus_t status;

    status = osSemaphoreAcquire((osSemaphoreId_t)semaphore_id,
                                (timeout == OS_WRAPPER_WAIT_FOREVER) ?
                                osWaitForever : timeout);
    if (status != osOK) {
        return OS_WRAPPER_ERROR;
    }

    return 0;
}

uint32_t os_wrapper_semaphore_release(uint32_t sema)
{
    osStatus_t status;

    status = osSemaphoreRelease((osSemaphoreId_t)sema);
    if (status != osOK) {
        return OS_WRAPPER_ERROR;
    }

    return 0;
}

uint32_t os_wrapper_semaphore_delete(uint32_t sema)
{
    osStatus_t status;

    status = osSemaphoreDelete((osSemaphoreId_t)sema);
    if (status != osOK) {
        return OS_WRAPPER_ERROR;
    }

    return 0;
}

uint32_t os_wrapper_thread_get_id(void)
{
    osThreadId_t thread_id;

    thread_id = osThreadGetId();
    if (thread_id == NULL) {
        return OS_WRAPPER_ERROR;
    }

    return (uint32_t)thread_id;
}

uint32_t os_wrapper_thread_get_priority(uint32_t id)
{
    osPriority_t prio;

    prio = osThreadGetPriority((osThreadId_t)id);
    if (prio == osPriorityError) {
        return OS_WRAPPER_ERROR;
    }

    return prio;
}

void os_wrapper_thread_exit(void)
{
    osThreadExit();
}
