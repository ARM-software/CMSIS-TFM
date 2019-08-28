/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * Original code taken from mcuboot project at:
 * https://github.com/JuulLabs-OSS/mcuboot
 * Git SHA of the original version: 178be54bd6e5f035cc60e98205535682acd26e64
 * Modifications are Copyright (c) 2018-2019 Arm Limited.
 */

/**
 * This file provides an interface to the boot loader.  Functions defined in
 * this file should only be called while the boot loader is running.
 */

#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "flash_map/flash_map.h"
#include "bootutil/bootutil.h"
#include "bootutil/image.h"
#include "bootutil_priv.h"
#include "bl2/include/tfm_boot_status.h"
#include "bl2/include/boot_record.h"
#include "security_cnt.h"

#define BOOT_LOG_LEVEL BOOT_LOG_LEVEL_INFO
#include "bootutil/bootutil_log.h"

static struct boot_loader_state boot_data;

#if !defined(MCUBOOT_NO_SWAP) && !defined(MCUBOOT_RAM_LOADING)

#if defined(MCUBOOT_VALIDATE_PRIMARY_SLOT) && !defined(MCUBOOT_OVERWRITE_ONLY)
static int boot_status_fails = 0;
#define BOOT_STATUS_ASSERT(x)                \
    do {                                     \
        if (!(x)) {                          \
            boot_status_fails++;             \
        }                                    \
    } while (0)
#else
#define BOOT_STATUS_ASSERT(x) assert(x)
#endif

struct boot_status_table {
    uint8_t bst_magic_primary_slot;
    uint8_t bst_magic_scratch;
    uint8_t bst_copy_done_primary_slot;
    uint8_t bst_status_source;
};

/**
 * This set of tables maps swap state contents to boot status location.
 * When searching for a match, these tables must be iterated in order.
 */
static const struct boot_status_table boot_status_tables[] = {
    {
        /*           | primary slot | scratch      |
         * ----------+--------------+--------------|
         *     magic | Good         | Any          |
         * copy-done | Set          | N/A          |
         * ----------+--------------+--------------'
         * source: none                            |
         * ----------------------------------------'
         */
        .bst_magic_primary_slot =     BOOT_MAGIC_GOOD,
        .bst_magic_scratch =          BOOT_MAGIC_ANY,
        .bst_copy_done_primary_slot = BOOT_FLAG_SET,
        .bst_status_source =          BOOT_STATUS_SOURCE_NONE,
    },

    {
        /*           | primary slot | scratch      |
         * ----------+--------------+--------------|
         *     magic | Good         | Any          |
         * copy-done | Unset        | N/A          |
         * ----------+--------------+--------------'
         * source: primary slot                    |
         * ----------------------------------------'
         */
        .bst_magic_primary_slot =     BOOT_MAGIC_GOOD,
        .bst_magic_scratch =          BOOT_MAGIC_ANY,
        .bst_copy_done_primary_slot = BOOT_FLAG_UNSET,
        .bst_status_source =          BOOT_STATUS_SOURCE_PRIMARY_SLOT,
    },

    {
        /*           | primary slot | scratch      |
         * ----------+--------------+--------------|
         *     magic | Any          | Good         |
         * copy-done | Any          | N/A          |
         * ----------+--------------+--------------'
         * source: scratch                         |
         * ----------------------------------------'
         */
        .bst_magic_primary_slot =     BOOT_MAGIC_ANY,
        .bst_magic_scratch =          BOOT_MAGIC_GOOD,
        .bst_copy_done_primary_slot = BOOT_FLAG_ANY,
        .bst_status_source =          BOOT_STATUS_SOURCE_SCRATCH,
    },

    {
        /*           | primary slot | scratch      |
         * ----------+--------------+--------------|
         *     magic | Unset        | Any          |
         * copy-done | Unset        | N/A          |
         * ----------+--------------+--------------|
         * source: varies                          |
         * ----------------------------------------+--------------------------+
         * This represents one of two cases:                                  |
         * o No swaps ever (no status to read, so no harm in checking).       |
         * o Mid-revert; status in the primary slot.                          |
         * -------------------------------------------------------------------'
         */
        .bst_magic_primary_slot =     BOOT_MAGIC_UNSET,
        .bst_magic_scratch =          BOOT_MAGIC_ANY,
        .bst_copy_done_primary_slot = BOOT_FLAG_UNSET,
        .bst_status_source =          BOOT_STATUS_SOURCE_PRIMARY_SLOT,
    },
};

#define BOOT_STATUS_TABLES_COUNT \
    (sizeof(boot_status_tables) / sizeof(boot_status_tables[0]))

#define BOOT_LOG_SWAP_STATE(area, state)                            \
    BOOT_LOG_INF("%s: magic=%5s, copy_done=0x%x, image_ok=0x%x",    \
                 (area),                                            \
                 ((state)->magic == BOOT_MAGIC_GOOD ? "good" :      \
                  (state)->magic == BOOT_MAGIC_UNSET ? "unset" :    \
                  "bad"),                                           \
                 (state)->copy_done,                                \
                 (state)->image_ok)
#endif /* !MCUBOOT_NO_SWAP && !MCUBOOT_RAM_LOADING */


static int
boot_read_image_header(int slot, struct image_header *out_hdr)
{
    const struct flash_area *fap = NULL;
    int area_id;
    int rc;

    area_id = flash_area_id_from_image_slot(slot);
    rc = flash_area_open(area_id, &fap);
    if (rc != 0) {
        rc = BOOT_EFLASH;
        goto done;
    }

    rc = flash_area_read(fap, 0, out_hdr, sizeof(*out_hdr));
    if (rc != 0) {
        rc = BOOT_EFLASH;
        goto done;
    }

    rc = 0;

done:
    flash_area_close(fap);
    return rc;
}

static int
boot_read_image_headers(bool require_all)
{
    int rc;
    int i;

    for (i = 0; i < BOOT_NUM_SLOTS; i++) {
        rc = boot_read_image_header(i, boot_img_hdr(&boot_data, i));
        if (rc != 0) {
            /* If `require_all` is set, fail on any single fail, otherwise
             * if at least the first slot's header was read successfully,
             * then the boot loader can attempt a boot.
             *
             * Failure to read any headers is a fatal error.
             */
            if (i > 0 && !require_all) {
                return 0;
            } else {
                return rc;
            }
        }
    }

    return 0;
}

static uint8_t
boot_write_sz(void)
{
    const struct flash_area *fap;
    uint8_t elem_sz;
    uint8_t align;
    int rc;

    /* Figure out what size to write update status update as.  The size depends
     * on what the minimum write size is for scratch area, active image slot.
     * We need to use the bigger of those 2 values.
     */
    rc = flash_area_open(FLASH_AREA_IMAGE_PRIMARY, &fap);
    assert(rc == 0);
    elem_sz = flash_area_align(fap);
    flash_area_close(fap);

    rc = flash_area_open(FLASH_AREA_IMAGE_SCRATCH, &fap);
    assert(rc == 0);
    align = flash_area_align(fap);
    flash_area_close(fap);

    if (align > elem_sz) {
        elem_sz = align;
    }

    return elem_sz;
}

/**
 * Determines the sector layout of both image slots and the scratch area.
 * This information is necessary for calculating the number of bytes to erase
 * and copy during an image swap.  The information collected during this
 * function is used to populate the boot_data global.
 */
static int
boot_read_sectors(void)
{
    int rc;

    rc = boot_initialize_area(&boot_data, FLASH_AREA_IMAGE_PRIMARY);
    if (rc != 0) {
        return BOOT_EFLASH;
    }

    rc = boot_initialize_area(&boot_data, FLASH_AREA_IMAGE_SECONDARY);
    if (rc != 0) {
        return BOOT_EFLASH;
    }

    BOOT_WRITE_SZ(&boot_data) = boot_write_sz();

    return 0;
}

