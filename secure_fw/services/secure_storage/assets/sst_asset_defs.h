/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __SST_ASSET_DEFS_H__
#define __SST_ASSET_DEFS_H__

#include "tfm_sst_defs.h"
#include "secure_fw/services/secure_storage/sst_asset_management.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SST_ASSET_ID_NO_ASSET 0
#define SST_ASSET_ID_DELETED_ASSET 1
/**********************************/
#define SST_ASSET_ID_AES_KEY_128 3
#define SST_ASSET_ID_AES_KEY_192 4
#define SST_ASSET_ID_AES_KEY_256 5
#define SST_ASSET_ID_RSA_KEY_1024 6
#define SST_ASSET_ID_RSA_KEY_2048 7
#define SST_ASSET_ID_RSA_KEY_4096 8
#define SST_ASSET_ID_X509_CERT_SMALL 9
#define SST_ASSET_ID_X509_CERT_LARGE 10
#define SST_ASSET_ID_SHA224_HASH 11
#define SST_ASSET_ID_SHA384_HASH 12

#define SST_ASSET_MAX_SIZE_AES_KEY_128 16
#define SST_ASSET_MAX_SIZE_AES_KEY_192 24
#define SST_ASSET_MAX_SIZE_AES_KEY_256 32
#define SST_ASSET_MAX_SIZE_RSA_KEY_1024 128
#define SST_ASSET_MAX_SIZE_RSA_KEY_2048 256
#define SST_ASSET_MAX_SIZE_RSA_KEY_4096 512
#define SST_ASSET_MAX_SIZE_X509_CERT_SMALL 512
#define SST_ASSET_MAX_SIZE_X509_CERT_LARGE 2048
#define SST_ASSET_MAX_SIZE_SHA224_HASH 28
#define SST_ASSET_MAX_SIZE_SHA384_HASH 48

#define SST_ASSET_PERMS_COUNT_AES_KEY_128 1
#define SST_ASSET_PERMS_COUNT_AES_KEY_192 1
#define SST_ASSET_PERMS_COUNT_AES_KEY_256 1
#define SST_ASSET_PERMS_COUNT_RSA_KEY_1024 1
#define SST_ASSET_PERMS_COUNT_RSA_KEY_2048 1
#define SST_ASSET_PERMS_COUNT_RSA_KEY_4096 1
#define SST_ASSET_PERMS_COUNT_X509_CERT_SMALL 1
#define SST_ASSET_PERMS_COUNT_X509_CERT_LARGE 3
#define SST_ASSET_PERMS_COUNT_SHA224_HASH 1
#define SST_ASSET_PERMS_COUNT_SHA384_HASH 1

#define SST_APP_ID_0 9
#define SST_APP_ID_1 10
#define SST_APP_ID_2 11
#define SST_APP_ID_3 12

/* Maximum number of assets that can be stored in the cache */
#define SST_NUM_ASSETS 10
/* Largest defined asset size */
#define SST_MAX_ASSET_SIZE 2048

#ifdef __cplusplus
}
#endif

#endif /* __SST_ASSET_DEFS_H__ */
