/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __SST_FLASH_H__
#define __SST_FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Adjust to match your system's block size */
#define SST_BLOCK_SIZE 4096

/* Adjust to a size that will allow all assets to fit */
#define SST_TOTAL_NUM_OF_BLOCKS 5

/* Default value of flash when erased */
#define SST_FLASH_DEFAULT_VAL 0xFF
/* A single host specific sst_flash_xxx.c should be included during compile */

/* Invalid block index */
#define SST_BLOCK_INVALID_ID 0xFFFFFFFF

#define SST_FLASH_SUCCESS  0
#define SST_FLASH_ERROR    1

/**
 * \brief Reads block data from the position specifed by block ID and offset.
 *
 * \param[in]  block_id  Block ID
 * \param[out] buff      Buffer pointer to store the data read
 * \param[in]  offset    Offset position from the init of the block
 * \param[in]  size      Number of bytes to read
 *
 * \note This function considers all input values are valid. That means,
 *       the range of address, based on blockid + offset + size, are always
 *       valid in the memory.
 *
 * \return Returns 0 if the function succeeds, otherwise 1.
 */
uint32_t flash_read(uint32_t block_id, uint8_t *buff,
                    uint32_t offset, uint32_t size);

/**
 * \brief Writes block data from the position specifed by block ID and offset.
 *
 * \param[in] block_id  Block ID
 * \param[in] buff      Buffer pointer to the write data
 * \param[in] offset    Offset position from the init of the block
 * \param[in] size      Number of bytes to write
 *
 * \note This function considers all input values are valid. That means,
 *       the range of address, based on blockid + offset + size, are always
 *       valid in the memory.
 *
 * \return Returns 0 if the function succeeds, otherwise 1.
 */
uint32_t flash_write(uint32_t block_id, const uint8_t *buff,
                     uint32_t offset, uint32_t size);

/**
 * \brief Gets physical address of the given block ID.
 *
 * \param[in]  block_id  Block ID
 * \param[in]  offset    Offset position from the init of the block
 *
 * \returns Returns physical address for the given block ID.
 */
uint32_t flash_get_phys_address(uint32_t block_id, uint32_t offset);

/**
 * \brief Moves data from src block ID to destination block ID.
 *
 * \param[in] dst_block  Destination block ID
 * \param[in] dst_offset Destination offset position from the init of the
 *                       destination block
 * \param[in] src_block  Source block ID
 * \param[in] src_offset Source offset position from the init of the source
 *                       block
 * \param[in] size       Number of bytes to moves
 *
 * \note This function considers all input values are valid. That means,
 *       the range of address, based on block_id + offset + size, are always
 *       valid in the memory.
 *
 * \return Returns 0 if the function succeeds, otherwise 1.
 */
uint32_t flash_block_to_block_move(uint32_t dst_block, uint32_t dst_offset,
                                   uint32_t src_block, uint32_t src_offset,
                                   uint32_t size);

/**
 * \brief Erases block ID data.
 *
 * \param[in] block_id  Block ID
 *
 * \note This function considers all input values valids.
 *
 * \return Returns 0 if the function succeeds, otherwise 1.
 */
uint32_t flash_erase_block(uint32_t block_id);

#ifdef __cplusplus
}
#endif

#endif /* __SST_FLASH_H__ */