/**
 * Validate image hash/signature and security counter in a slot.
 */
static int
boot_image_check(struct image_header *hdr, const struct flash_area *fap)
{
    static uint8_t tmpbuf[BOOT_TMPBUF_SZ];

    if (bootutil_img_validate(hdr, fap, tmpbuf, BOOT_TMPBUF_SZ,
                              NULL, 0, NULL)) {
        return BOOT_EBADIMAGE;
    }
    return 0;
}

static inline int
boot_magic_is_erased(uint8_t erased_val, uint32_t magic)
{
    uint8_t i;
    for (i = 0; i < sizeof(magic); i++) {
        if (erased_val != *(((uint8_t *)&magic) + i)) {
            return 0;
        }
    }
    return 1;
}

static int
boot_validate_slot(int slot)
{
    const struct flash_area *fap;
    struct image_header *hdr;
    int rc;

    rc = flash_area_open(flash_area_id_from_image_slot(slot), &fap);
    if (rc != 0) {
        return BOOT_EFLASH;
    }

    hdr = boot_img_hdr(&boot_data, slot);
    if (boot_magic_is_erased(flash_area_erased_val(fap), hdr->ih_magic) ||
            (hdr->ih_flags & IMAGE_F_NON_BOOTABLE)) {
        /* No bootable image in slot; continue booting from the primary slot. */
        return -1;
    }

    if ((hdr->ih_magic != IMAGE_MAGIC ||
        boot_image_check(hdr, fap) != 0)) {
        if (slot != 0) {
            rc = flash_area_erase(fap, 0, fap->fa_size);
            if(rc != 0) {
                flash_area_close(fap);
                return BOOT_EFLASH;
            }
            /* Image in the secondary slot is invalid. Erase the image and
             * continue booting from the primary slot.
             */
        }
        BOOT_LOG_ERR("Authentication failed! Image in the %s slot is not valid."
                     , (slot == BOOT_PRIMARY_SLOT) ? "primary" : "secondary");
        flash_area_close(fap);
        return -1;
    }

    flash_area_close(fap);

    /* Image in the secondary slot is valid. */
    return 0;
}

/**
 * Updates the stored security counter value with the image's security counter
 * value which resides in the given slot if it's greater than the stored value.
 *
 * @param slot      Slot number of the image.
 * @param hdr       Pointer to the image header structure of the image that is
 *                  currently stored in the given slot.
 *
 * @return          0 on success; nonzero on failure.
 */
static int
boot_update_security_counter(int slot, struct image_header *hdr)
{
    const struct flash_area *fap = NULL;
    uint32_t img_security_cnt;
    int rc;

    rc = flash_area_open(flash_area_id_from_image_slot(slot), &fap);
    if (rc != 0) {
        rc = BOOT_EFLASH;
        goto done;
    }

    rc = bootutil_get_img_security_cnt(hdr, fap, &img_security_cnt);
    if (rc != 0) {
        goto done;
    }

    rc = boot_nv_security_counter_update(0, img_security_cnt);
    if (rc != 0) {
        goto done;
    }

done:
    flash_area_close(fap);
    return rc;
}

#if !defined(MCUBOOT_NO_SWAP) && !defined(MCUBOOT_OVERWRITE_ONLY)
/*
 * Compute the total size of the given image.  Includes the size of
 * the TLVs.
 */
static int
boot_read_image_size(int slot, struct image_header *hdr, uint32_t *size)
{
    const struct flash_area *fap = NULL;
    struct image_tlv_info info;
    int area_id;
    int rc;

    area_id = flash_area_id_from_image_slot(slot);
    rc = flash_area_open(area_id, &fap);
    if (rc != 0) {
        rc = BOOT_EFLASH;
        goto done;
    }

    rc = flash_area_read(fap, hdr->ih_hdr_size + hdr->ih_img_size,
                         &info, sizeof(info));
    if (rc != 0) {
        rc = BOOT_EFLASH;
        goto done;
    }
    if (info.it_magic != IMAGE_TLV_INFO_MAGIC) {
        rc = BOOT_EBADIMAGE;
        goto done;
    }
    *size = hdr->ih_hdr_size + hdr->ih_img_size + info.it_tlv_tot;
    rc = 0;

done:
    flash_area_close(fap);
    return rc;
}
#endif /* !MCUBOOT_NO_SWAP && !MCUBOOT_OVERWRITE_ONLY */

#if !defined(MCUBOOT_NO_SWAP) && !defined(MCUBOOT_RAM_LOADING)
/**
 * Determines where in flash the most recent boot status is stored. The boot
 * status is necessary for completing a swap that was interrupted by a boot
 * loader reset.
 *
 * @return  BOOT_STATUS_SOURCE_[...] code indicating where
 *          status should be read from.
 */
static int
boot_status_source(void)
{
    const struct boot_status_table *table;
    struct boot_swap_state state_scratch;
    struct boot_swap_state state_primary_slot;
    int rc;
    size_t i;
    uint8_t source;

    rc = boot_read_swap_state_by_id(FLASH_AREA_IMAGE_PRIMARY,
                                    &state_primary_slot);
    assert(rc == 0);

    rc = boot_read_swap_state_by_id(FLASH_AREA_IMAGE_SCRATCH, &state_scratch);
    assert(rc == 0);

    BOOT_LOG_SWAP_STATE("Image 0", &state_primary_slot);
    BOOT_LOG_SWAP_STATE("Scratch", &state_scratch);

    for (i = 0; i < BOOT_STATUS_TABLES_COUNT; i++) {
        table = &boot_status_tables[i];

        if ((table->bst_magic_primary_slot == BOOT_MAGIC_ANY    ||
             table->bst_magic_primary_slot == state_primary_slot.magic) &&
            (table->bst_magic_scratch == BOOT_MAGIC_ANY         ||
             table->bst_magic_scratch == state_scratch.magic)           &&
            (table->bst_copy_done_primary_slot == BOOT_FLAG_ANY ||
             table->bst_copy_done_primary_slot == state_primary_slot.copy_done))
        {
            source = table->bst_status_source;
            BOOT_LOG_INF("Boot source: %s",
                         source == BOOT_STATUS_SOURCE_NONE ? "none" :
                         source == BOOT_STATUS_SOURCE_SCRATCH ? "scratch" :
                         source == BOOT_STATUS_SOURCE_PRIMARY_SLOT ?
                                   "primary slot" : "BUG; can't happen");
            return source;
        }
    }

    BOOT_LOG_INF("Boot source: none");
    return BOOT_STATUS_SOURCE_NONE;
}

/**
 * Calculates the type of swap that just completed.
 *
 * This is used when a swap is interrupted by an external event. After
 * finishing the swap operation determines what the initial request was.
 */
static int
boot_previous_swap_type(void)
{
    int post_swap_type;

    post_swap_type = boot_swap_type();

    switch (post_swap_type) {
    case BOOT_SWAP_TYPE_NONE   : return BOOT_SWAP_TYPE_PERM;
    case BOOT_SWAP_TYPE_REVERT : return BOOT_SWAP_TYPE_TEST;
    case BOOT_SWAP_TYPE_PANIC  : return BOOT_SWAP_TYPE_PANIC;
    }

    return BOOT_SWAP_TYPE_FAIL;
}

static int
boot_slots_compatible(void)
{
    size_t num_sectors_0 = boot_img_num_sectors(&boot_data,
                                                BOOT_PRIMARY_SLOT);
    size_t num_sectors_1 = boot_img_num_sectors(&boot_data,
                                                BOOT_SECONDARY_SLOT);
    size_t size_0, size_1;
    size_t i;

    if (num_sectors_0 > BOOT_MAX_IMG_SECTORS || num_sectors_1 > BOOT_MAX_IMG_SECTORS) {
        BOOT_LOG_WRN("Cannot upgrade: more sectors than allowed");
        return 0;
    }

    /* Ensure both image slots have identical sector layouts. */
    if (num_sectors_0 != num_sectors_1) {
        BOOT_LOG_WRN("Cannot upgrade: number of sectors differ between slots");
        return 0;
    }

    for (i = 0; i < num_sectors_0; i++) {
        size_0 = boot_img_sector_size(&boot_data, BOOT_PRIMARY_SLOT, i);
        size_1 = boot_img_sector_size(&boot_data, BOOT_SECONDARY_SLOT, i);
        if (size_0 != size_1) {
            BOOT_LOG_WRN("Cannot upgrade: an incompatible sector was found");
            return 0;
        }
    }

    return 1;
}

