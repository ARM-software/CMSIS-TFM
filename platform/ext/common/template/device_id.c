/*
 * Copyright (c) 2018-2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stddef.h>
#include "platform/include/tfm_plat_device_id.h"
/*
 * NOTE: Functions in this file must be ported per target platform.
 */

static const uint8_t implementation_id[] = {
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD,
};

static const uint8_t example_ean_13[] = "060456527282910010";
/**
 * \brief Copy the device specific ID to the destination buffer
 *
 * \param[out]  p_dst  Pointer to buffer where to store ID
 * \param[in]   p_src  Pointer to the ID
 * \param[in]   size   Length of the ID
 */
static inline void copy_id(uint8_t *p_dst, const uint8_t *p_src, size_t size)
{
    uint32_t i;

    for (i = size; i > 0; i--) {
        *p_dst = *p_src;
        p_src++;
        p_dst++;
    }
}

enum tfm_plat_err_t tfm_plat_get_implementation_id(uint32_t *size,
                                                   uint8_t  *buf)
{
    const uint8_t *p_impl_id = implementation_id;
    uint32_t impl_id_size = sizeof(implementation_id);

    if (*size < impl_id_size) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    copy_id(buf, p_impl_id, impl_id_size);
    *size = impl_id_size;

    return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_get_hw_version(uint32_t *size, uint8_t *buf)
{
    const uint8_t *p_hw_version = example_ean_13;
    uint32_t hw_version_size = sizeof(example_ean_13) - 1;

    if (*size < hw_version_size) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    copy_id(buf, p_hw_version, hw_version_size);
    *size = hw_version_size;

    return TFM_PLAT_ERR_SUCCESS;
}