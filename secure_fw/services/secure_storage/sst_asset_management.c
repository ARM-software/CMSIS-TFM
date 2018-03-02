/*
 * Copyright (c) 2017-2018, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stddef.h>
#include "tfm_secure_api.h"
#include "tfm_sst_defs.h"
#include "assets/sst_asset_defs.h"
#include "sst_asset_management.h"
#include "sst_core.h"
#include "sst_core_interface.h"
#include "sst_utils.h"

/******************************/
/* Asset management functions */
/******************************/

/* Policy database */
extern struct sst_asset_info_t asset_perms[];
extern struct sst_asset_perm_t asset_perms_modes[];

/**
 * \brief Looks up for policy entry for give app and uuid
 *
 * \param[in] db_entry  Asset specific entry
 * \param[in] app_id    Identify of the application calling the service
 *
 * \return Returns the perms entry on successful lookup
 */
static struct sst_asset_perm_t *sst_am_lookup_app_perms(
                                        const struct sst_asset_info_t *db_entry,
                                        uint32_t app_id)
{
    struct sst_asset_perm_t *perm_entry;
    uint32_t i;

    for (i = 0; i < db_entry->perms_count; i++) {
        perm_entry = &asset_perms_modes[db_entry->perms_modes_start_idx+i];
        if (perm_entry->app == app_id) {
            return perm_entry;
        }
    }

    return NULL;
}

/**
 * \brief Gets pointer to policy entry for an asset
 *
 * \param[in] uuid  Unique identifier of the object being accessed
 *
 * \return Returns the pointer for entry for specified asset
 */
static struct sst_asset_info_t *sst_am_lookup_db_entry(uint16_t uuid)
{
    uint32_t i;

    /* Lookup in db for matching entry */
    for (i = 0; i < SST_NUM_ASSETS; i++) {
        if (asset_perms[i].asset_uuid == uuid) {
            return &asset_perms[i];
        }
    }

    return NULL;
}

/**
 * \brief Checks the compile time policy for secure/non-secure separation
 *
 * \param[in] app_id        caller's application ID
 * \param[in] request_type  requested action to perform(
 *
 * \return Returns the sanitized request_type
 */
static uint16_t sst_am_check_s_ns_policy(uint32_t app_id, uint16_t request_type)
{
    enum tfm_sst_err_t err;
    uint16_t access;

    /* FIXME: based on level 1 tfm isolation, any entity on the secure side
     * can have full access if it uses secure app ID to make the call.
     * When the secure caller passes on the app_id of non-secure entity,
     * the code only allows read by reference. I.e. if the app_id
     * has the reference permission, the secure caller will be allowed
     * to read the entry. This needs a revisit when for higher level
     * of isolation.
     *
     * FIXME: current code allows only a referenced read, however there
     * is a case for refereced create/write/delete as well, for example
     * a NS entity may ask another secure service to derive a key and securely
     * store it, and make references for encryption/decryption and later on
     * delete it.
     * For now it is for the other secure service to create/delete/write
     * resources with the secure app ID.
     */
    err = sst_utils_validate_secure_caller();

    if (err == TFM_SST_ERR_SUCCESS) {
        if (app_id != S_APP_ID) {
            if (request_type & SST_PERM_READ) {
                access = SST_PERM_REFERENCE;
            } else {
                /* Other permissions can not be delegated */
                access = SST_PERM_FORBIDDEN;
            }
        } else {
            /* a call from secure entity on it's own behalf.
             * In level 1 isolation, any secure entity has
             * full access to storage.
             */
            access = SST_PERM_BYPASS;
        }
    } else if (app_id == S_APP_ID) {
        /* non secure caller spoofing as secure caller */
        access = SST_PERM_FORBIDDEN;
    } else {
        access = request_type;
    }
    return access;
}

