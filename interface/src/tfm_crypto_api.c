/*
 * Copyright (c) 2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifdef TFM_PARTITION_CRYPTO
#ifdef RTE_TFM_API_SFN
#include "tfm_crypto_func_api.c"
#endif
#ifdef RTE_TFM_API_IPC
#include "tfm_crypto_ipc_api.c"
#endif
#endif
