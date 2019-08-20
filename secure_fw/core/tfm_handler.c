/*
 * Copyright (c) 2017-2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include <string.h>

#include "secure_utilities.h"
#include "tfm_svc.h"
#include "tfm_secure_api.h"
#include "region_defs.h"
#include "spm_partition_defs.h"
#include "tfm_api.h"
#include "tfm_internal.h"
#include "tfm_memory_utils.h"
#include "tfm_arch.h"
#include "tfm_peripherals_def.h"
#include "tfm_irq_list.h"
#ifdef TFM_PSA_API
#include <stdbool.h>
#include "tfm_svcalls.h"
#endif

/* This SVC handler is called when a secure partition requests access to a
 * buffer area
 */
extern void tfm_core_set_buffer_area_handler(const uint32_t args[]);
#ifdef TFM_PSA_API
extern void tfm_psa_ipc_request_handler(const uint32_t svc_args[]);
#endif


/* Include the definitions of the privileged IRQ handlers in case of library
 * model
 */
#ifndef TFM_PSA_API
#include "tfm_secure_irq_handlers.inc"
#endif

uint32_t SVCHandler_main(uint32_t *svc_args, uint32_t lr, uint32_t *msp)
{
    uint8_t svc_number;
    /*
     * Stack contains:
     * r0, r1, r2, r3, r12, r14 (lr), the return address and xPSR
     * First argument (r0) is svc_args[0]
     */
    if (is_return_secure_stack(lr)) {
        /* SV called directly from secure context. Check instruction for
         * svc_number
         */
        svc_number = ((uint8_t *)svc_args[6])[-2];
    } else {
        /* Secure SV executing with NS return.
         * NS cannot directly trigger S SVC so this should not happen
         * FixMe: check for security implications
         */
        return lr;
    }
    switch (svc_number) {
#ifdef TFM_PSA_API
    case TFM_SVC_IPC_REQUEST:
        tfm_psa_ipc_request_handler(svc_args);
        break;
#else
    case TFM_SVC_SFN_REQUEST:
        lr = tfm_core_partition_request_svc_handler(svc_args, lr);
        break;
    case TFM_SVC_SFN_RETURN:
        lr = tfm_core_partition_return_handler(lr);
        break;
    case TFM_SVC_VALIDATE_SECURE_CALLER:
        tfm_core_validate_secure_caller_handler(svc_args);
        break;
    case TFM_SVC_GET_CALLER_CLIENT_ID:
        tfm_core_get_caller_client_id_handler(svc_args);
        break;
    case TFM_SVC_SPM_REQUEST:
        tfm_core_spm_request_handler((struct tfm_state_context_t *)svc_args);
        break;
    case TFM_SVC_MEMORY_CHECK:
        tfm_core_memory_permission_check_handler(svc_args);
        break;
    case TFM_SVC_SET_SHARE_AREA:
        tfm_core_set_buffer_area_handler(svc_args);
        break;
    case TFM_SVC_DEPRIV_REQ:
        lr = tfm_core_depriv_req_handler(svc_args, lr);
        break;
    case TFM_SVC_DEPRIV_RET:
        lr = tfm_core_depriv_return_handler(msp, lr);
        break;
    case TFM_SVC_PSA_WAIT:
        tfm_core_psa_wait(svc_args);
        break;
    case TFM_SVC_PSA_EOI:
        tfm_core_psa_eoi(svc_args);
        break;
    case TFM_SVC_ENABLE_IRQ:
        tfm_core_enable_irq_handler(svc_args);
        break;
    case TFM_SVC_DISABLE_IRQ:
        tfm_core_disable_irq_handler(svc_args);
        break;
#endif
    case TFM_SVC_PRINT:
        printf("\033[1;34m[Sec Thread] %s\033[0m\r\n", (char *)svc_args[0]);
        break;
    case TFM_SVC_GET_BOOT_DATA:
        tfm_core_get_boot_data_handler(svc_args);
        break;
    default:
#ifdef TFM_PSA_API
        svc_args[0] = SVC_Handler_IPC(svc_number, svc_args, lr);
#endif
        break;
    }

    return lr;
}

void tfm_access_violation_handler(void)
{
    while (1) {
        ;
    }
}
