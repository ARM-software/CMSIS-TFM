/*
 * Copyright (c) 2017-2018 Arm Limited. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __FLASH_LAYOUT_H__
#define __FLASH_LAYOUT_H__

/* Flash layout on MPS2 AN519 with BL2:
 *
 * 0x0000_0000 BL2 - MCUBoot(0.5 MB)
 * 0x0008_0000 Flash_area_image_0(1 MB):
 *    0x0008_0000 Secure     image primary
 *    0x0010_0000 Non-secure image primary
 * 0x0018_0000 Flash_area_image_1(1 MB):
 *    0x0018_0000 Secure     image secondary
 *    0x0020_0000 Non-secure image secondary
 * 0x0028_0000 Scratch area(1 MB)
 * 0x0038_0000 Secure Storage Area (0.02 MB)
 * 0x0038_5000 Unused(0.482 MB)
 *
 * Flash layout on MPS2 AN519, if BL2 not defined:
 * 0x0000_0000 Secure     image
 * 0x0010_0000 Non-secure image
 */

/* This header file is included from linker scatter file as well, where only a
 * limited C constructs are allowed. Therefore it is not possible to include
 * here the platform_retarget.h to access flash related defines. To resolve this
 * some of the values are redefined here with different names, these are marked
 * with comment.
 */

/* The size of a partition. This should be large enough to contain a S or NS
 * sw binary. Each FLASH_AREA_IMAGE contains two partitions. See Flash layout
 * above.
 */
#define FLASH_PARTITION_SIZE            (0x80000)    /* 512 kB */

/* Sector size of the flash hardware; same as FLASH0_SECTOR_SIZE */
#define FLASH_AREA_IMAGE_SECTOR_SIZE    (0x1000)     /* 4 kB */
/* Same as FLASH0_SIZE */
#define FLASH_TOTAL_SIZE                (0x00400000) /* 4 MB */

/* Flash layout info for BL2 bootloader */
#define FLASH_BASE_ADDRESS              (0x10000000) /* same as FLASH0_BASE_S */

/* Offset and size definitions of the flash partitions that are handled by the
 * bootloader. The image swapping is done between IMAGE_0 and IMAGE_1, SCRATCH
 * is used as a temporary storage during image swapping.
 */
#define FLASH_AREA_BL2_OFFSET           (0x0)
#define FLASH_AREA_BL2_SIZE             (FLASH_PARTITION_SIZE)

#define FLASH_AREA_IMAGE_0_OFFSET       (0x080000)
#define FLASH_AREA_IMAGE_0_SIZE         (2 * FLASH_PARTITION_SIZE)

#define FLASH_AREA_IMAGE_1_OFFSET       (0x180000)
#define FLASH_AREA_IMAGE_1_SIZE         (2 * FLASH_PARTITION_SIZE)

#define FLASH_AREA_IMAGE_SCRATCH_OFFSET (0x280000)
#define FLASH_AREA_IMAGE_SCRATCH_SIZE   (2 * FLASH_PARTITION_SIZE)

/* Maximum number of status entries supported by the bootloader. */
#define BOOT_STATUS_MAX_ENTRIES         ((2 * FLASH_PARTITION_SIZE) / \
                                         FLASH_AREA_IMAGE_SCRATCH_SIZE)

/** Maximum number of image sectors supported by the bootloader. */
#define BOOT_MAX_IMG_SECTORS            ((2 * FLASH_PARTITION_SIZE) / \
                                         FLASH_AREA_IMAGE_SECTOR_SIZE)

#define FLASH_SST_AREA_OFFSET           (0x380000)
#define FLASH_SST_AREA_SIZE             (0x5000)   /* 20 KB */

/* Offset and size definition in flash area, used by assemble.py */
#define SECURE_IMAGE_OFFSET             0x0
#define SECURE_IMAGE_MAX_SIZE           0x80000

#define NON_SECURE_IMAGE_OFFSET         0x80000
#define NON_SECURE_IMAGE_MAX_SIZE       0x80000

/* Flash device name used by BL2 and SST
 * Name is defined in flash driver file: Driver_Flash.c
 */
#define FLASH_DEV_NAME Driver_FLASH0

/* Secure Storage (SST) Service definitions */
/* In this target the CMSIS driver requires only the offset from the base
 * address instead of the full memory address.
 */
#define SST_FLASH_AREA_ADDR  FLASH_SST_AREA_OFFSET
#define SST_SECTOR_SIZE      FLASH_AREA_IMAGE_SECTOR_SIZE
/* The sectors must be in consecutive memory location */
#define SST_NBR_OF_SECTORS  (FLASH_SST_AREA_SIZE / SST_SECTOR_SIZE)
/* Specifies the smallest flash programmable unit in bytes */
#define SST_FLASH_PROGRAM_UNIT  0x1

#endif /* __FLASH_LAYOUT_H__ */