static uint32_t
boot_status_internal_off(int idx, int state, int elem_sz)
{
    int idx_sz;

    idx_sz = elem_sz * BOOT_STATUS_STATE_COUNT;

    return (idx - BOOT_STATUS_IDX_0) * idx_sz +
           (state - BOOT_STATUS_STATE_0) * elem_sz;
}

/**
 * Reads the status of a partially-completed swap, if any.  This is necessary
 * to recover in case the boot lodaer was reset in the middle of a swap
 * operation.
 */
static int
boot_read_status_bytes(const struct flash_area *fap, struct boot_status *bs)
{
    uint32_t off;
    uint8_t status;
    int max_entries;
    int found;
    int found_idx;
    int invalid;
    int rc;
    int i;

    off = boot_status_off(fap);
    max_entries = boot_status_entries(fap);

    found = 0;
    found_idx = 0;
    invalid = 0;
    for (i = 0; i < max_entries; i++) {
        rc = flash_area_read_is_empty(fap, off + i * BOOT_WRITE_SZ(&boot_data),
                &status, 1);
        if (rc < 0) {
            return BOOT_EFLASH;
        }

        if (rc == 1) {
            if (found && !found_idx) {
                found_idx = i;
            }
        } else if (!found) {
            found = 1;
        } else if (found_idx) {
            invalid = 1;
            break;
        }
    }

    if (invalid) {
        /* This means there was an error writing status on the last
         * swap. Tell user and move on to validation!
         */
        BOOT_LOG_ERR("Detected inconsistent status!");

#if !defined(MCUBOOT_VALIDATE_PRIMARY_SLOT)
        /* With validation of the primary slot disabled, there is no way
         * to be sure the swapped primary slot is OK, so abort!
         */
        assert(0);
#endif
    }

    if (found) {
        if (!found_idx) {
            found_idx = i;
        }
        found_idx--;
        bs->idx = (found_idx / BOOT_STATUS_STATE_COUNT) + 1;
        bs->state = (found_idx % BOOT_STATUS_STATE_COUNT) + 1;
    }

    return 0;
}

/**
 * Reads the boot status from the flash.  The boot status contains
 * the current state of an interrupted image copy operation.  If the boot
 * status is not present, or it indicates that previous copy finished,
 * there is no operation in progress.
 */
static int
boot_read_status(struct boot_status *bs)
{
    const struct flash_area *fap;
    int status_loc;
    int area_id;
    int rc;

    memset(bs, 0, sizeof *bs);
    bs->idx = BOOT_STATUS_IDX_0;
    bs->state = BOOT_STATUS_STATE_0;

#ifdef MCUBOOT_OVERWRITE_ONLY
    /* Overwrite-only doesn't make use of the swap status area. */
    return 0;
#endif

    status_loc = boot_status_source();
    switch (status_loc) {
    case BOOT_STATUS_SOURCE_NONE:
        return 0;

    case BOOT_STATUS_SOURCE_SCRATCH:
        area_id = FLASH_AREA_IMAGE_SCRATCH;
        break;

    case BOOT_STATUS_SOURCE_PRIMARY_SLOT:
        area_id = FLASH_AREA_IMAGE_PRIMARY;
        break;

    default:
        assert(0);
        return BOOT_EBADARGS;
    }

    rc = flash_area_open(area_id, &fap);
    if (rc != 0) {
        return BOOT_EFLASH;
    }

    rc = boot_read_status_bytes(fap, bs);

    flash_area_close(fap);

    return rc;
}

/**
 * Writes the supplied boot status to the flash file system.  The boot status
 * contains the current state of an in-progress image copy operation.
 *
 * @param bs                    The boot status to write.
 *
 * @return                      0 on success; nonzero on failure.
 */
int
boot_write_status(struct boot_status *bs)
{
    const struct flash_area *fap = NULL;
    uint32_t off;
    int area_id;
    int rc;
    uint8_t buf[BOOT_MAX_ALIGN];
    uint8_t align;
    uint8_t erased_val;

    /* NOTE: The first sector copied (that is the last sector on slot) contains
     *       the trailer. Since in the last step the primary slot is erased, the
     *       first two status writes go to the scratch which will be copied to
     *       the primary slot!
     */

    if (bs->use_scratch) {
        /* Write to scratch. */
        area_id = FLASH_AREA_IMAGE_SCRATCH;
    } else {
        /* Write to the primary slot. */
        area_id = FLASH_AREA_IMAGE_PRIMARY;
    }

    rc = flash_area_open(area_id, &fap);
    if (rc != 0) {
        rc = BOOT_EFLASH;
        goto done;
    }

    off = boot_status_off(fap) +
          boot_status_internal_off(bs->idx, bs->state,
                                   BOOT_WRITE_SZ(&boot_data));

    align = flash_area_align(fap);
    erased_val = flash_area_erased_val(fap);
    memset(buf, erased_val, BOOT_MAX_ALIGN);
    buf[0] = bs->state;

    rc = flash_area_write(fap, off, buf, align);
    if (rc != 0) {
        rc = BOOT_EFLASH;
        goto done;
    }

    rc = 0;

done:
    flash_area_close(fap);
    return rc;
}

/**
 * Determines which swap operation to perform, if any.  If it is determined
 * that a swap operation is required, the image in the secondary slot is checked
 * for validity.  If the image in the secondary slot is invalid, it is erased,
 * and a swap type of "none" is indicated.
 *
 * @return                      The type of swap to perform (BOOT_SWAP_TYPE...)
 */
static int
boot_validated_swap_type(void)
{
    int swap_type;

    swap_type = boot_swap_type();
    switch (swap_type) {
    case BOOT_SWAP_TYPE_TEST:
    case BOOT_SWAP_TYPE_PERM:
    case BOOT_SWAP_TYPE_REVERT:
        /* Boot loader wants to switch to the secondary slot.
         * Ensure image is valid.
         */
        if (boot_validate_slot(BOOT_SECONDARY_SLOT) != 0) {
            swap_type = BOOT_SWAP_TYPE_FAIL;
        }
    }

    return swap_type;
}

/**
 * Calculates the number of sectors the scratch area can contain.  A "last"
 * source sector is specified because images are copied backwards in flash
 * (final index to index number 0).
 *
 * @param last_sector_idx       The index of the last source sector
 *                                  (inclusive).
 * @param out_first_sector_idx  The index of the first source sector
 *                                  (inclusive) gets written here.
 *
 * @return                      The number of bytes comprised by the
 *                                  [first-sector, last-sector] range.
 */
#ifndef MCUBOOT_OVERWRITE_ONLY
static uint32_t
boot_copy_sz(int last_sector_idx, int *out_first_sector_idx)
{
    size_t scratch_sz;
    uint32_t new_sz;
    uint32_t sz;
    int i;

    sz = 0;

    scratch_sz = boot_scratch_area_size(&boot_data);
    for (i = last_sector_idx; i >= 0; i--) {
        new_sz = sz + boot_img_sector_size(&boot_data, BOOT_PRIMARY_SLOT, i);
        if (new_sz > scratch_sz) {
            break;
        }
        sz = new_sz;
    }

    /* i currently refers to a sector that doesn't fit or it is -1 because all
     * sectors have been processed.  In both cases, exclude sector i.
     */
    *out_first_sector_idx = i + 1;
    return sz;
}
#endif /* !MCUBOOT_OVERWRITE_ONLY */

