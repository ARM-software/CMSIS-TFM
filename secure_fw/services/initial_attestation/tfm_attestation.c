/*
 * Copyright (c) 2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "tfm_api.h"
#include "tfm_secure_api.h"
#include "attestation.h"
#include "psa_initial_attestation_api.h"
#include "bl2/include/tfm_boot_status.h"

enum psa_attest_err_t
attest_check_memory_access(void *addr,
                           uint32_t size,
                           enum attest_memory_access_t access)
{
    enum tfm_status_e tfm_res;
    enum psa_attest_err_t attest_res = PSA_ATTEST_ERR_SUCCESS;

    tfm_res = tfm_core_memory_permission_check(addr, size, access);
    if (tfm_res) {
        attest_res =  PSA_ATTEST_ERR_INVALID_INPUT;
     }

     return attest_res;
}

enum psa_attest_err_t
attest_get_caller_client_id(int32_t *caller_id)
{
    enum tfm_status_e tfm_res;
    enum psa_attest_err_t attest_res = PSA_ATTEST_ERR_SUCCESS;

    tfm_res =  tfm_core_get_caller_client_id(caller_id);
    if (tfm_res) {
        attest_res =  PSA_ATTEST_ERR_CLAIM_UNAVAILABLE;
     }

    return attest_res;
}

enum psa_attest_err_t
attest_get_boot_data(uint8_t major_type,
                     struct tfm_boot_data *boot_data,
                     uint32_t len)
{
    enum psa_attest_err_t attest_res = PSA_ATTEST_ERR_SUCCESS;

#ifndef BL2
    /* Avoid compiler warning due to unused argument */
    (void)len;
    (void)major_type;

    boot_data->header.tlv_magic   = SHARED_DATA_TLV_INFO_MAGIC;
    boot_data->header.tlv_tot_len = SHARED_DATA_HEADER_SIZE;
#else
    enum tfm_status_e tfm_res;

    tfm_res = tfm_core_get_boot_data(major_type, boot_data, len);
    if (tfm_res != TFM_SUCCESS) {
        attest_res =  PSA_ATTEST_ERR_INIT_FAILED;
    }
#endif /* BL2 */

    return attest_res;
}
