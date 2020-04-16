/*
 * Copyright (c) 2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef TFM_CONFIG_H
#define TFM_CONFIG_H

#define DOMAIN_NS 1

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <h>TF-M API

//   <c>Secure Storage
#define TFM_PARTITION_SECURE_STORAGE
//   </c>

//   <c>Internal Trusted Storage
#define TFM_PARTITION_INTERNAL_TRUSTED_STORAGE
//   </c>

//   <c>Audit Logging
//#define TFM_PARTITION_AUDIT_LOG
//   </c>

//   <c>Crypto
#define TFM_PARTITION_CRYPTO
//   </c>

//   <c>Platform
//#define TFM_PARTITION_PLATFORM
//   </c>

//   <c>Initial Attestation
#define TFM_PARTITION_INITIAL_ATTESTATION
//   </c>

// </h>

//------------- <<< end of configuration section >>> ---------------------------

#endif /* TFM_CONFIG_H */
