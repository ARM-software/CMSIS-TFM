/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_SVC_H__
#define __TFM_SVC_H__

#include "cmsis.h"

typedef enum {
    TFM_SVC_SERVICE_REQUEST = 0,
    TFM_SVC_SERVICE_RETURN,
    TFM_SVC_VALIDATE_SECURE_CALLER,
    TFM_SVC_MEMORY_CHECK,
    TFM_SVC_SET_SHARE_AREA,
    TFM_SVC_PRINT,
} tfm_svc_number_t;

#define SVC(code) __ASM("svc %0" : : "I" (code))

#endif /* __TFM_SVC_H__ */