/**
 * Erases a region of flash.
 *
 * @param flash_area_idx        The ID of the flash area containing the region
 *                                  to erase.
 * @param off                   The offset within the flash area to start the
 *                                  erase.
 * @param sz                    The number of bytes to erase.
 *
 * @return                      0 on success; nonzero on failure.
 */
static int
boot_erase_sector(int flash_area_id, uint32_t off, uint32_t sz)
{
    const struct flash_area *fap = NULL;
    int rc;

    rc = flash_area_open(flash_area_id, &fap);
    if (rc != 0) {
        rc = BOOT_EFLASH;
        goto done;
    }

    rc = flash_area_erase(fap, off, sz);
    if (rc != 0) {
        rc = BOOT_EFLASH;
        goto done;
    }

    rc = 0;

done:
    flash_area_close(fap);
    return rc;
}

/**
 * Copies the contents of one flash region to another.  You must erase the
 * destination region prior to calling this function.
 *
 * @param flash_area_id_src     The ID of the source flash area.
 * @param flash_area_id_dst     The ID of the destination flash area.
 * @param off_src               The offset within the source flash area to
 *                                  copy from.
 * @param off_dst               The offset within the destination flash area to
 *                                  copy to.
 * @param sz                    The number of bytes to copy.
 *
 * @return                      0 on success; nonzero on failure.
 */
static int
boot_copy_sector(int flash_area_id_src, int flash_area_id_dst,
                 uint32_t off_src, uint32_t off_dst, uint32_t sz)
{
    const struct flash_area *fap_src;
    const struct flash_area *fap_dst;
    uint32_t bytes_copied;
    int chunk_sz;
    int rc;

    static uint8_t buf[1024];

    fap_src = NULL;
    fap_dst = NULL;

    rc = flash_area_open(flash_area_id_src, &fap_src);
    if (rc != 0) {
        rc = BOOT_EFLASH;
        goto done;
    }

    rc = flash_area_open(flash_area_id_dst, &fap_dst);
    if (rc != 0) {
        rc = BOOT_EFLASH;
        goto done;
    }

    bytes_copied = 0;
    while (bytes_copied < sz) {
        if (sz - bytes_copied > sizeof(buf)) {
            chunk_sz = sizeof(buf);
        } else {
            chunk_sz = sz - bytes_copied;
        }

        rc = flash_area_read(fap_src, off_src + bytes_copied, buf, chunk_sz);
        if (rc != 0) {
            rc = BOOT_EFLASH;
            goto done;
        }

        rc = flash_area_write(fap_dst, off_dst + bytes_copied, buf, chunk_sz);
        if (rc != 0) {
            rc = BOOT_EFLASH;
            goto done;
        }

        bytes_copied += chunk_sz;
    }

    rc = 0;

done:
    if (fap_src) {
        flash_area_close(fap_src);
    }
    if (fap_dst) {
        flash_area_close(fap_dst);
    }
    return rc;
}

#ifndef MCUBOOT_OVERWRITE_ONLY
static inline int
boot_status_init_by_id(int flash_area_id, const struct boot_status *bs)
{
    const struct flash_area *fap;
    struct boot_swap_state swap_state;
    int rc;

    rc = flash_area_open(flash_area_id, &fap);
    assert(rc == 0);

    rc = boot_read_swap_state_by_id(FLASH_AREA_IMAGE_SECONDARY, &swap_state);
    assert(rc == 0);

    if (swap_state.image_ok == BOOT_FLAG_SET) {
        rc = boot_write_image_ok(fap);
        assert(rc == 0);
    }

    rc = boot_write_swap_size(fap, bs->swap_size);
    assert(rc == 0);

    rc = boot_write_magic(fap);
    assert(rc == 0);

    flash_area_close(fap);

    return 0;
}

static int
boot_erase_last_sector_by_id(int flash_area_id)
{
    uint8_t slot;
    uint32_t last_sector;
    int rc;

    switch (flash_area_id) {
    case FLASH_AREA_IMAGE_PRIMARY:
        slot = BOOT_PRIMARY_SLOT;
        break;
    case FLASH_AREA_IMAGE_SECONDARY:
        slot = BOOT_SECONDARY_SLOT;
        break;
    default:
        return BOOT_EFLASH;
    }

    last_sector = boot_img_num_sectors(&boot_data, slot) - 1;
    rc = boot_erase_sector(flash_area_id,
            boot_img_sector_off(&boot_data, slot, last_sector),
            boot_img_sector_size(&boot_data, slot, last_sector));
    assert(rc == 0);

    return rc;
}
#endif /* !MCUBOOT_OVERWRITE_ONLY */

/**
 * Swaps the contents of two flash regions within the two image slots.
 *
 * @param idx                   The index of the first sector in the range of
 *                                  sectors being swapped.
 * @param sz                    The number of bytes to swap.
 * @param bs                    The current boot status.  This struct gets
 *                                  updated according to the outcome.
 *
 * @return                      0 on success; nonzero on failure.
 */
#ifndef MCUBOOT_OVERWRITE_ONLY
static void
boot_swap_sectors(int idx, uint32_t sz, struct boot_status *bs)
{
    const struct flash_area *fap;
    uint32_t copy_sz;
    uint32_t trailer_sz;
    uint32_t img_off;
    uint32_t scratch_trailer_off;
    struct boot_swap_state swap_state;
    size_t last_sector;
    int rc;

    /* Calculate offset from start of image area. */
    img_off = boot_img_sector_off(&boot_data, BOOT_PRIMARY_SLOT, idx);

    copy_sz = sz;
    trailer_sz = boot_slots_trailer_sz(BOOT_WRITE_SZ(&boot_data));

    /* sz in this function is always is always sized on a multiple of the
     * sector size. The check against the start offset of the last sector
     * is to determine if we're swapping the last sector. The last sector
     * needs special handling because it's where the trailer lives. If we're
     * copying it, we need to use scratch to write the trailer temporarily.
     *
     * NOTE: `use_scratch` is a temporary flag (never written to flash) which
     * controls if special handling is needed (swapping last sector).
     */
    last_sector = boot_img_num_sectors(&boot_data, BOOT_PRIMARY_SLOT) - 1;
    if (img_off + sz > boot_img_sector_off(&boot_data, BOOT_PRIMARY_SLOT,
                                           last_sector)) {
        copy_sz -= trailer_sz;
    }

    bs->use_scratch = (bs->idx == BOOT_STATUS_IDX_0 && copy_sz != sz);

    if (bs->state == BOOT_STATUS_STATE_0) {
        rc = boot_erase_sector(FLASH_AREA_IMAGE_SCRATCH, 0, sz);
        assert(rc == 0);

        rc = boot_copy_sector(FLASH_AREA_IMAGE_SECONDARY, FLASH_AREA_IMAGE_SCRATCH,
                              img_off, 0, copy_sz);
        assert(rc == 0);

        if (bs->idx == BOOT_STATUS_IDX_0) {
            if (bs->use_scratch) {
                boot_status_init_by_id(FLASH_AREA_IMAGE_SCRATCH, bs);
            } else {
                /* Prepare the status area... here it is known that the
                 * last sector is not being used by the image data so it's
                 * safe to erase.
                 */
                rc = boot_erase_last_sector_by_id(FLASH_AREA_IMAGE_PRIMARY);
                assert(rc == 0);

                boot_status_init_by_id(FLASH_AREA_IMAGE_PRIMARY, bs);
            }
        }

        bs->state = BOOT_STATUS_STATE_1;
        rc = boot_write_status(bs);
        BOOT_STATUS_ASSERT(rc == 0);
    }

    if (bs->state == BOOT_STATUS_STATE_1) {
        rc = boot_erase_sector(FLASH_AREA_IMAGE_SECONDARY, img_off, sz);
        assert(rc == 0);

        rc = boot_copy_sector(FLASH_AREA_IMAGE_PRIMARY,
                              FLASH_AREA_IMAGE_SECONDARY,
                              img_off, img_off, copy_sz);
        assert(rc == 0);

        if (bs->idx == BOOT_STATUS_IDX_0 && !bs->use_scratch) {
            /* If not all sectors of the slot are being swapped,
             * guarantee here that only the primary slot will have the state.
             */
            rc = boot_erase_last_sector_by_id(FLASH_AREA_IMAGE_SECONDARY);
            assert(rc == 0);
        }

        bs->state = BOOT_STATUS_STATE_2;
        rc = boot_write_status(bs);
        BOOT_STATUS_ASSERT(rc == 0);
    }

    if (bs->state == BOOT_STATUS_STATE_2) {
        rc = boot_erase_sector(FLASH_AREA_IMAGE_PRIMARY, img_off, sz);
        assert(rc == 0);

        /* NOTE: also copy trailer from scratch (has status info) */
        rc = boot_copy_sector(FLASH_AREA_IMAGE_SCRATCH,
                              FLASH_AREA_IMAGE_PRIMARY,
                              0, img_off, copy_sz);
        assert(rc == 0);

        if (bs->use_scratch) {
            rc = flash_area_open(FLASH_AREA_IMAGE_SCRATCH, &fap);
            assert(rc == 0);

            scratch_trailer_off = boot_status_off(fap);

            flash_area_close(fap);

            rc = flash_area_open(FLASH_AREA_IMAGE_PRIMARY, &fap);
            assert(rc == 0);

            /* copy current status that is being maintained in scratch */
            rc = boot_copy_sector(FLASH_AREA_IMAGE_SCRATCH,
                        FLASH_AREA_IMAGE_PRIMARY,
                        scratch_trailer_off,
                        img_off + copy_sz,
                        BOOT_STATUS_STATE_COUNT * BOOT_WRITE_SZ(&boot_data));
            BOOT_STATUS_ASSERT(rc == 0);

            rc = boot_read_swap_state_by_id(FLASH_AREA_IMAGE_SCRATCH,
                                            &swap_state);
            assert(rc == 0);

            if (swap_state.image_ok == BOOT_FLAG_SET) {
                rc = boot_write_image_ok(fap);
                assert(rc == 0);
            }

            rc = boot_write_swap_size(fap, bs->swap_size);
            assert(rc == 0);

            rc = boot_write_magic(fap);
            assert(rc == 0);

            flash_area_close(fap);
        }

        bs->idx++;
        bs->state = BOOT_STATUS_STATE_0;
        bs->use_scratch = 0;
        rc = boot_write_status(bs);
        BOOT_STATUS_ASSERT(rc == 0);
    }
}
#endif /* !MCUBOOT_OVERWRITE_ONLY */

