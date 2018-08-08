/*
 * Copyright (c) 2017-2018, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include <string.h>

#include "cmsis.h"
#include "secure_utilities.h"
#include "arm_acle.h"
#include "tfm_svc.h"
#include "tfm_secure_api.h"
#include "region_defs.h"
#include "tfm_api.h"

/* This SVC handler is called if veneer is running in thread mode */
extern void tfm_core_partition_request_svc_handler(
        uint32_t *svc_args, uint32_t *lr_ptr);

/* This SVC handler is called when sfn returns */
extern int32_t tfm_core_partition_return_handler(uint32_t *lr_ptr);

extern int32_t
        tfm_core_validate_secure_caller_handler(const uint32_t svc_args[]);

extern int32_t
        tfm_core_memory_permission_check_handler(const uint32_t svc_args[]);

/* This SVC handler is called when a secure partition requests access to a
 * buffer area
 */
extern int32_t tfm_core_set_buffer_area_handler(const uint32_t args[]);

struct tfm_fault_context_s {
    uint32_t R0;
    uint32_t R1;
    uint32_t R2;
    uint32_t R3;
    uint32_t R12;
    uint32_t LR;
    uint32_t ReturnAddress;
    uint32_t RETPSR;
} tfm_fault_context;

#if defined(__ARM_ARCH_8M_MAIN__)
/**
 * \brief Overwrites default Secure fault handler.
 */
void SecureFault_Handler(void)
{
    /* figure out context from which we landed in fault handler */
    uint32_t lr = __get_LR();
    uint32_t sp;

    if (lr & EXC_RETURN_SECURE_STACK) {
        if (lr & EXC_RETURN_STACK_PROCESS) {
            sp = __get_PSP();
        } else {
            sp = __get_MSP();
        }
    } else {
        if (lr & EXC_RETURN_STACK_PROCESS) {
            sp =  __TZ_get_PSP_NS();
        } else {
            sp = __TZ_get_MSP_NS();
        }
    }

    /* Only save the context if sp is valid */
    if ((sp >=  S_DATA_START &&
         sp <=  S_DATA_LIMIT - sizeof(tfm_fault_context) + 1) ||
        (sp >= NS_DATA_START &&
         sp <= NS_DATA_LIMIT - sizeof(tfm_fault_context) + 1)) {
        memcpy(&tfm_fault_context, (const void *)sp, sizeof(tfm_fault_context));
    }

    LOG_MSG("Oops... Secure fault!!! You're not going anywhere!");
    while (1) {
        ;
    }
}
#elif defined(__ARM_ARCH_8M_BASE__)
/**
 * \brief Overwrites default Hard fault handler.
 *
 * In case of a baseline implementation fault conditions that would generate a
 * SecureFault in a mainline implementation instead generate a Secure HardFault.
 */
void HardFault_Handler(void)
{
    /* In a baseline implementation there is no way, to find out whether this is
     * a hard fault triggered directly, or another fault that has been
     * escalated.
     */
    while (1) {
        ;
    }
}
#else
#error "Unsupported ARM Architecture."
#endif

#if defined(__ARM_ARCH_8M_MAIN__)
__attribute__((naked)) void SVC_Handler(void)
{
    __ASM(
    "TST     lr, #4\n"  /* Check store SP in thread mode to r0 */
    "ITE     EQ\n"
    "MRSEQ   r0, MSP\n"
    "MRSNE   r0, PSP\n"
    "PUSH    {r0, lr}\n"
    "MOV     r1, sp\n"
    "ADD     r1, r1, #4\n"
    "BL      SVCHandler_main\n"
    "POP     {r1, pc}\n");
}
#elif defined(__ARM_ARCH_8M_BASE__)
__attribute__((naked)) void SVC_Handler(void)
{
    __ASM(
    ".syntax unified\n"
    "MOVS    r0, #4\n"  /* Check store SP in thread mode to r0 */
    "MOV     r1, lr\n"
    "TST     r0, r1\n"
    "BEQ     handler\n"
    "MRS     r0, PSP\n"  /* Coming from thread mode */
    "B sp_stored\n"
    "handler:\n"
    "MRS     r0, MSP\n"  /* Coming from handler mode */
    "sp_stored:\n"
    "PUSH    {r0, lr}\n"
    "MOV     r1, sp\n"
    "ADDS    r1, r1, #4\n"
    "BL      SVCHandler_main\n"
    "POP     {r1, pc}\n");
}
#else
#error "Unsupported ARM Architecture."
#endif


int32_t SVCHandler_main(uint32_t *svc_args, uint32_t *lr_ptr)
{
    uint8_t svc_number;
    /*
     * Stack contains:
     * r0, r1, r2, r3, r12, r14 (lr), the return address and xPSR
     * First argument (r0) is svc_args[0]
     */
    if (*lr_ptr & EXC_RETURN_SECURE_STACK) {
        /* SV called directly from secure context. Check instruction for
         * svc_number
         */
        svc_number = ((uint8_t *)svc_args[6])[-2];
    } else {
        /* Secure SV executing with NS return.
         * NS cannot directly trigger S SVC so this should not happen
         * FixMe: check for security implications
         */
        return TFM_ERROR_GENERIC;
    }
    switch (svc_number) {
    case TFM_SVC_SFN_REQUEST:
        tfm_core_partition_request_svc_handler(svc_args, lr_ptr);
        return TFM_SUCCESS;
    case TFM_SVC_SFN_RETURN:
        return tfm_core_partition_return_handler(lr_ptr);
    case TFM_SVC_VALIDATE_SECURE_CALLER:
        tfm_core_validate_secure_caller_handler(svc_args);
        return TFM_SUCCESS;
    case TFM_SVC_MEMORY_CHECK:
        tfm_core_memory_permission_check_handler(svc_args);
        return TFM_SUCCESS;
    case TFM_SVC_SET_SHARE_AREA:
        tfm_core_set_buffer_area_handler(svc_args);
        return TFM_SUCCESS;
    case TFM_SVC_PRINT:
        printf("\e[1;34m[Sec Thread] %s\e[0m\r\n", (char *)svc_args[0]);
        return TFM_SUCCESS;
    default:
        LOG_MSG("Unknown SVC number requested!");
        break;
    }

    return TFM_ERROR_GENERIC;
}

void tfm_access_violation_handler(void)
{
    while (1) {
        ;
    }
}
