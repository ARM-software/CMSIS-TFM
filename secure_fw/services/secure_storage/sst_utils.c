/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>
#include "tfm_secure_api.h"
#include "tfm_sst_defs.h"
#include "sst_core.h"
#include "assets/sst_asset_defs.h"
#include "sst_utils.h"

void sst_global_lock(void)
{
    /* FIXME: a system call to be added for acquiring lock */
    return;
}

void sst_global_unlock(void)
{
    /* FIXME: a system call to be added for releasing lock */
    return;
}

enum tfm_sst_err_t sst_utils_memory_bound_check(void *addr,
                                                uint32_t size,
                                                uint32_t app_id,
                                                uint32_t access)
{
    enum tfm_sst_err_t err;

    /* FIXME:
     * The only check that may be required is whether the caller app
     * has permission to read/write to the memory area specified
     * by addr and size.
     */
    (void) app_id;
    err = tfm_core_memory_permission_check(addr, size, access);

    return err;
}

enum tfm_sst_err_t sst_utils_bound_check_and_copy(uint8_t *src,
                                                  uint8_t *dest,
                                                  uint32_t size,
                                                  uint32_t app_id)
{
    enum tfm_sst_err_t bound_check;

    /* src is passed on from untrusted domain, verify boundry */
    bound_check = sst_utils_memory_bound_check(src, size, app_id,
                                               TFM_MEMORY_ACCESS_RO);
    if (bound_check == TFM_SST_ERR_SUCCESS) {
        sst_utils_memcpy(dest, src, size);
    }

    return bound_check;
}

enum tfm_sst_err_t sst_utils_check_contained_in(uint32_t superset_start,
                                                uint32_t superset_size,
                                                uint32_t subset_start,
                                                uint32_t subset_size)
{
    /* Check if the subset is really within superset
     * if the subset's size is large enough, it can cause integer rollover
     * causing appearance of subset being within superset.
     * to avoid this, all of the parameters are promoted to 64 bit
     * so that 64 bit mathematics is performed (on actually 32 bit values)
     * removing possibility of rollovers.
     */
    uint64_t tmp_superset_start = superset_start;
    uint64_t tmp_superset_size = superset_size;
    uint64_t tmp_subset_start = subset_start;
    uint64_t tmp_subset_size = subset_size;
    enum tfm_sst_err_t err = TFM_SST_ERR_SUCCESS;

    if ((tmp_subset_start < tmp_superset_start) ||
        ((tmp_subset_start + tmp_subset_size) >
         (tmp_superset_start + tmp_superset_size))) {
        err = TFM_SST_ERR_PARAM_ERROR;
    }
    return err;
}

/* FIXME: the asset handle is currently composed of
 * uuid and index into object metadata.
 * This allows for state-less operation in the sst
 * implementation as no context needs to be maintained.
 * However, this leaks details of flash layout and where
 * the object is physically stored and can be a potential
 * attack vector.
 * Potential solutions could be-
 * Allocate the handle in RAM and use the pointer
 * as the handle.
 *
 * OR
 *
 * Encrypt the handle so that it is not interpretable
 * by the caller
 */
uint32_t sst_utils_compose_handle(uint16_t asset_uuid, uint16_t meta_idx)
{
    return ((((uint32_t) asset_uuid) << 16) | meta_idx);
}

uint16_t sst_utils_extract_uuid_from_handle(uint32_t asset_handle)
{
    return ((asset_handle >> 16) & 0xFFFF);
}

uint16_t sst_utils_extract_index_from_handle(uint32_t asset_handle)
{
    return (asset_handle & 0xFFFF);
}

uint32_t sst_utils_validate_secure_caller(void)
{
    return tfm_core_validate_secure_caller();
}


/**
 * \brief Validates asset's ID
 *
 * \param[in] unique_id  Asset's ID
 *
 * \return Returns 1 if the asset ID is valid, otherwise 0.
 */
enum tfm_sst_err_t sst_utils_validate_uuid(uint16_t unique_id)
{
    if (unique_id == SST_INVALID_UUID) {
        return TFM_SST_ERR_ASSET_NOT_FOUND;
    }

    return TFM_SST_ERR_SUCCESS;
}

/* FIXME: following functions are not optimized and will eventually to be
 *        replaced by system provided APIs.
 */
void sst_utils_memcpy(void *dest, const void *src, uint32_t size)
{
    uint32_t i;
    uint8_t *p_dst = (uint8_t *)dest;
    const uint8_t *p_src = (const uint8_t *)src;

    for (i = size; i > 0; i--) {
        *p_dst = *p_src;
        p_src++;
        p_dst++;
    }
}

void sst_utils_memset(void *dest, const uint8_t pattern, uint32_t size)
{
    uint32_t i;
    uint8_t *p_dst = (uint8_t *)dest;

    for (i = size; i > 0; i--) {
        *p_dst = pattern;
        p_dst++;
    }
}