/**
 * Swaps the two images in flash.  If a prior copy operation was interrupted
 * by a system reset, this function completes that operation.
 *
 * @param bs                    The current boot status.  This function reads
 *                                  this struct to determine if it is resuming
 *                                  an interrupted swap operation.  This
 *                                  function writes the updated status to this
 *                                  function on return.
 *
 * @return                      0 on success; nonzero on failure.
 */
#ifdef MCUBOOT_OVERWRITE_ONLY
static int
boot_copy_image(struct boot_status *bs)
{
    size_t sect_count;
    size_t sect;
    int rc;
    size_t size = 0;
    size_t this_size;
    size_t last_sector;

    (void)bs;

    BOOT_LOG_INF("Image upgrade secondary slot -> primary slot");
    BOOT_LOG_INF("Erasing the primary slot");

    sect_count = boot_img_num_sectors(&boot_data, BOOT_PRIMARY_SLOT);
    for (sect = 0; sect < sect_count; sect++) {
        this_size = boot_img_sector_size(&boot_data, BOOT_PRIMARY_SLOT, sect);
        rc = boot_erase_sector(FLASH_AREA_IMAGE_PRIMARY,
                               size,
                               this_size);
        assert(rc == 0);

        size += this_size;
    }

    BOOT_LOG_INF("Copying the secondary slot to the primary slot: 0x%zx bytes",
                 size);
    rc = boot_copy_sector(FLASH_AREA_IMAGE_SECONDARY, FLASH_AREA_IMAGE_PRIMARY,
                          0, 0, size);

    /* Update the stored security counter with the new image's security counter
     * value. Both slots hold the new image at this point, but the secondary
     * slot's image header must be passed because the read image headers in the
     * boot_data structure have not been updated yet.
     */
    rc = boot_update_security_counter(BOOT_PRIMARY_SLOT,
                                 boot_img_hdr(&boot_data, BOOT_SECONDARY_SLOT));
    if (rc != 0) {
        BOOT_LOG_ERR("Security counter update failed after image upgrade.");
        return rc;
    }

    /*
     * Erases header and trailer. The trailer is erased because when a new
     * image is written without a trailer as is the case when using newt, the
     * trailer that was left might trigger a new upgrade.
     */
    rc = boot_erase_sector(FLASH_AREA_IMAGE_SECONDARY,
                           boot_img_sector_off(&boot_data,
                                               BOOT_SECONDARY_SLOT, 0),
                           boot_img_sector_size(&boot_data,
                                                BOOT_SECONDARY_SLOT, 0));
    assert(rc == 0);
    last_sector = boot_img_num_sectors(&boot_data, BOOT_SECONDARY_SLOT) - 1;
    rc = boot_erase_sector(FLASH_AREA_IMAGE_SECONDARY,
                           boot_img_sector_off(&boot_data, BOOT_SECONDARY_SLOT,
                                               last_sector),
                           boot_img_sector_size(&boot_data, BOOT_SECONDARY_SLOT,
                                                last_sector));
    assert(rc == 0);

    /* TODO: Perhaps verify the primary slot's signature again? */

    return 0;
}
#else
static int
boot_copy_image(struct boot_status *bs)
{
    uint32_t sz;
    int first_sector_idx;
    int last_sector_idx;
    uint32_t swap_idx;
    struct image_header *hdr;
    uint32_t size;
    uint32_t copy_size;
    int rc;

    /* FIXME: just do this if asked by user? */

    size = copy_size = 0;

    if (bs->idx == BOOT_STATUS_IDX_0 && bs->state == BOOT_STATUS_STATE_0) {
        /*
         * No swap ever happened, so need to find the largest image which
         * will be used to determine the amount of sectors to swap.
         */
        hdr = boot_img_hdr(&boot_data, BOOT_PRIMARY_SLOT);
        if (hdr->ih_magic == IMAGE_MAGIC) {
            rc = boot_read_image_size(BOOT_PRIMARY_SLOT, hdr, &copy_size);
            assert(rc == 0);
        }

        hdr = boot_img_hdr(&boot_data, BOOT_SECONDARY_SLOT);
        if (hdr->ih_magic == IMAGE_MAGIC) {
            rc = boot_read_image_size(BOOT_SECONDARY_SLOT, hdr, &size);
            assert(rc == 0);
        }

        if (size > copy_size) {
            copy_size = size;
        }

        bs->swap_size = copy_size;
    } else {
        /*
         * If a swap was under way, the swap_size should already be present
         * in the trailer...
         */
        rc = boot_read_swap_size(&bs->swap_size);
        assert(rc == 0);

        copy_size = bs->swap_size;
    }

    size = 0;
    last_sector_idx = 0;
    while (1) {
        size += boot_img_sector_size(&boot_data, BOOT_PRIMARY_SLOT,
                                     last_sector_idx);
        if (size >= copy_size) {
            break;
        }
        last_sector_idx++;
    }

    swap_idx = 0;
    while (last_sector_idx >= 0) {
        sz = boot_copy_sz(last_sector_idx, &first_sector_idx);
        if (swap_idx >= (bs->idx - BOOT_STATUS_IDX_0)) {
            boot_swap_sectors(first_sector_idx, sz, bs);
        }

        last_sector_idx = first_sector_idx - 1;
        swap_idx++;
    }

#ifdef MCUBOOT_VALIDATE_PRIMARY_SLOT
    if (boot_status_fails > 0) {
        BOOT_LOG_WRN("%d status write fails performing the swap", boot_status_fails);
    }
#endif

    return 0;
}
#endif

