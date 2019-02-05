/*
 * Copyright (c) 2018-2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __SST_OBJECT_TABLE_H__
#define __SST_OBJECT_TABLE_H__

#include <stdint.h>

#include "psa_protected_storage.h"
#include "tfm_sst_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \struct sst_obj_table_info_t
 *
 * \brief Object table information structure.
 */
struct sst_obj_table_info_t {
    uint32_t fid;      /*!< File ID in the file system */
#ifdef SST_ENCRYPTION
    uint8_t *tag;      /*!< Pointer to the MAC value of AEAD object */
#else
    uint32_t version;  /*!< Object version */
#endif
};

/**
 * \brief Creates object table.
 *
 * \return Returns error code as specified in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t sst_object_table_create(void);

/**
 * \brief Initializes object table.
 *
 * \param[in/out] obj_data  Pointer to the static object data allocated
 *                          in other to reuse that memory to allocated a
 *                          temporary object table.
 *
 * \return Returns error code as specified in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t sst_object_table_init(uint8_t *obj_data);

/**
 * \brief Checks if there is an entry in the table for the provided UID and
 *        client ID pair.
 *
 * \param[in] uid        Identifier for the data
 * \param[in] client_id  Identifier of the asset’s owner (client)
 *
 * \return Returns error code as specified in \ref tfm_sst_err_t
 *
 * \retval TFM_SST_ERR_SUCCESS        If there is a table entry for the object
 * \retval TFM_SST_ERR_UID_NOT_FOUND  If no table entry exists for the object
 */
enum tfm_sst_err_t sst_object_table_obj_exist(psa_ps_uid_t uid,
                                              int32_t client_id);

/**
 * \brief Gets a not in use file ID.
 *
 * \param[out] p_fid  Pointer to the location to store the file ID
 *
 * \return Returns TFM_SST_ERR_SUCCESS if the fid is valid. Otherwise, it
 *         returns an error code as specified in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t sst_object_table_get_free_fid(uint32_t *p_fid);

/**
 * \brief Sets object table information in the object table and stores it
 *        persistently, for the provided UID and client ID pair.
 *
 * \param[in] uid           Identifier for the data.
 * \param[in] client_id     Identifier of the asset’s owner (client)
 * \param[in] obj_tbl_info  Pointer to the location to store object table
 *                          information \ref sst_obj_table_info_t
 *
 * \note  A call to this function results in writing the table to the
 *        file system.
 *
 * \return Returns error code as specified in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t sst_object_table_set_obj_tbl_info(psa_ps_uid_t uid,
                                                     int32_t client_id,
                               const struct sst_obj_table_info_t *obj_tbl_info);

/**
 * \brief Gets object table information from the object table for the provided
 *        UID and client ID pair.
 *
 * \param[in]  uid           Identifier for the data.
 * \param[in]  client_id     Identifier of the asset’s owner (client)
 * \param[out] obj_tbl_info  Pointer to the location to store object table
 *                           information
 *
 * \return Returns TFM_SST_ERR_SUCCESS if the object exists. Otherwise, it
 *         returns TFM_SST_ERR_UID_NOT_FOUND.
 */
enum tfm_sst_err_t sst_object_table_get_obj_tbl_info(psa_ps_uid_t uid,
                                                     int32_t client_id,
                                     struct sst_obj_table_info_t *obj_tbl_info);

/**
 * \brief Deletes the table entry for the provided UID and client ID pair.
 *
 * \param[in]  uid        Identifier for the data.
 * \param[in]  client_id  Identifier of the asset’s owner (client)
 *
 * \return Returns error code as specified in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t sst_object_table_delete_object(psa_ps_uid_t uid,
                                                  int32_t client_id);

/**
 * \brief Deletes old object table from the persistent area.
 *
 * \return Returns error code as specified in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t sst_object_table_delete_old_table(void);

#ifdef __cplusplus
}
#endif

#endif /* __SST_OBJECT_TABLE_H__ */
