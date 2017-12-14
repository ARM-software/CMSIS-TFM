/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include <cmsis_compiler.h>

#ifndef __TFM_INTEG_TEST_H__
#define __TFM_INTEG_TEST_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Avoids the semihosting issue */
#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__asm("  .global __ARM_use_no_argv\n");
#endif

/**
 * \brief Simple macro to mark UNUSED variables
 *
 */
#define UNUSED_VARIABLE(X) ((void)(X))

/**
 * \brief Declarations for User defined SVC functions
 *        used in CORE_TEST_INTERACTIVE or
 *        CORE_TEST_POSITIVE
 *
 */
void svc_secure_decrement_ns_lock_1(void);
void svc_secure_decrement_ns_lock_2(void);

#ifdef TEST_FRAMEWORK_NS
/**
 * \brief Main test application for the RTX-TFM core
 *        integration tests
 *
 */
void test_app(void *argument);
#endif /* TEST_FRAMEWORK_NS */

/**
 * \brief Execute the interactive test cases (button push)
 *
 */
void execute_ns_interactive_tests(void);

/**
 * \brief Logging function
 *
 */
__attribute__((always_inline)) __STATIC_INLINE void LOG_MSG(const char *MSG)
{
#ifndef LOG_MSG_HANDLER_MODE_PRINTF_ENABLED
    /* if IPSR is non-zero, exception is active. NOT banked S/NS */
    if (!__get_IPSR()) {
        printf("\t\e[1;32m[Non-Sec] %s\e[0m\r\n", MSG);
    }
#else
    printf("\t\e[1;32m[Non-Sec] %s\e[0m\r\n", MSG);
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* __TFM_INTEG_TEST_H__ */