/**
 * Marks the image in the primary slot as fully copied.
 */
#ifndef MCUBOOT_OVERWRITE_ONLY
static int
boot_set_copy_done(void)
{
    const struct flash_area *fap;
    int rc;

    rc = flash_area_open(FLASH_AREA_IMAGE_PRIMARY, &fap);
    if (rc != 0) {
        return BOOT_EFLASH;
    }

    rc = boot_write_copy_done(fap);
    flash_area_close(fap);
    return rc;
}
#endif /* !MCUBOOT_OVERWRITE_ONLY */

/**
 * Marks a reverted image in the primary slot as confirmed. This is necessary to
 * ensure the status bytes from the image revert operation don't get processed
 * on a subsequent boot.
 *
 * NOTE: image_ok is tested before writing because if there's a valid permanent
 * image installed on the primary slot and the new image to be upgrade to has a
 * bad sig, image_ok would be overwritten.
 */
#ifndef MCUBOOT_OVERWRITE_ONLY
static int
boot_set_image_ok(void)
{
    const struct flash_area *fap;
    struct boot_swap_state state;
    int rc;

    rc = flash_area_open(FLASH_AREA_IMAGE_PRIMARY, &fap);
    if (rc != 0) {
        return BOOT_EFLASH;
    }

    rc = boot_read_swap_state(fap, &state);
    if (rc != 0) {
        rc = BOOT_EFLASH;
        goto out;
    }

    if (state.image_ok == BOOT_FLAG_UNSET) {
        rc = boot_write_image_ok(fap);
    }

out:
    flash_area_close(fap);
    return rc;
}
#endif /* !MCUBOOT_OVERWRITE_ONLY */

/**
 * Performs an image swap if one is required.
 *
 * @param out_swap_type         On success, the type of swap performed gets
 *                                  written here.
 *
 * @return                      0 on success; nonzero on failure.
 */
static int
boot_swap_if_needed(int *out_swap_type)
{
    struct boot_status bs;
    int swap_type;
    int rc;

    /* Determine if we rebooted in the middle of an image swap
     * operation.
     */
    rc = boot_read_status(&bs);
    assert(rc == 0);
    if (rc != 0) {
        return rc;
    }

    /* If a partial swap was detected, complete it. */
    if (bs.idx != BOOT_STATUS_IDX_0 || bs.state != BOOT_STATUS_STATE_0) {
        rc = boot_copy_image(&bs);
        assert(rc == 0);

        /* NOTE: here we have finished a swap resume. The initial request
         * was either a TEST or PERM swap, which now after the completed
         * swap will be determined to be respectively REVERT (was TEST)
         * or NONE (was PERM).
         */

        /* Extrapolate the type of the partial swap.  We need this
         * information to know how to mark the swap complete in flash.
         */
        swap_type = boot_previous_swap_type();
    } else {
        swap_type = boot_validated_swap_type();
        switch (swap_type) {
        case BOOT_SWAP_TYPE_TEST:
        case BOOT_SWAP_TYPE_PERM:
        case BOOT_SWAP_TYPE_REVERT:
            rc = boot_copy_image(&bs);
            assert(rc == 0);
            break;
        }
    }

    *out_swap_type = swap_type;
    return 0;
}

/**
 * Prepares the booting process.  This function moves images around in flash as
 * appropriate, and tells you what address to boot from.
 *
 * @param rsp                   On success, indicates how booting should occur.
 *
 * @return                      0 on success; nonzero on failure.
 */
int
boot_go(struct boot_rsp *rsp)
{
    int swap_type;
    size_t slot;
    int rc;
    int fa_id;
    bool reload_headers = false;

    /* The array of slot sectors are defined here (as opposed to file scope) so
     * that they don't get allocated for non-boot-loader apps.  This is
     * necessary because the gcc option "-fdata-sections" doesn't seem to have
     * any effect in older gcc versions (e.g., 4.8.4).
     */
    static boot_sector_t primary_slot_sectors[BOOT_MAX_IMG_SECTORS];
    static boot_sector_t secondary_slot_sectors[BOOT_MAX_IMG_SECTORS];

    boot_data.imgs[BOOT_PRIMARY_SLOT].sectors = primary_slot_sectors;
    boot_data.imgs[BOOT_SECONDARY_SLOT].sectors = secondary_slot_sectors;

    /* Open boot_data image areas for the duration of this call. */
    for (slot = 0; slot < BOOT_NUM_SLOTS; slot++) {
        fa_id = flash_area_id_from_image_slot(slot);
        rc = flash_area_open(fa_id, &BOOT_IMG_AREA(&boot_data, slot));
        assert(rc == 0);
    }

    rc = flash_area_open(FLASH_AREA_IMAGE_SCRATCH,
                         &BOOT_SCRATCH_AREA(&boot_data));
    assert(rc == 0);

    /* Determine the sector layout of the image slots and scratch area. */
    rc = boot_read_sectors();
    if (rc != 0) {
        BOOT_LOG_WRN("Failed reading sectors; BOOT_MAX_IMG_SECTORS=%d - too small?",
                BOOT_MAX_IMG_SECTORS);
        goto out;
    }

    /* Attempt to read an image header from each slot. */
    rc = boot_read_image_headers(false);
    if (rc != 0) {
        goto out;
    }

    /* If the image slots aren't compatible, no swap is possible.  Just boot
     * into the primary slot.
     */
    if (boot_slots_compatible()) {
        rc = boot_swap_if_needed(&swap_type);
        assert(rc == 0);
        if (rc != 0) {
            goto out;
        }

        /*
         * The following states need image_ok be explicitly set after the
         * swap was finished to avoid a new revert.
         */
        if (swap_type == BOOT_SWAP_TYPE_REVERT ||
            swap_type == BOOT_SWAP_TYPE_FAIL) {
#ifndef MCUBOOT_OVERWRITE_ONLY
            rc = boot_set_image_ok();
            if (rc != 0) {
                swap_type = BOOT_SWAP_TYPE_PANIC;
            }
#endif /* !MCUBOOT_OVERWRITE_ONLY */
        }
    } else {
        swap_type = BOOT_SWAP_TYPE_NONE;
    }

    switch (swap_type) {
    case BOOT_SWAP_TYPE_NONE:
        slot = BOOT_PRIMARY_SLOT;
        break;

    case BOOT_SWAP_TYPE_TEST:          /* fallthrough */
    case BOOT_SWAP_TYPE_PERM:          /* fallthrough */
    case BOOT_SWAP_TYPE_REVERT:
        slot = BOOT_SECONDARY_SLOT;
        reload_headers = true;
#ifndef MCUBOOT_OVERWRITE_ONLY
        if (swap_type == BOOT_SWAP_TYPE_PERM) {
            /* Update the stored security counter with the new image's security
             * counter value. The primary slot holds the new image at this
             * point, but the secondary slot's image header must be passed
             * because the read image headers in the boot_data structure have
             * not been updated yet.
             *
             * In case of a permanent image swap mcuboot will never attempt to
             * revert the images on the next reboot. Therefore, the security
             * counter must be increased right after the image upgrade.
             */
            rc = boot_update_security_counter(BOOT_PRIMARY_SLOT,
                                 boot_img_hdr(&boot_data, BOOT_SECONDARY_SLOT));
            if (rc != 0) {
                BOOT_LOG_ERR("Security counter update failed after "
                             "image upgrade.");
                goto out;
            }
        }

        rc = boot_set_copy_done();
        if (rc != 0) {
            swap_type = BOOT_SWAP_TYPE_PANIC;
        }
#endif /* !MCUBOOT_OVERWRITE_ONLY */
        break;

    case BOOT_SWAP_TYPE_FAIL:
        /* The image in the secondary slot was invalid and is now erased.
         * Ensure we don't try to boot into it again on the next reboot.
         * Do this by pretending we just reverted back to the primary slot.
         */
        slot = BOOT_PRIMARY_SLOT;
        reload_headers = true;
        break;

    default:
        swap_type = BOOT_SWAP_TYPE_PANIC;
    }

    if (swap_type == BOOT_SWAP_TYPE_PANIC) {
        BOOT_LOG_ERR("panic!");
        assert(0);

        /* Loop forever... */
        while (1) {}
    }

    if (reload_headers) {
        rc = boot_read_image_headers(false);
        if (rc != 0) {
            goto out;
        }
        /* Since headers were reloaded, it can be assumed we just performed a
         * swap or overwrite. Now the header info that should be used to
         * provide the data for the bootstrap, which previously was at the
         * secondary slot, was updated to the primary slot.
         */
        slot = BOOT_PRIMARY_SLOT;
    }

#ifdef MCUBOOT_VALIDATE_PRIMARY_SLOT
    rc = boot_validate_slot(BOOT_PRIMARY_SLOT);
    assert(rc == 0);
    if (rc != 0) {
        rc = BOOT_EBADIMAGE;
        goto out;
    }
#else
    /* Even if we're not re-validating the primary slot, we could be booting
     * onto an empty flash chip. At least do a basic sanity check that
     * the magic number on the image is OK.
     */
    if (boot_data.imgs[BOOT_PRIMARY_SLOT].hdr.ih_magic != IMAGE_MAGIC) {
        BOOT_LOG_ERR("bad image magic 0x%lx",
                (unsigned long)boot_data.imgs[BOOT_PRIMARY_SLOT].hdr.ih_magic);
        rc = BOOT_EBADIMAGE;
        goto out;
    }
#endif /* MCUBOOT_VALIDATE_PRIMARY_SLOT */

    /* Update the stored security counter with the active image's security
     * counter value. It will be updated only if the new security counter is
     * greater than the stored value.
     *
     * In case of a successful image swapping when the swap type is TEST the
     * security counter can be increased only after a reset, when the swap type
     * is NONE and the image has marked itself "OK" (the image_ok flag has been
     * set). This way a "revert" swap can be performed if it's necessary.
     */
    if (swap_type == BOOT_SWAP_TYPE_NONE) {
        rc = boot_update_security_counter(BOOT_PRIMARY_SLOT,
                                   boot_img_hdr(&boot_data, BOOT_PRIMARY_SLOT));
        if (rc != 0) {
            BOOT_LOG_ERR("Security counter update failed after image "
                         "validation.");
            goto out;
        }
    }

    /* Always boot from the primary slot. */
    rsp->br_flash_dev_id = boot_img_fa_device_id(&boot_data, BOOT_PRIMARY_SLOT);
    rsp->br_image_off = boot_img_slot_off(&boot_data, BOOT_PRIMARY_SLOT);
    rsp->br_hdr = boot_img_hdr(&boot_data, slot);

    /* Save boot status to shared memory area */
    rc = boot_save_boot_status(SW_S_NS,
                               rsp->br_hdr,
                               BOOT_IMG_AREA(&boot_data, slot));
    if (rc) {
        BOOT_LOG_ERR("Failed to add data to shared area");
    }

 out:
    flash_area_close(BOOT_SCRATCH_AREA(&boot_data));
    for (slot = 0; slot < BOOT_NUM_SLOTS; slot++) {
        flash_area_close(BOOT_IMG_AREA(&boot_data, BOOT_NUM_SLOTS - 1 - slot));
    }
    return rc;
}

