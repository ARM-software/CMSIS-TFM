/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "tfm_ss_core_test_veneers.h"
#include "tfm_secure_api.h"
#include "tfm_ss_core_test.h"
#include "secure_fw/spm/spm_api.h"

int32_t tfm_core_test_sfn_init_success(void)
{
    TFM_CORE_SERVICE_REQUEST(TFM_SEC_FUNC_CORE_TEST_ID,
                             spm_core_test_sfn_init_success,
                             0, 0, 0, 0);
}

int32_t tfm_core_test_sfn_direct_recursion(int32_t depth)
{
    TFM_CORE_SERVICE_REQUEST(TFM_SEC_FUNC_CORE_TEST_ID,
                             spm_core_test_sfn_direct_recursion,
                             depth, 0, 0, 0);
}

int32_t tfm_core_test_sfn(int32_t a, int32_t b, int32_t c, int32_t d)
{
    TFM_CORE_SERVICE_REQUEST(TFM_SEC_FUNC_CORE_TEST_ID, spm_core_test_sfn,
                             a, b, c, d);
}