/**
 * \brief Gets asset's permissions if the application is allowed
 *        based on the request_type
 *
 * \param[in] app_id        Caller's application ID
 * \param[in] uuid          Asset's unique identifier
 * \param[in] request_type  Type of requested access
 *
 * \note If request_type contains multiple permissions, this function
 *       returns the entry pointer for specified asset if at least one
 *       of those permissions match.
 *
 * \return Returns the entry pointer for specified asset
 */
static struct sst_asset_info_t *sst_am_get_db_entry(uint32_t app_id,
                                                    uint16_t uuid,
                                                    uint8_t request_type)
{
    struct sst_asset_perm_t *perm_entry;
    struct sst_asset_info_t *db_entry;

    request_type = sst_am_check_s_ns_policy(app_id, request_type);

    /* security access violation */
    if (request_type == SST_PERM_FORBIDDEN) {
        /* FIXME: this is prone to timing attacks. Ideally the time
         * spent in this function should always be constant irrespective
         * of success or failure of checks. Timing attacks will be
         * addressed in later version.
         */
        return NULL;
    }

    /* Find policy db entry for the the asset */
    db_entry = sst_am_lookup_db_entry(uuid);
    if (db_entry == NULL) {
        return NULL;
    }

    if (request_type == SST_PERM_BYPASS) {
         return db_entry;
     }

    /* Find the app ID entry in the database */
    perm_entry = sst_am_lookup_app_perms(db_entry, app_id);
    if (perm_entry == NULL) {
        return NULL;
    }

     /* Check if the db permission matches with at least one of the
      * requested permissions types.
      */
    if ((perm_entry->perm & request_type) != 0) {
        return db_entry;
    }
    return NULL;
}

/**
 * \brief Validates if the requested access is allowed
 *
 * \param[in] app_id        Caller's application ID
 * \param[in] asset_handle  Handle passed on by caller
 * \param[in] request_type  Type of requested access
 *
 * \return Returns the pointer for entry for specified asset
 */
static struct sst_asset_info_t *sst_am_get_db_entry_by_hdl(uint32_t app_id,
                                                          uint32_t asset_handle,
                                                          uint8_t request_type)
{
    uint16_t uuid;

    uuid = sst_utils_extract_uuid_from_handle(asset_handle);

    return sst_am_get_db_entry(app_id, uuid, request_type);
}

/**
 * \brief Validates the policy database's integrity
 *        Stub function.
 *
 * \return Returns value specified in \ref tfm_sst_err_t
 */
static enum tfm_sst_err_t validate_policy_db(void)
{
    /* Currently the policy database is inbuilt
     * in the code. It's sanity is assumed to be correct.
     * In the later revisions if access policy is
     * stored differently, it may require sanity check
     * as well.
     */
    return TFM_SST_ERR_SUCCESS;
}

enum tfm_sst_err_t sst_am_prepare(void)
{
    enum tfm_sst_err_t err;
    /* FIXME: outcome of this function should determine
     * state machine of asset manager. If this
     * step fails other APIs shouldn't entertain
     * any user calls. Not a major issue for now
     * as policy db check is a dummy function, and
     * sst core maintains it's own state machine.
     */

    /* Validate policy database */
    err = validate_policy_db();

    /* Initialize underlying storage system */
    if (err != TFM_SST_ERR_SUCCESS) {
        return TFM_SST_ERR_SYSTEM_ERROR;
    }
    err = sst_object_prepare();
#ifdef SST_RAM_FS
    /* in case of RAM based system there wouldn't be
     * any content in the boot time. Call the wipe API
     * to create a storage structure.
     */
    if (err != TFM_SST_ERR_SUCCESS) {
        sst_object_wipe_all();
        /* attempt to initialise again */
        err = sst_object_prepare();
    }
#endif

    return err;
}

/**
 * \brief Validate incoming iovec structure
 *
 * \param[in] src     Incoming iovec for the read/write request
 * \param[in] dest    Pointer to local copy of the iovec
 * \param[in] app_id  Application ID of the caller
 * \param[in] access  Access type to be permormed on the given dest->data
 *                    address
 *
 * \return Returns value specified in \ref tfm_sst_err_t
 */