#else /* MCUBOOT_NO_SWAP || MCUBOOT_RAM_LOADING */

#define BOOT_LOG_IMAGE_INFO(area, hdr, state)                               \
    BOOT_LOG_INF("Image %u: version=%u.%u.%u+%u, magic=%5s, image_ok=0x%x", \
                 (area),                                                    \
                 (hdr)->ih_ver.iv_major,                                    \
                 (hdr)->ih_ver.iv_minor,                                    \
                 (hdr)->ih_ver.iv_revision,                                 \
                 (hdr)->ih_ver.iv_build_num,                                \
                 ((state)->magic == BOOT_MAGIC_GOOD  ? "good"  :            \
                  (state)->magic == BOOT_MAGIC_UNSET ? "unset" :            \
                  "bad"),                                                   \
                 (state)->image_ok)

struct image_slot_version {
    uint64_t version;
    uint32_t slot_number;
};

/**
 * Extract the version number from the image header. This function must be
 * ported if version number format has changed in the image header.
 *
 * @param hdr  Pointer to an image header structure
 *
 * @return     Version number casted to uint64_t
 */
static uint64_t
boot_get_version_number(struct image_header *hdr)
{
    uint64_t version = 0;
    version |= (uint64_t)hdr->ih_ver.iv_major << (IMAGE_VER_MINOR_LENGTH
                                                + IMAGE_VER_REVISION_LENGTH
                                                + IMAGE_VER_BUILD_NUM_LENGTH);
    version |= (uint64_t)hdr->ih_ver.iv_minor << (IMAGE_VER_REVISION_LENGTH
                                                + IMAGE_VER_BUILD_NUM_LENGTH);
    version |= (uint64_t)hdr->ih_ver.iv_revision << IMAGE_VER_BUILD_NUM_LENGTH;
    version |= hdr->ih_ver.iv_build_num;
    return version;
}

/**
 * Comparator function for `qsort` to compare version numbers. This function
 * must be ported if version number format has changed in the image header.
 *
 * @param ver1  Pointer to an array element which holds the version number
 * @param ver2  Pointer to another array element which holds the version
 *              number
 *
 * @return      if version1 >  version2  -1
 *              if version1 == version2   0
 *              if version1 <  version2   1
 */
static int
boot_compare_version_numbers(const void *ver1, const void *ver2)
{
    if (((struct image_slot_version *)ver1)->version <
        ((struct image_slot_version *)ver2)->version) {
        return 1;
    }

    if (((struct image_slot_version *)ver1)->version ==
        ((struct image_slot_version *)ver2)->version) {
        return 0;
    }

    return -1;
}

/**
 * Sort the available images based on the version number and puts them in
 * a list.
 *
 * @param boot_sequence  A pointer to an array, whose aim is to carry
 *                       the boot order of candidate images.
 * @param slot_cnt       The number of flash areas, which can contains firmware
 *                       images.
 *
 * @return               The number of valid images.
 */
uint32_t
boot_get_boot_sequence(uint32_t *boot_sequence, uint32_t slot_cnt)
{
    struct boot_swap_state slot_state;
    struct image_header *hdr;
    struct image_slot_version image_versions[BOOT_NUM_SLOTS] = {{0}};
    uint32_t image_cnt = 0;
    uint32_t slot;
    int32_t rc;
    int32_t fa_id;

    for (slot = 0; slot < slot_cnt; slot++) {
        hdr = boot_img_hdr(&boot_data, slot);
        fa_id = flash_area_id_from_image_slot(slot);
        rc = boot_read_swap_state_by_id(fa_id, &slot_state);
        if (rc != 0) {
            BOOT_LOG_ERR("Error during reading image trailer from slot: %u",
                         slot);
            continue;
        }

        if (hdr->ih_magic == IMAGE_MAGIC) {
            if (slot_state.magic    == BOOT_MAGIC_GOOD ||
                slot_state.image_ok == BOOT_FLAG_SET) {
                /* Valid cases:
                 *  - Test mode:      magic is OK in image trailer
                 *  - Permanent mode: image_ok flag has previously set
                 */
                image_versions[slot].slot_number = slot;
                image_versions[slot].version = boot_get_version_number(hdr);
                image_cnt++;
            }

            BOOT_LOG_IMAGE_INFO(slot, hdr, &slot_state);
        } else {
            BOOT_LOG_INF("Image %u: No valid image", slot);
        }
    }

    /* Sort the images based on version number */
    qsort(&image_versions[0],
          slot_cnt,
          sizeof(struct image_slot_version),
          boot_compare_version_numbers);

    /* Copy the calculated boot sequence to boot_sequence array */
    for (slot = 0; slot < slot_cnt; slot++) {
        boot_sequence[slot] = image_versions[slot].slot_number;
    }

    return image_cnt;
}

