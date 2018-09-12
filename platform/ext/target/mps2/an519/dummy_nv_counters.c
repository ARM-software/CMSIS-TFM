/*
 * Copyright (c) 2018, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/* NOTE: This API should be implemented by platform vendor. For the
 * security of the secure storage system rollback protection and others, it is
 * CRITICAL to use a internal (in-die) persistent memory for multiple time
 * programabe (MTP) non-volatile counters or use a One-time Programmable (OTP)
 * non-volatile counters solution.
 *
 * AN519 does not have any available MTP or OTP non-volatile counters, so a
 * software dummy implementation has been implemented in this case.
 */

#include "platform/include/tfm_plat_nv_counters.h"

#include <limits.h>
#include "Driver_Flash.h"
#include "flash_layout.h"

/* Compilation time checks to be sure the defines are well defined */
#ifndef TFM_NV_COUNTERS_AREA_ADDR
#error "TFM_NV_COUNTERS_AREA_ADDR must be defined in flash_layout.h"
#endif

#ifndef TFM_NV_COUNTERS_AREA_SIZE
#error "TFM_NV_COUNTERS_AREA_SIZE must be defined in flash_layout.h"
#endif

#ifndef TFM_NV_COUNTERS_SECTOR_ADDR
#error "TFM_NV_COUNTERS_SECTOR_ADDR must be defined in flash_layout.h"
#endif

#ifndef TFM_NV_COUNTERS_SECTOR_SIZE
#error "TFM_NV_COUNTERS_SECTOR_SIZE must be defined in flash_layout.h"
#endif

#ifndef FLASH_DEV_NAME
#error "FLASH_DEV_NAME must be defined in flash_layout.h"
#endif
/* End of compilation time checks to be sure the defines are well defined */

#define SECTOR_OFFSET    0
#define NV_COUNTER_SIZE  sizeof(uint32_t)
#define INIT_VALUE_SIZE  NV_COUNTER_SIZE
#define NV_COUNTERS_AREA_OFFSET (TFM_NV_COUNTERS_AREA_ADDR - \
                                 TFM_NV_COUNTERS_SECTOR_ADDR)

#define NV_COUNTERS_INITIALIZED 0xC0DE0042

/* Import the CMSIS flash device driver */
extern ARM_DRIVER_FLASH FLASH_DEV_NAME;

enum tfm_plat_err_t tfm_plat_init_nv_counter(void)
{
    int32_t  err;
    uint32_t i;
    uint32_t nbr_counters = ((TFM_NV_COUNTERS_AREA_SIZE - INIT_VALUE_SIZE)
                             / NV_COUNTER_SIZE);
    uint32_t *p_nv_counter;
    uint8_t sector_data[TFM_NV_COUNTERS_SECTOR_SIZE] = {0};

    /* Read the whole sector to be able to erase and write later in the flash */
    err = FLASH_DEV_NAME.ReadData(TFM_NV_COUNTERS_SECTOR_ADDR, sector_data,
                                  TFM_NV_COUNTERS_SECTOR_SIZE);
    if (err != ARM_DRIVER_OK) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* Set the pointer to nv counters position */
    p_nv_counter = (uint32_t *)(sector_data + NV_COUNTERS_AREA_OFFSET);

    if (p_nv_counter[nbr_counters] == NV_COUNTERS_INITIALIZED) {
        return TFM_PLAT_ERR_SUCCESS;
    }

    /* Add watermark, at the end of the NV counters area, to indicate that NV
     * counters have been initialized.
     */
    p_nv_counter[nbr_counters] = NV_COUNTERS_INITIALIZED;

    /* Initialize all counters to 0 */
    for (i = 0; i < nbr_counters; i++) {
        p_nv_counter[i] = 0;
    }

    /* Erase sector before write in it */
    err = FLASH_DEV_NAME.EraseSector(TFM_NV_COUNTERS_SECTOR_ADDR);
    if (err != ARM_DRIVER_OK) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* Write in flash the in-memory block content after modification */
    err = FLASH_DEV_NAME.ProgramData(TFM_NV_COUNTERS_SECTOR_ADDR, sector_data,
                                     TFM_NV_COUNTERS_SECTOR_SIZE);
    if (err != ARM_DRIVER_OK) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_read_nv_counter(enum tfm_nv_counter_t counter_id,
                                             uint32_t size, uint8_t *val)
{
    int32_t  err;
    uint32_t flash_addr;

    if (size != NV_COUNTER_SIZE) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    flash_addr = TFM_NV_COUNTERS_AREA_ADDR + (counter_id * NV_COUNTER_SIZE);

    err = FLASH_DEV_NAME.ReadData(flash_addr, val, NV_COUNTER_SIZE);
    if (err != ARM_DRIVER_OK) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_increment_nv_counter(
                                               enum tfm_nv_counter_t counter_id)
{
    int32_t  err;
    uint32_t *p_nv_counter;
    uint8_t  sector_data[TFM_NV_COUNTERS_SECTOR_SIZE];

    /* Read the whole sector to be able to erase and write later in the flash */
    err = FLASH_DEV_NAME.ReadData(TFM_NV_COUNTERS_SECTOR_ADDR, sector_data,
                                  TFM_NV_COUNTERS_SECTOR_SIZE);
    if (err != ARM_DRIVER_OK) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* Set the pointer to nv counter position */
    p_nv_counter = (uint32_t *)(sector_data + NV_COUNTERS_AREA_OFFSET +
                                (counter_id * NV_COUNTER_SIZE));

    if (*p_nv_counter == UINT32_MAX) {
        return TFM_PLAT_ERR_MAX_VALUE;
    }

    /* Next value is the current value + 1 */
    *p_nv_counter = *p_nv_counter + 1;

    /* Erase sector before write in it */
    err = FLASH_DEV_NAME.EraseSector(TFM_NV_COUNTERS_SECTOR_ADDR);
    if (err != ARM_DRIVER_OK) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    /* Write in flash the in-memory block content after modification */
    err = FLASH_DEV_NAME.ProgramData(TFM_NV_COUNTERS_SECTOR_ADDR, sector_data,
                                     TFM_NV_COUNTERS_SECTOR_SIZE);
    if (err != ARM_DRIVER_OK) {
        return TFM_PLAT_ERR_SYSTEM_ERR;
    }

    return TFM_PLAT_ERR_SUCCESS;
}