static enum tfm_sst_err_t validate_copy_validate_iovec(
                                                const struct tfm_sst_buf_t *src,
                                                struct tfm_sst_buf_t *dest,
                                                uint32_t app_id,
                                                uint32_t access)
{
    /* iovec struct needs to be used as veneers do not allow
     * more than four params.
     * First validate the pointer for iovec itself, then copy
     * the iovec, then validate the local copy of iovec.
     */
    enum tfm_sst_err_t bound_check;

    bound_check = sst_utils_bound_check_and_copy((uint8_t *) src,
                      (uint8_t *) dest, sizeof(struct tfm_sst_buf_t), app_id);
    if (bound_check == TFM_SST_ERR_SUCCESS) {
        bound_check = sst_utils_memory_bound_check(dest->data, dest->size,
                                                   app_id, access);
    }

    return bound_check;
}

enum tfm_sst_err_t sst_am_get_handle(uint32_t app_id, uint16_t asset_uuid,
                                     uint32_t *hdl)
{
    struct sst_asset_info_t *db_entry;
    uint8_t all_perms = SST_PERM_REFERENCE | SST_PERM_READ | SST_PERM_WRITE;
    enum tfm_sst_err_t err;
    /* Lower layers trust the incoming request, use a local pointer */
    uint32_t temp_hdl;

    /* Check if application has access to the asset */
    db_entry = sst_am_get_db_entry(app_id, asset_uuid, all_perms);
    if (db_entry == NULL) {
        return TFM_SST_ERR_ASSET_NOT_FOUND;
    }

    /* Check handle pointer value */
    err = sst_utils_memory_bound_check(hdl, sizeof(uint32_t),
                                       app_id, TFM_MEMORY_ACCESS_RW);
    if (err != TFM_SST_ERR_SUCCESS) {
        return TFM_SST_ERR_PARAM_ERROR;
    }

    /* FIXME: the handle is composed of UUID and metadata table index,
     * which means leaking info about where a certain object may be stored.
     * While this is okay in the current implementation as the metadata
     * block layout is quite fixed. However, in later designs if
     * different partitions are used for storing for different security groups
     * (e.g. chip manufacture data, device manufacture data, user data), the
     * threat model may require not leaking any info about where an object
     * may be stored.
     * In such a scenario the handle can
     * be encrypted before passing on to the caller.
     * Other option could be to allocate a handle in RAM and provide a pointer
     * to caller as handle. However, the design attemts to avoid
     * maintaining any kind of transient state for robustness.
     */
    err = sst_object_handle(asset_uuid, &temp_hdl);
    if (err != TFM_SST_ERR_SUCCESS) {
        return TFM_SST_ERR_ASSET_NOT_FOUND;
    }

    /* memcpy instead of direct *hdl = temp_hdl.
     * ensures malicious entities can't trigger a misaligned access fault.
     */
    sst_utils_memcpy(hdl, &temp_hdl, sizeof(uint32_t));
    return TFM_SST_ERR_SUCCESS;
}

enum tfm_sst_err_t sst_am_get_attributes(uint32_t app_id,
                                         uint32_t asset_handle,
                                         struct tfm_sst_attribs_t *attrib)
{
    enum tfm_sst_err_t bound_check;
    struct sst_asset_info_t *db_entry;
    struct tfm_sst_attribs_t tmp_attrib;
    enum tfm_sst_err_t err;
    uint8_t all_perms = SST_PERM_REFERENCE | SST_PERM_READ | SST_PERM_WRITE;

    bound_check = sst_utils_memory_bound_check(attrib,
                                               sizeof(struct tfm_sst_attribs_t),
                                               app_id, TFM_MEMORY_ACCESS_RW);
    if (bound_check != TFM_SST_ERR_SUCCESS) {
        return TFM_SST_ERR_PARAM_ERROR;
    }

