/*
 * Copyright (c) 2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef BL2_CONFIG_H
#define BL2_CONFIG_H

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <h>MCUBoot Configuration

//   <o>Upgrade Strategy
//     <1=> Overwrite Only  <2=> Swap  <3=> No Swap <4=> RAM Loading
#define MCUBOOT_UPGRADE_STRATEGY    1

//   <o>Signature Type
//     <1=> RSA-3072 <2=> RSA-2048
#define MCUBOOT_SIGNATURE_TYPE      1

//   <o>Number of Images
//     <1=> 1 <2=> 2
//   <i> 1: Single Image, secure and non-secure images are signed and updated together.
//   <i> 2: Mulitple Image, secure and non-secure images are signed and updatable independently.
#define MCUBOOT_IMAGE_NUMBER        2

//   <c>Hardware Key
#define MCUBOOT_HW_KEY
//   </c>

// <o>Logging Level
//   <0=> Off  <1=> Error  <2=> Warning  <3=> Info  <4=> Debug
#define MCUBOOT_LOG_LEVEL           3

// </h>

//------------- <<< end of configuration section >>> ---------------------------

#if    (MCUBOOT_UPGRADE_STRATEGY == 1)
#define MCUBOOT_OVERWRITE_ONLY
#elif  (MCUBOOT_UPGRADE_STRATEGY == 2)
#elif  (MCUBOOT_UPGRADE_STRATEGY == 3)
#define MCUBOOT_NO_SWAP
#elif  (MCUBOOT_UPGRADE_STRATEGY == 4)
#define MCUBOOT_RAM_LOADING
#else
#error "MCUBoot Configuration: Invalid Upgrade Strategy!"
#endif

#if    (MCUBOOT_IMAGE_NUMBER != 1) && \
      ((MCUBOOT_UPGRADE_STRATEGY == 3) || (MCUBOOT_UPGRADE_STRATEGY == 4))
#error "MCUBoot Configuration: No Swap and RAM Loading Upgrade Strategy supports only single image!"
#endif

#if    (MCUBOOT_SIGNATURE_TYPE == 1)
#define MCUBOOT_SIGN_RSA
#define MCUBOOT_SIGN_RSA_LEN 3072
#elif  (MCUBOOT_SIGNATURE_TYPE == 2)
#define MCUBOOT_SIGN_RSA
#define MCUBOOT_SIGN_RSA_LEN 2048
#else
#error "MCUBoot Configuration: Invalid Signature Type!"
#endif

#if   ((MCUBOOT_IMAGE_NUMBER != 1) && (MCUBOOT_IMAGE_NUMBER != 2))
#error "MCUBoot Configuration: Invalid number of Images!"
#endif

#define MCUBOOT_VALIDATE_PRIMARY_SLOT
#define MCUBOOT_USE_FLASH_AREA_GET_SECTORS

#endif /* BL2_CONFIG_H */
