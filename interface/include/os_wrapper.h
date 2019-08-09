/*
 * Copyright (c) 2017-2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __OS_WRAPPER_H__
#define __OS_WRAPPER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define OS_WRAPPER_ERROR              (0xFFFFFFFFU)
#define OS_WRAPPER_WAIT_FOREVER       (0xFFFFFFFFU)
#define OS_WRAPPER_DEFAULT_STACK_SIZE (-1)

/* prototype for the thread entry function */
typedef void (*os_wrapper_thread_func) (void *argument);

/**
 * \brief Creates a new semaphore
 *
 * \param[in] max_count       Highest count of the semaphore
 * \param[in] initial_count   Starting count of the semaphore
 * \param[in] name            Name of the semaphore
 *
 * \return Returns ID of the semaphore created, or OS_WRAPPER_ERROR in case of
 *         error
 */
uint32_t os_wrapper_semaphore_create(uint32_t max_count, uint32_t initial_count,
                                     const char *name);

/**
 * \brief Acquires the semaphore
 *
 * \param[in] semaphore_id Semaphore ID
 * \param[in] timeout      Timeout value
 *
 * \return 0 in case of successful acquision, or OS_WRAPPER_ERROR in case of
 *         error
 */
uint32_t os_wrapper_semaphore_acquire(uint32_t semaphore_id, uint32_t timeout);

/**
 * \brief Releases the semaphore
 *
 * \param[in] semaphore_id Semaphore ID
 *
 * \return 0 in case of successful release, or OS_WRAPPER_ERROR in case of error
 */
uint32_t os_wrapper_semaphore_release(uint32_t semaphore_id);

/**
 * \brief Deletes the semaphore
 *
 * \param[in] semaphore_id Semaphore ID
 *
 * \return 0 in case of successful release, or OS_WRAPPER_ERROR in case of error
 */
uint32_t os_wrapper_semaphore_delete(uint32_t semaphore_id);

/**
 * \brief Creates a new thread
 *
 * \param[in] name        Name of the thread
 * \param[in] stack_size  Size of stack to be allocated for this thread. It can
 *                        be OS_WRAPPER_DEFAULT_STACK_SIZE to use the default
 *                        value provided by the underlying RTOS
 * \param[in] func        Pointer to the function invoked by thread
 * \param[in] arg         Argument to pass to the function invoked by thread
 * \param[in] priority    Initial thread priority
 *
 * \return Returns thread ID, or OS_WRAPPER_ERROR in case of error
 */
uint32_t os_wrapper_thread_new(const char *name, int32_t stack_size,
                               os_wrapper_thread_func func, void *arg,
                               uint32_t priority);
/**
 * \brief Gets current thread ID
 *
 * \return Returns thread ID, or OS_WRAPPER_ERROR in case of error
 */
uint32_t os_wrapper_thread_get_id(void);

/**
 * \brief Gets thread priority
 *
 * \param[in] id  Thread ID
 *
 * \return Returns thread priority value, or OS_WRAPPER_ERROR in case of error
 */
uint32_t os_wrapper_thread_get_priority(uint32_t id);

/**
 * \brief Exits the calling thread
 *
 * \return void
 */
void os_wrapper_thread_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* __OS_WRAPPER_H__ */