    db_entry = sst_am_get_db_entry_by_hdl(app_id, asset_handle, all_perms);
    if (db_entry == NULL) {
        return TFM_SST_ERR_ASSET_NOT_FOUND;
    }

    err = sst_object_get_attributes(asset_handle, &tmp_attrib);
    if (err == TFM_SST_ERR_SUCCESS) {
        /* memcpy instead of direct *hdl = temp_hdl.
         * ensures malicious entities can't trigger
         * a misaligned access fault.
         */
        sst_utils_memcpy(attrib, &tmp_attrib, sizeof(struct tfm_sst_attribs_t));
    }

    return err;
}

enum tfm_sst_err_t sst_am_create(uint32_t app_id, uint16_t asset_uuid)
{
    enum tfm_sst_err_t err;
    struct sst_asset_info_t *db_entry;

    db_entry = sst_am_get_db_entry(app_id, asset_uuid, SST_PERM_WRITE);
    if (db_entry == NULL) {
        return TFM_SST_ERR_ASSET_NOT_FOUND;
    }

    err = sst_object_create(asset_uuid, db_entry->max_size);
    return err;
}

enum tfm_sst_err_t sst_am_read(uint32_t app_id, uint32_t asset_handle,
                               struct tfm_sst_buf_t *data)
{
    struct tfm_sst_buf_t local_data;
    enum tfm_sst_err_t err;
    struct sst_asset_info_t *db_entry;

    /* Check application ID permissions */
    db_entry = sst_am_get_db_entry_by_hdl(app_id, asset_handle, SST_PERM_READ);
    if (db_entry == NULL) {
        return TFM_SST_ERR_ASSET_NOT_FOUND;
    }

    /* Make a local copy of the iovec data structure */
    err = validate_copy_validate_iovec(data, &local_data,
                                       app_id, TFM_MEMORY_ACCESS_RW);
    if (err != TFM_SST_ERR_SUCCESS) {
        return TFM_SST_ERR_ASSET_NOT_FOUND;
    }

    err = sst_object_read(asset_handle, local_data.data,
                          local_data.offset, local_data.size);
    return err;
}

enum tfm_sst_err_t sst_am_write(uint32_t app_id, uint32_t asset_handle,
                                const struct tfm_sst_buf_t *data)
{
    struct tfm_sst_buf_t local_data;
    enum tfm_sst_err_t err;
    struct sst_asset_info_t *db_entry;

    /* Check application ID permissions */
    db_entry = sst_am_get_db_entry_by_hdl(app_id, asset_handle, SST_PERM_WRITE);
    if (db_entry == NULL) {
        return TFM_SST_ERR_ASSET_NOT_FOUND;
    }

    /* Make a local copy of the iovec data structure */
    err = validate_copy_validate_iovec(data, &local_data,
                                       app_id, TFM_MEMORY_ACCESS_RO);
    if (err != TFM_SST_ERR_SUCCESS) {
        return TFM_SST_ERR_ASSET_NOT_FOUND;
    }

    /* Boundary check the incoming request */
    err = sst_utils_check_contained_in(0, db_entry->max_size,
                                       local_data.offset, local_data.size);

    if (err == TFM_SST_ERR_SUCCESS) {
        err = sst_object_write(asset_handle, local_data.data,
                               local_data.offset, local_data.size);
    }
    return err;
}

enum tfm_sst_err_t sst_am_delete(uint32_t app_id, uint32_t asset_handle)
{
    enum tfm_sst_err_t err;
    struct sst_asset_info_t *db_entry;

    db_entry = sst_am_get_db_entry_by_hdl(app_id, asset_handle, SST_PERM_WRITE);
    if (db_entry == NULL) {
        return TFM_SST_ERR_ASSET_NOT_FOUND;
    }

    err = sst_object_delete(asset_handle);
    return err;
}
