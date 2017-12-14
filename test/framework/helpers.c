/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "helpers.h"

#include <stdio.h>

const char *sst_err_to_str(enum tfm_sst_err_t err)
{
    switch (err) {
    case TFM_SST_ERR_SUCCESS:
        return "TFM_SST_ERR_SUCCESS";
    case TFM_SST_ERR_ASSET_NOT_PREPARED:
        return "TFM_SST_ERR_ASSET_NOT_PREPARED";
    case TFM_SST_ERR_ASSET_NOT_FOUND:
        return "TFM_SST_ERR_ASSET_NOT_FOUND";
    case TFM_SST_ERR_PARAM_ERROR:
        return "TFM_SST_ERR_PARAM_ERROR";
    case TFM_SST_ERR_INVALID_HANDLE:
        return "TFM_SST_ERR_INVALID_HANDLE";
    case TFM_SST_ERR_STORAGE_SYSTEM_FULL:
        return "TFM_SST_ERR_STORAGE_SYSTEM_FULL";
    case TFM_SST_ERR_SYSTEM_ERROR:
        return "TFM_SST_ERR_SYSTEM_ERROR";
    case TFM_SST_ERR_FORCE_INT_SIZE:
        return "TFM_SST_ERR_FORCE_INT_SIZE";
    /* default:  The default is not defined intentionally to force the
     *           compiler to check that all the enumeration values are
     *           covered in the switch.
     */
    }
}

const char *asset_perms_to_str(uint8_t permissions)
{
    switch (permissions) {
    case 0:
        return "No permissions";
    case 1:
        return "SECURE_ASSET_REFERENCE";
    case 2:
        return "SECURE_ASSET_WRITE";
    case 3:
        return "SECURE_ASSET_REFERENCE | SECURE_ASSET_WRITE";
    case 4:
        return "SECURE_ASSET_READ";
    case 5:
        return "SECURE_ASSET_REFERENCE | SECURE_ASSET_READ";
    case 6:
        return "SECURE_ASSET_WRITE | SECURE_ASSET_READ";
    case 7:
        return "SECURE_ASSET_REFERENCE | SECURE_ASSET_WRITE | "
               "SECURE_ASSET_READ";
    default:
        return "Unknown permissions";
    }
}

void printf_set_color(enum serial_color_t color_id)
{
    printf("\33[3%dm", color_id);
}