#ifdef MCUBOOT_RAM_LOADING
/**
 * Copies an image from a slot in the flash to an SRAM address, where the load
 * address has already been inserted into the image header by this point and is
 * extracted from it within this method. The copying is done sector-by-sector.
 *
 * @param slot            The flash slot of the image to be copied to SRAM.
 *
 * @param hdr             Pointer to the image header structure of the image
 *                        that needs to be copid to SRAM
 *
 * @return                0 on success; nonzero on failure.
 */
static int
boot_copy_image_to_sram(int slot, struct image_header *hdr)
{
    int rc;
    uint32_t sect_sz;
    uint32_t sect = 0;
    uint32_t bytes_copied = 0;
    const struct flash_area *fap_src = NULL;
    uint32_t dst = (uint32_t) hdr->ih_load_addr;
    uint32_t img_sz;

    if (dst % 4 != 0) {
        BOOT_LOG_INF("Cannot copy the image to the SRAM address 0x%x "
        "- the load address must be aligned with 4 bytes due to SRAM "
        "restrictions", dst);
        return BOOT_EBADARGS;
    }

    rc = flash_area_open(flash_area_id_from_image_slot(slot), &fap_src);
    if (rc != 0) {
        return BOOT_EFLASH;
    }

    rc = boot_read_image_size(slot, hdr, &img_sz);
    if (rc != 0) {
        return BOOT_EFLASH;
    }

    while (bytes_copied < img_sz) {
        sect_sz = boot_img_sector_size(&boot_data, slot, sect);
        /*
         * Direct copy from where the image sector resides in flash to its new
         * location in SRAM
         */
        rc = flash_area_read(fap_src,
                             bytes_copied,
                             (void *)(dst + bytes_copied),
                             sect_sz);
        if (rc != 0) {
            BOOT_LOG_INF("Error whilst copying image from Flash to SRAM");
            break;
        } else {
            bytes_copied += sect_sz;
        }
        sect++;
    }

    if (fap_src) {
        flash_area_close(fap_src);
    }
    return rc;
}
#endif /* MCUBOOT_RAM_LOADING */

/**
 * Prepares the booting process. This function choose the newer image in flash
 * as appropriate, and returns the address to boot from.
 *
 * @param rsp                   On success, indicates how booting should occur.
 *
 * @return                      0 on success; nonzero on failure.
 */
int
boot_go(struct boot_rsp *rsp)
{
    size_t slot = 0;
    int32_t i;
    int rc;
    int fa_id;
    uint32_t boot_sequence[BOOT_NUM_SLOTS];
    uint32_t img_cnt;
    struct image_header *newest_image_header;

    static boot_sector_t primary_slot_sectors[BOOT_MAX_IMG_SECTORS];
    static boot_sector_t secondary_slot_sectors[BOOT_MAX_IMG_SECTORS];

    boot_data.imgs[BOOT_PRIMARY_SLOT].sectors = &primary_slot_sectors[0];
    boot_data.imgs[BOOT_SECONDARY_SLOT].sectors = &secondary_slot_sectors[0];

    /* Open boot_data image areas for the duration of this call. */
    for (i = 0; i < BOOT_NUM_SLOTS; i++) {
        fa_id = flash_area_id_from_image_slot(i);
        rc = flash_area_open(fa_id, &BOOT_IMG_AREA(&boot_data, i));
        assert(rc == 0);
    }

    /* Determine the sector layout of the image slots. */
    rc = boot_read_sectors();
    if (rc != 0) {
        BOOT_LOG_WRN("Failed reading sectors; BOOT_MAX_IMG_SECTORS=%d - too small?",
                BOOT_MAX_IMG_SECTORS);
        goto out;
    }

    /* Attempt to read an image header from each slot. */
    rc = boot_read_image_headers(false);
    if (rc != 0) {
        goto out;
    }

    img_cnt = boot_get_boot_sequence(boot_sequence, BOOT_NUM_SLOTS);
    if (img_cnt) {
        /* Authenticate images */
        for (i = 0; i < img_cnt; i++) {
            rc = boot_validate_slot(boot_sequence[i]);
            if (rc == 0) {
                slot = boot_sequence[i];
                break;
            }
        }
        if (rc) {
            /* If there was no valid image at all */
            rc = BOOT_EBADIMAGE;
            goto out;
        }

        /* The slot variable now refers to the newest image's slot in flash */
        newest_image_header = boot_img_hdr(&boot_data, slot);

        /* Update the security counter with the newest image's security
         * counter value.
         */
        rc = boot_update_security_counter(slot, newest_image_header);
        if (rc != 0) {
            BOOT_LOG_ERR("Security counter update failed after image "
                         "validation.");
            goto out;
        }

        #ifdef MCUBOOT_RAM_LOADING
        if (newest_image_header->ih_flags & IMAGE_F_RAM_LOAD) {
            /* Copy image to the load address from where it
             * currently resides in flash */
            rc = boot_copy_image_to_sram(slot, newest_image_header);
            if (rc != 0) {
                rc = BOOT_EBADIMAGE;
                BOOT_LOG_INF("Could not copy image from the %s slot in "
                             "the Flash to load address 0x%x in SRAM, "
                             "aborting..", (slot == BOOT_PRIMARY_SLOT) ?
                             "primary" : "secondary",
                             newest_image_header->ih_load_addr);
                goto out;
            } else {
                BOOT_LOG_INF("Image has been copied from the %s slot in "
                             "the flash to SRAM address 0x%x",
                             (slot == BOOT_PRIMARY_SLOT) ?
                             "primary" : "secondary",
                             newest_image_header->ih_load_addr);
            }

            /* Validate the image hash in SRAM after the copy was successful */
            rc = bootutil_check_hash_after_loading(newest_image_header);
            if (rc != 0) {
                rc = BOOT_EBADIMAGE;
                BOOT_LOG_INF("Cannot validate the hash of the image that was "
                             "copied to SRAM, aborting..");
                goto out;
            }

            BOOT_LOG_INF("Booting image from SRAM at address 0x%x",
                         newest_image_header->ih_load_addr);
        } else {
#endif /* MCUBOOT_RAM_LOADING */
            BOOT_LOG_INF("Booting image from the %s slot",
                         (slot == BOOT_PRIMARY_SLOT) ? "primary" : "secondary");
#ifdef MCUBOOT_RAM_LOADING
        }
#endif

        rsp->br_hdr = newest_image_header;
        rsp->br_image_off = boot_img_slot_off(&boot_data, slot);
        rsp->br_flash_dev_id = boot_img_fa_device_id(&boot_data, slot);
    } else {
        /* No candidate image available */
        rc = BOOT_EBADIMAGE;
        goto out;
    }

    /* Save boot status to shared memory area */
    rc = boot_save_boot_status(SW_S_NS,
                               rsp->br_hdr,
                               BOOT_IMG_AREA(&boot_data, slot));
    if (rc) {
        BOOT_LOG_ERR("Failed to add data to shared area");
    }

out:
   for (slot = 0; slot < BOOT_NUM_SLOTS; slot++) {
       flash_area_close(BOOT_IMG_AREA(&boot_data, BOOT_NUM_SLOTS - 1 - slot));
   }
   return rc;
}
#endif /* MCUBOOT_NO_SWAP || MCUBOOT_RAM_LOADING */
