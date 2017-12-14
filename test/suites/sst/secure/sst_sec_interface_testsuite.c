/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "sst_tests.h"

#include <stdio.h>
#include <string.h>

#include "test/framework/helpers.h"
#include "secure_fw/services/secure_storage/assets/sst_asset_defs.h"
#include "secure_fw/services/secure_storage/sst_core_interface.h"
#include "tfm_sst_veneers.h"
#include "s_test_helpers.h"

/* Test suite defines */
#define INVALID_APP_ID         0xFFFFFFFF
#define INVALID_ASSET_ID           0xFFFF
#define READ_BUF_SIZE                12UL
#define WRITE_BUF_SIZE                5UL

/* Memory bounds to check */
#define ROM_ADDR_LOCATION        0x10000000
#define DEV_ADDR_LOCATION        0x20000000
#define NON_EXIST_ADDR_LOCATION  0xFFFFFFFF

/* Test data sized to fill the SHA224 asset */
#define READ_DATA_SHA224    "XXXXXXXXXXXXXXXXXXXXXXXXXXXX"
#define WRITE_DATA_SHA224_1 "TEST_DATA_ONE_TWO_THREE_FOUR"
#define WRITE_DATA_SHA224_2 "(ABCDEFGHIJKLMNOPQRSTUVWXYZ)"
#define BUF_SIZE_SHA224     (SST_ASSET_MAX_SIZE_SHA224_HASH + 1)

/* Define test suite for asset manager tests */
/* List of tests */
static void tfm_sst_test_2001(struct test_result_t *ret);
static void tfm_sst_test_2002(struct test_result_t *ret);
static void tfm_sst_test_2003(struct test_result_t *ret);
static void tfm_sst_test_2004(struct test_result_t *ret);
static void tfm_sst_test_2005(struct test_result_t *ret);
static void tfm_sst_test_2006(struct test_result_t *ret);
static void tfm_sst_test_2007(struct test_result_t *ret);
static void tfm_sst_test_2008(struct test_result_t *ret);
static void tfm_sst_test_2009(struct test_result_t *ret);
static void tfm_sst_test_2010(struct test_result_t *ret);
static void tfm_sst_test_2011(struct test_result_t *ret);
static void tfm_sst_test_2012(struct test_result_t *ret);
static void tfm_sst_test_2013(struct test_result_t *ret);

static struct test_t write_tests[] = {
    {&tfm_sst_test_2001, "TFM_SST_TEST_2001",
     "Create interface", {0} },
    {&tfm_sst_test_2002, "TFM_SST_TEST_2002",
     "Get handle interface", {0} },
    {&tfm_sst_test_2003, "TFM_SST_TEST_2003",
     "Get attributes interface", {0} },
    {&tfm_sst_test_2004, "TFM_SST_TEST_2004",
     "Write interface", {0} },
    {&tfm_sst_test_2005, "TFM_SST_TEST_2005",
     "Read interface", {0} },
    {&tfm_sst_test_2006, "TFM_SST_TEST_2006",
     "Delete interface", {0} },
    {&tfm_sst_test_2007, "TFM_SST_TEST_2007",
     "Write and partial reads", {0} },
    {&tfm_sst_test_2008, "TFM_SST_TEST_2008",
     "Write partial data in an asset and reload secure storage area", {0} },
    {&tfm_sst_test_2009, "TFM_SST_TEST_2009",
     "Write more data than asset max size", {0} },
    {&tfm_sst_test_2010, "TFM_SST_TEST_2010",
     "Appending data to an asset", {0} },
    {&tfm_sst_test_2011, "TFM_SST_TEST_2011",
     "Appending data to an asset until eof", {0} },
    {&tfm_sst_test_2012, "TFM_SST_TEST_2012",
     "Write data to two assets alternately", {0} },
    {&tfm_sst_test_2013, "TFM_SST_TEST_2013",
     "Write and read data from illegal locations", {0} },
};

void register_testsuite_s_sst_sec_interface(struct test_suite_t *p_test_suite)
{
    uint32_t list_size = (sizeof(write_tests) / sizeof(write_tests[0]));

    set_testsuite("SST secure interface tests (TFM_SST_TEST_2XXX)",
                  write_tests, list_size, p_test_suite);
}

/**
 * \brief Tests create function against:
 * - Valid application ID and asset ID
 * - Invalid asset ID
 * - Invalid application ID
 */
static void tfm_sst_test_2001(struct test_result_t *ret)
{
    const uint32_t app_id = S_APP_ID;
    const uint16_t asset_uuid = SST_ASSET_ID_X509_CERT_LARGE;
    enum tfm_sst_err_t err;

    /* Prepares test context */
    if (prepare_test_ctx(ret) != 0) {
        return;
    }

    /* Checks write permissions in create function */
    err = tfm_sst_veneer_create(app_id, asset_uuid);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Create should not fail for application S_APP_ID");
        return;
    }

    /* Calls create with invalid asset ID */
    err = tfm_sst_veneer_create(app_id, INVALID_ASSET_ID);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("Create should fail for invalid ASSET ID");
        return;
    }

    /* Calls create with invalid application ID */
    err = tfm_sst_veneer_create(INVALID_APP_ID, asset_uuid);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("Create should fail for invalid application ID");
        return;
    }

    ret->val = TEST_PASSED;
}

/**
 * \brief Tests get handle function against:
 * - Valid asset ID and not created file
 * - Valid asset ID and created file
 * - Invalid asset ID
 */
static void tfm_sst_test_2002(struct test_result_t *ret)
{
    const uint32_t app_id = S_APP_ID;
    const uint16_t asset_uuid = SST_ASSET_ID_X509_CERT_LARGE;
    enum tfm_sst_err_t err;
    uint32_t hdl;

    /* Prepares test context */
    if (prepare_test_ctx(ret) != 0) {
        return;
    }

    /* Calls get handle before create the asset */
    err = tfm_sst_veneer_get_handle(app_id, asset_uuid, &hdl);
    if (err == TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should fail as the file is not created");
        return;
    }

    /* Creates asset to get a valid handle */
    err = tfm_sst_veneer_create(app_id, asset_uuid);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Create should not fail for application S_APP_ID");
        return;
    }

    /* Resets handle before read the new one */
    hdl = 0;

    /* Gets asset's handle */
    err = tfm_sst_veneer_get_handle(app_id, asset_uuid, &hdl);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should return a valid asset handle");
        return;
    }

    /* Calls get handle with invalid app ID */
    err = tfm_sst_veneer_get_handle(INVALID_APP_ID, asset_uuid, &hdl);
    if (err == TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should fail as application ID is invalid");
        return;
    }

    /* Calls get handle with invalid asset ID */
    err = tfm_sst_veneer_get_handle(app_id, INVALID_ASSET_ID, &hdl);
    if (err == TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should fail as asset hanlder is invalid");
        return;
    }

    /* Calls get handle with invalid handle pointer */
    err = tfm_sst_veneer_get_handle(app_id, asset_uuid, NULL);
    if (err == TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should fail as asset hanlder pointer is invalid");
        return;
    }

    ret->val = TEST_PASSED;
}

/**
 * \brief Tests get attributes function against:
 * - Valid application ID, asset handle and attributes struct pointer
 * - Invalid application ID
 * - Invalid asset handle
 * - Invalid attributes struct pointer
 */
static void tfm_sst_test_2003(struct test_result_t *ret)
{
    const uint32_t app_id = S_APP_ID;
    const uint16_t asset_uuid = SST_ASSET_ID_X509_CERT_LARGE;
    struct tfm_sst_attribs_t asset_attrs;
    enum tfm_sst_err_t err;
    uint32_t hdl;

    /* Prepares test context */
    if (prepare_test_ctx(ret) != 0) {
        return;
    }

    /* Creates asset to get a valid handle */
    err = tfm_sst_veneer_create(app_id, asset_uuid);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Create should not fail for application S_APP_ID");
        return;
    }

    /* Gets asset's handle */
    err = tfm_sst_veneer_get_handle(app_id, asset_uuid, &hdl);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should return a valid asset handle");
        return;
    }

    /* Calls get_attributes with valid application ID, asset handle and
     * attributes struct pointer
     */
    err = tfm_sst_veneer_get_attributes(app_id, hdl, &asset_attrs);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Application S_APP_ID should be able to read the "
                  "attributes of this file");
        return;
    }

    /* Checks attributes */
    if (asset_attrs.size_current != 0) {
        TEST_FAIL("Asset current size should be 0 as it is only created");
        return;
    }

    if (asset_attrs.size_max != SST_ASSET_MAX_SIZE_X509_CERT_LARGE) {
        TEST_FAIL("Max size of the asset is incorrect");
        return;
    }

    /* Calls get_attributes with invalid application ID */
    err = tfm_sst_veneer_get_attributes(INVALID_APP_ID, hdl, &asset_attrs);
    if (err == TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get attributes function should fail for an invalid "
                  "application ID");
        return;
    }

    /* Calls get_attributes with invalid asset handle */
    err = tfm_sst_veneer_get_attributes(app_id, 0, &asset_attrs);
    if (err == TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get attributes function should fail for an invalid "
                  "asset handle");
        return;
    }

    /* Calls get_attributes with invalid struct attributes pointer */
    err = tfm_sst_veneer_get_attributes(app_id, hdl, NULL);
    if (err != TFM_SST_ERR_PARAM_ERROR) {
        TEST_FAIL("Get attributes function should fail for an invalid "
                  "struct attributes pointer");
        return;
    }

    ret->val = TEST_PASSED;
}

/**
 * \brief Tests write function against:
 * - Valid application ID, asset handle and data pointer
 * - Invalid application ID
 * - Invalid asset handle
 * - NULL pointer as write buffer
 * - Offset + write data size larger than max asset size
 */
static void tfm_sst_test_2004(struct test_result_t *ret)
{
    const uint32_t app_id = S_APP_ID;
    const uint16_t asset_uuid = SST_ASSET_ID_X509_CERT_LARGE;
    struct tfm_sst_attribs_t asset_attrs;
    enum tfm_sst_err_t err;
    struct tfm_sst_buf_t io_data;
    uint32_t hdl;
    uint8_t wrt_data[WRITE_BUF_SIZE] = "DATA";

    /* Prepares test context */
    if (prepare_test_ctx(ret) != 0) {
        return;
    }

    /* Creates asset to get a valid handle */
    err = tfm_sst_veneer_create(app_id, asset_uuid);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Create should not fail for application S_APP_ID");
        return;
    }

    /* Gets asset's handle */
    err = tfm_sst_veneer_get_handle(app_id, asset_uuid, &hdl);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should return a valid asset handle");
        return;
    }

    /* Sets data structure */
    io_data.data = wrt_data;
    io_data.size = WRITE_BUF_SIZE;
    io_data.offset = 0;

    /* Write data in the asset */
    err = tfm_sst_veneer_write(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Write should works correctly");
        return;
    }

    /* Calls get_attributes with valid application ID, asset handle and
     * attributes struct pointer
     */
    err = tfm_sst_veneer_get_attributes(app_id, hdl, &asset_attrs);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Application S_APP_ID should be able to read the "
                  "attributes of this file");
        return;
    }

    /* Checks attributes */
    if (asset_attrs.size_current != WRITE_BUF_SIZE) {
        TEST_FAIL("Asset current size should be size of the write data");
        return;
    }

    /* Calls write function with invalid application ID */
    err = tfm_sst_veneer_write(INVALID_APP_ID, hdl, &io_data);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("Invalid application ID should not write in the file");
        return;
    }

    /* Calls write function with invalid asset handle */
    err = tfm_sst_veneer_write(app_id, 0, &io_data);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("Invalid asset handle should not write in the file");
        return;
    }

    /* Calls write function with invalid asset handle */
    err = tfm_sst_veneer_write(app_id, hdl, NULL);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("NULL data pointer should make the write fail");
        return;
    }

    /* Sets data structure */
    io_data.data = wrt_data;
    io_data.size = 1;
    io_data.offset = SST_ASSET_MAX_SIZE_X509_CERT_LARGE;

    /* Calls write function with offset + write data size larger than
     * max asset size
     */
    err = tfm_sst_veneer_write(app_id, hdl, &io_data);
    if (err == TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Offset + write data size larger than max asset size "
                  "should make the write fail");
        return;
    }

    ret->val = TEST_PASSED;
}

/**
 * \brief Tests read function against:
 * - Valid application ID, asset handle and data pointer
 * - Invalid application ID
 * - Invalid asset handle
 * - NULL pointer as write buffer
 * - Offset + read data size larger than current asset size
 */
static void tfm_sst_test_2005(struct test_result_t *ret)
{
    const uint32_t app_id = S_APP_ID;
    const uint16_t asset_uuid = SST_ASSET_ID_X509_CERT_LARGE;
    enum tfm_sst_err_t err;
    struct tfm_sst_buf_t io_data;
    struct tfm_sst_attribs_t asset_attrs;
    uint32_t hdl;
    uint8_t wrt_data[WRITE_BUF_SIZE] = "DATA";
    uint8_t read_data[READ_BUF_SIZE] = "XXXXXXXXXXX";

    /* Prepares test context */
    if (prepare_test_ctx(ret) != 0) {
        return;
    }

    /* Creates asset to get a valid handle */
    err = tfm_sst_veneer_create(app_id, asset_uuid);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Create should not fail for application S_APP_ID");
        return;
    }

    /* Gets asset's handle */
    err = tfm_sst_veneer_get_handle(app_id, asset_uuid, &hdl);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should return a valid asset handle");
        return;
    }

    /* Sets data structure */
    io_data.data = wrt_data;
    io_data.size = WRITE_BUF_SIZE;
    io_data.offset = 0;

    /* Write data in the asset */
    err = tfm_sst_veneer_write(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Write should works correctly");
        return;
    }

    /* Sets data structure for read*/
    io_data.data = read_data+3;
    io_data.size = WRITE_BUF_SIZE;
    io_data.offset = 0;

    /* Read data from the asset */
    err = tfm_sst_veneer_read(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Read should works correctly");
        return;
    }

    if (memcmp(read_data, "XXX", 3) != 0) {
        TEST_FAIL("Read buffer contains illegal pre-data");
        return;
    }

    if (memcmp((read_data+3), wrt_data, WRITE_BUF_SIZE) != 0) {
        TEST_FAIL("Read buffer has read incorrect data");
        return;
    }

    if (memcmp((read_data+8), "XXX", 3) != 0) {
        TEST_FAIL("Read buffer contains illegal post-data");
        return;
    }

    /* Calls read with invalid application ID */
    err = tfm_sst_veneer_read(INVALID_APP_ID, hdl, &io_data);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("Read should fail when read is called with an invalid "
                  "application ID");
        return;
    }

    /* Calls read with invalid asset handle */
    err = tfm_sst_veneer_read(app_id, 0, &io_data);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("Read should fail when read is called with an invalid "
                  "asset handle");
        return;
    }

    /* Calls read with invalid asset handle */
    err = tfm_sst_veneer_read(app_id, hdl, NULL);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("Read should fail when read is called with an invalid "
                  "data pointer");
        return;
    }

    /* Gets current asset attributes */
    err = tfm_sst_veneer_get_attributes(app_id, hdl, &asset_attrs);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Application S_APP_ID should be able to read the "
                  "attributes of this file");
        return;
    }

    /* Checks attributes */
    if (asset_attrs.size_current == 0) {
        TEST_FAIL("Asset current size should be bigger than 0");
        return;
    }

    /* Sets data structure */
    io_data.data = read_data;
    io_data.size = 1;
    io_data.offset = asset_attrs.size_current;

    /* Calls write function with offset + read data size larger than current
     * asset size
     */
    err = tfm_sst_veneer_read(app_id, hdl, &io_data);
    if (err == TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Offset + read data size larger than current asset size");
        return;
    }

    ret->val = TEST_PASSED;
}

/**
 * \brief Tests delete function against:
 * - Valid application ID and asset handle
 * - Invalid application ID
 * - Invalid asset handle
 * - Remove first asset in the data block and check if
 *   next asset's data is compacted correctly.
 */
static void tfm_sst_test_2006(struct test_result_t *ret)
{
    const uint32_t app_id_1 = S_APP_ID;
    const uint32_t app_id_2 = S_APP_ID;
    const uint16_t asset_uuid_1 = SST_ASSET_ID_SHA224_HASH;
    const uint16_t asset_uuid_2 = SST_ASSET_ID_SHA384_HASH;
    struct tfm_sst_buf_t io_data;
    enum tfm_sst_err_t err;
    uint32_t hdl_1;
    uint32_t hdl_2;
    uint8_t read_data[BUF_SIZE_SHA224] = READ_DATA_SHA224;
    uint8_t wrt_data[BUF_SIZE_SHA224] = WRITE_DATA_SHA224_1;

    /* Prepares test context */
    if (prepare_test_ctx(ret) != 0) {
        return;
    }

    /* Creates assset to get a valid handle */
    err = tfm_sst_veneer_create(app_id_1, asset_uuid_1);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Create should not fail for application S_APP_ID");
        return;
    }

    /* Gets asset's handle */
    err = tfm_sst_veneer_get_handle(app_id_1, asset_uuid_1, &hdl_1);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should return a valid asset handle");
        return;
    }

    /* Calls delete asset with invalid application ID */
    err = tfm_sst_veneer_delete(INVALID_APP_ID, hdl_1);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("The delete action should fail if an invalid application "
                  "ID is provided");
        return;
    }

    /* Calls delete asset */
    err = tfm_sst_veneer_delete(app_id_1, hdl_1);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("The delete action should work correctly");
        return;
    }

    /* Calls delete with a deleted asset handle */
    err = tfm_sst_veneer_delete(app_id_1, hdl_1);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("The delete action should fail as handle is not valid");
        return;
    }

    /* Calls delete asset with invalid asset handle */
    err = tfm_sst_veneer_delete(app_id_1, 0);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("The delete action should fail if an invalid asset handle "
                  "is provided");
        return;
    }

    /***** Test data block compact feature *****/
    /* Create asset 2 to locate it at the beginning of the block. Then,
     * create asset 1 to be located after asset 2. Write data on asset
     * 1 and remove asset 2. If delete works correctly, when the code
     * reads back the asset 1 data, the data must be correct.
     */

    /* Creates assset 2 first to locate it at the beginning of the
     * data block
     */
    err = tfm_sst_veneer_create(app_id_2, asset_uuid_2);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Create should not fail for application S_APP_ID");
        return;
    }

    /* Gets asset 2 handle */
    err = tfm_sst_veneer_get_handle(app_id_2, asset_uuid_2, &hdl_2);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should return a valid asset handle");
        return;
    }

    /* Creates asset 1 to locate it after the asset 2 in the data block */
    err = tfm_sst_veneer_create(app_id_1, asset_uuid_1);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Create should not fail for application S_APP_ID");
        return;
    }

    /* Gets asset 1 handle */
    err = tfm_sst_veneer_get_handle(app_id_1, asset_uuid_1, &hdl_1);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should return a valid asset handle");
        return;
    }

    /* Sets data structure */
    io_data.data = wrt_data;
    io_data.size = SST_ASSET_MAX_SIZE_SHA224_HASH;
    io_data.offset = 0;

    /* Write data in asset 1 */
    err = tfm_sst_veneer_write(app_id_1, hdl_1, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Write data should work for application S_APP_ID");
        return;
    }

    /* Deletes asset 2. It means that after the delete call, asset 1 should be
     * at the beginning of the block.
     */
    err = tfm_sst_veneer_delete(app_id_2, hdl_2);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("The delete action should work correctly");
        return;
    }

    /* If compact works as expected, the code should be able to read back
     * correctly the data from asset 1
     */

    /* Sets data structure */
    io_data.data = read_data;
    io_data.size = SST_ASSET_MAX_SIZE_SHA224_HASH;
    io_data.offset = 0;

    /* Read back the asset 1 */
    err = tfm_sst_veneer_read(app_id_1, hdl_1, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Incorrect number of bytes read back");
        return;
    }

    if (memcmp(read_data, wrt_data, BUF_SIZE_SHA224) != 0) {
        TEST_FAIL("Read buffer has incorrect data");
        return;
    }

    ret->val = TEST_PASSED;
}

/**
 * \brief Tests write and partial reads.
 */
static void tfm_sst_test_2007(struct test_result_t *ret)
{
    const uint32_t app_id = S_APP_ID;
    const uint16_t asset_uuid = SST_ASSET_ID_X509_CERT_LARGE;
    enum tfm_sst_err_t err;
    struct tfm_sst_buf_t io_data;
    uint32_t hdl;
    uint32_t i;
    uint8_t read_data[READ_BUF_SIZE] = "XXXXXXXXXXX";
    uint8_t wrt_data[WRITE_BUF_SIZE] = "DATA";

    /* Prepares test context */
    if (prepare_test_ctx(ret) != 0) {
        return;
    }

    /* Creates asset to get a valid handle */
    err = tfm_sst_veneer_create(app_id, asset_uuid);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Create should not fail for application S_APP_ID");
        return;
    }

    /* Gets asset's handle */
    err = tfm_sst_veneer_get_handle(app_id, asset_uuid, &hdl);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should return a valid asset handle");
        return;
    }

    /* Sets data structure */
    io_data.data = wrt_data;
    io_data.size = WRITE_BUF_SIZE;
    io_data.offset = 0;

    /* Write data in the asset */
    err = tfm_sst_veneer_write(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Write should works correctly");
        return;
    }

    /* Sets data structure for read*/
    io_data.data = (read_data + 3);
    io_data.size = 1;
    io_data.offset = 0;

    for (i = 0; i < WRITE_BUF_SIZE; i++) {
        /* Read data from the asset */
        err = tfm_sst_veneer_read(app_id, hdl, &io_data);
        if (err != TFM_SST_ERR_SUCCESS) {
            TEST_FAIL("Read should works correctly");
            return;
        }

        /* Increases data pointer and offset */
        io_data.data++;
        io_data.offset++;
    }

    if (memcmp(read_data, "XXX", 3) != 0) {
        TEST_FAIL("Read buffer contains illegal pre-data");
        return;
    }

    if (memcmp((read_data + 3), wrt_data, WRITE_BUF_SIZE) != 0) {
        TEST_FAIL("Read buffer has read incorrect data");
        return;
    }

    if (memcmp((read_data + 8), "XXX", 3) != 0) {
        TEST_FAIL("Read buffer contains illegal post-data");
        return;
    }

    ret->val = TEST_PASSED;
}

/**
 * \brief Tests correct behaviour when data is written in the secure storage
 *        area and the secure_fs_perpare is called after it.
 *        The expected behaviour is to read back the data wrote
 *        before the seconds perpare call.
 */
static void tfm_sst_test_2008(struct test_result_t *ret)
{
    const uint32_t app_id = S_APP_ID;
    const uint16_t asset_uuid = SST_ASSET_ID_X509_CERT_LARGE;
    enum tfm_sst_err_t err;
    struct tfm_sst_buf_t io_data;
    uint32_t hdl;
    uint8_t read_data[READ_BUF_SIZE] = "XXXXXXXXXXX";
    uint8_t wrt_data[WRITE_BUF_SIZE] = "DATA";

    /* Prepares test context */
    if (prepare_test_ctx(ret) != 0) {
        return;
    }

    /* Creates asset to get a valid handle */
    err = tfm_sst_veneer_create(app_id, asset_uuid);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Create should not fail for application S_APP_ID");
        return;
    }

    /* Gets asset's handle */
    err = tfm_sst_veneer_get_handle(app_id, asset_uuid, &hdl);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should return a valid asset handle");
        return;
    }

    /* Sets data structure */
    io_data.data = wrt_data;
    io_data.size = WRITE_BUF_SIZE;
    io_data.offset = 0;

    /* Write data in the asset */
    err = tfm_sst_veneer_write(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Write should works correctly");
        return;
    }

    /* Calls prepare again to simulate reinitialization */
    err = sst_object_prepare();
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Saved system should have been preparable");
        return;
    }

    /* Sets data structure */
    io_data.data = (read_data + 3);
    io_data.size = WRITE_BUF_SIZE;
    io_data.offset = 0;

    /* Read back the data after the prepare */
    err = tfm_sst_veneer_read(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Incorrect number of bytes read back");
        return;
    }

    if (memcmp(read_data, "XXX", 3) != 0) {
        TEST_FAIL("Read buffer contains illegal pre-data");
        return;
    }

    if (memcmp((read_data + 3), wrt_data, WRITE_BUF_SIZE) != 0) {
        TEST_FAIL("Read buffer has read incorrect data");
        return;
    }

    if (memcmp((read_data + 8), "XXX", 3) != 0) {
        TEST_FAIL("Read buffer contains illegal post-data");
        return;
    }

    ret->val = TEST_PASSED;
}

/**
 * \brief Tests write function against a write call where data size is
 *        bigger than the maximum assert size.
 */
static void tfm_sst_test_2009(struct test_result_t *ret)
{
    const uint32_t app_id = S_APP_ID;
    const uint16_t asset_uuid = SST_ASSET_ID_SHA224_HASH;
    enum tfm_sst_err_t err;
    struct tfm_sst_buf_t io_data;
    uint32_t hdl;
    uint8_t wrt_data[BUF_SIZE_SHA224] = {0};

    /* Prepares test context */
    if (prepare_test_ctx(ret) != 0) {
        return;
    }

    /* Creates asset to get a valid handle */
    err = tfm_sst_veneer_create(app_id, asset_uuid);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Create should not fail for application S_APP_ID");
        return;
    }

    /* Gets asset's handle */
    err = tfm_sst_veneer_get_handle(app_id, asset_uuid, &hdl);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should return a valid asset handle");
        return;
    }

    /* Sets data structure */
    io_data.data = wrt_data;
    io_data.size = SST_ASSET_MAX_SIZE_SHA224_HASH + 1;
    io_data.offset = 0;

    /* Write data in the asset when data size is bigger than asset size */
    err = tfm_sst_veneer_write(app_id, hdl, &io_data);
    if (err == TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Should have failed asset write of too large");
        return;
    }

    ret->val = TEST_PASSED;
}

/**
 * \brief Tests write function against multiple writes.
 */
static void tfm_sst_test_2010(struct test_result_t *ret)
{
    const uint32_t app_id = S_APP_ID;
    const uint16_t asset_uuid = SST_ASSET_ID_SHA224_HASH;
    enum tfm_sst_err_t err;
    struct tfm_sst_buf_t io_data;
    uint32_t hdl;
    uint8_t read_data[READ_BUF_SIZE]  = "XXXXXXXXXXX";
    uint8_t wrt_data[WRITE_BUF_SIZE+1]  = "Hello";
    uint8_t wrt_data2[WRITE_BUF_SIZE+1] = "World";

    /* Prepares test context */
    if (prepare_test_ctx(ret) != 0) {
        return;
    }

    /* Creates asset to get a valid handle */
    err = tfm_sst_veneer_create(app_id, asset_uuid);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Create should not fail for application S_APP_ID");
        return;
    }

    /* Gets asset's handle */
    err = tfm_sst_veneer_get_handle(app_id, asset_uuid, &hdl);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should return a valid asset handle");
        return;
    }

    /* Sets data structure */
    io_data.data = wrt_data;
    io_data.size = WRITE_BUF_SIZE;
    io_data.offset = 0;

    /* Write data in the asset */
    err = tfm_sst_veneer_write(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Write data 1 failed");
        return;
    }

    /* Sets data structure */
    io_data.data = wrt_data2;
    io_data.size = WRITE_BUF_SIZE;
    io_data.offset = WRITE_BUF_SIZE;

    /* Write data 2 in the asset */
    err = tfm_sst_veneer_write(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Write data 2 failed");
        return;
    }

    /* Sets data structure */
    io_data.data = read_data;
    io_data.size = WRITE_BUF_SIZE * 2;
    io_data.offset = 0;

    /* Read back the data */
    err = tfm_sst_veneer_read(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Incorrect number of bytes read back");
        return;
    }

    /* The X is used to check that the number of bytes read was exactly the
     * number requested
     */
    if (memcmp(read_data, "HelloWorldX", READ_BUF_SIZE) != 0) {
        TEST_FAIL("Read buffer has read incorrect data");
        return;
    }

    ret->val = TEST_PASSED;
}

/**
 * \brief Tests write function against multiple writes until the end of asset.
 */
static void tfm_sst_test_2011(struct test_result_t *ret)
{
    const uint32_t app_id = S_APP_ID;
    const uint16_t asset_uuid = SST_ASSET_ID_SHA224_HASH;
    enum tfm_sst_err_t err;
    struct tfm_sst_buf_t io_data;
    uint32_t hdl;
    uint8_t read_data[BUF_SIZE_SHA224] = READ_DATA_SHA224;
    uint8_t wrt_data[BUF_SIZE_SHA224] = WRITE_DATA_SHA224_1;
    uint8_t wrt_data2[BUF_SIZE_SHA224] = WRITE_DATA_SHA224_2;

    /* Prepares test context */
    if (prepare_test_ctx(ret) != 0) {
        return;
    }

    /* Creates asset to get a valid handle */
    err = tfm_sst_veneer_create(app_id, asset_uuid);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Create should not fail for application S_APP_ID");
        return;
    }

    /* Gets asset's handle */
    err = tfm_sst_veneer_get_handle(app_id, asset_uuid, &hdl);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should return a valid asset handle");
        return;
    }

    /* Sets data structure */
    io_data.data = wrt_data;
    io_data.size = WRITE_BUF_SIZE;
    io_data.offset = 0;

    /* Write data in the asset */
    err = tfm_sst_veneer_write(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Write data 1 failed");
        return;
    }

    /* Sets data structure */
    io_data.data = wrt_data2;
    io_data.size = (SST_ASSET_MAX_SIZE_SHA224_HASH - WRITE_BUF_SIZE) + 1;
    io_data.offset = WRITE_BUF_SIZE;

    /* Write data in the asset */
    err = tfm_sst_veneer_write(app_id, hdl, &io_data);
    if (err == TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Write data 2 should have failed as this write tries to "
                  "write more bytes that the max size");
        return;
    }

    /* Sets data structure */
    io_data.data = wrt_data + WRITE_BUF_SIZE;
    io_data.size = SST_ASSET_MAX_SIZE_SHA224_HASH - WRITE_BUF_SIZE;
    io_data.offset = WRITE_BUF_SIZE;

    /* Write data in the asset */
    err = tfm_sst_veneer_write(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Write data 3 failed");
        return;
    }

    /* Sets data structure */
    io_data.data = read_data;
    io_data.size = SST_ASSET_MAX_SIZE_SHA224_HASH;
    io_data.offset = 0;

    /* Read back the data */
    err = tfm_sst_veneer_read(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Incorrect number of bytes read back");
        return;
    }

    if (memcmp(read_data, wrt_data, BUF_SIZE_SHA224) != 0) {
        TEST_FAIL("Read buffer has incorrect data");
        return;
    }

    ret->val = TEST_PASSED;
}
/**
 * \brief Tests write and read to/from 2 assets.
 */
static void tfm_sst_test_2012(struct test_result_t *ret)
{

    const uint32_t app_id_1 = S_APP_ID;
    const uint32_t app_id_2 = S_APP_ID;
    const uint16_t asset_uuid_1 = SST_ASSET_ID_X509_CERT_LARGE;
    const uint16_t asset_uuid_2 = SST_ASSET_ID_SHA224_HASH;
    enum tfm_sst_err_t err;
    uint32_t hdl_1;
    uint32_t hdl_2;
    struct tfm_sst_buf_t io_data;
    uint8_t read_data[READ_BUF_SIZE] = "XXXXXXXXXXX";
    uint8_t wrt_data[WRITE_BUF_SIZE+1] = "Hello";
    uint8_t wrt_data2[3] = "Hi";
    uint8_t wrt_data3[WRITE_BUF_SIZE+1] = "World";
    uint8_t wrt_data4[WRITE_BUF_SIZE+1] = "12345";

    /* Prepares test context */
    if (prepare_test_ctx(ret) != 0) {
        return;
    }

    /* Creates asset 1 to get a valid handle */
    err = tfm_sst_veneer_create(app_id_1, asset_uuid_1);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Create should not fail for application S_APP_ID");
        return;
    }

    /* Gets asset's handle 1 */
    err = tfm_sst_veneer_get_handle(app_id_1, asset_uuid_1, &hdl_1);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should return a valid asset handle");
        return;
    }

    /* Creates asset 2 to get a valid handle */
    err = tfm_sst_veneer_create(app_id_2, asset_uuid_2);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Create should not fail for application S_APP_ID");
        return;
    }

    /* Gets asset's handle 2 */
    err = tfm_sst_veneer_get_handle(app_id_2, asset_uuid_2, &hdl_2);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should return a valid asset handle");
        return;
    }

    /* Sets data structure */
    io_data.data = wrt_data;
    io_data.size = WRITE_BUF_SIZE;
    io_data.offset = 0;

    /* Write data in asset 1 */
    err = tfm_sst_veneer_write(app_id_1, hdl_1, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Write data should work for application S_APP_ID");
        return;
    }

    /* Sets data structure */
    io_data.data = wrt_data2;
    io_data.size = 2;
    io_data.offset = 0;

    /* Write data 2 in asset 2 */
    err = tfm_sst_veneer_write(app_id_2, hdl_2, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Write data should work for application S_APP_ID");
        return;
    }

    /* Sets data structure */
    io_data.data = wrt_data3;
    io_data.size = WRITE_BUF_SIZE;
    io_data.offset = WRITE_BUF_SIZE;

    /* Write data 3 in asset 1 */
    err = tfm_sst_veneer_write(app_id_1, hdl_1, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Write data should work for application S_APP_ID");
        return;
    }

    /* Sets data structure */
    io_data.data = wrt_data4;
    io_data.size = WRITE_BUF_SIZE;
    io_data.offset = 2;

    /* Write data 4 in asset 2 */
    err = tfm_sst_veneer_write(app_id_2, hdl_2, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Write data should work for application S_APP_ID");
        return;
    }

    /* Sets data structure */
    io_data.data = read_data;
    io_data.size = WRITE_BUF_SIZE * 2; /* size of wrt_data + wrt_data3 */
    io_data.offset = 0;

    /* Read back the asset 1 */
    err = tfm_sst_veneer_read(app_id_1, hdl_1, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Incorrect number of bytes read back");
        return;
    }

    if (memcmp(read_data, "HelloWorldX", READ_BUF_SIZE) != 0) {
        TEST_FAIL("Read buffer has incorrect data");
        return;
    }

    /* Resets read buffer content to a known data */
    memset(read_data, 'X', READ_BUF_SIZE - 1);

    /* Sets data structure */
    io_data.data = read_data;
    io_data.size = 2 + WRITE_BUF_SIZE; /* size of wrt_data2 + wrt_data4 */
    io_data.offset = 0;

    /* Read back the asset 2 */
    err = tfm_sst_veneer_read(app_id_2, hdl_2, &io_data);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Incorrect number of bytes read back");
        return;
    }

    if (memcmp(read_data, "Hi12345XXXX", READ_BUF_SIZE) != 0) {
        TEST_FAIL("Read buffer has incorrect data");
        return;
    }

    ret->val = TEST_PASSED;
}

/**
 * \brief Tests write and read to/from the follow illegal locations:
 * - ROM memory
 * - Device memory
 * - Non existing memory location
 */
static void tfm_sst_test_2013(struct test_result_t *ret)
{
    uint32_t app_id = S_APP_ID;
    const uint16_t asset_uuid = SST_ASSET_ID_SHA224_HASH;
    enum tfm_sst_err_t err;
    struct tfm_sst_buf_t io_data;
    uint32_t hdl;

    /* Prepares test context */
    if (prepare_test_ctx(ret) != 0) {
        return;
    }

    /* Creates asset to get a valid handle */
    err = tfm_sst_veneer_create(app_id, asset_uuid);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Create should not fail for application S_APP_ID");
        return;
    }

    /* Gets asset's handle */
    err = tfm_sst_veneer_get_handle(app_id, asset_uuid, &hdl);
    if (err != TFM_SST_ERR_SUCCESS) {
        TEST_FAIL("Get handle should return a valid asset handle");
        return;
    }

    /*** Check functions against ROM address location ***/

    /* Gets asset's handle with a ROM address location to store asset's
     * handle
     */
    err = tfm_sst_veneer_get_handle(app_id, asset_uuid,
                                    (uint32_t *)ROM_ADDR_LOCATION);
    if (err != TFM_SST_ERR_PARAM_ERROR) {
        TEST_FAIL("Get handle should fail for an illegal location");
        return;
    }

    /* Sets data structure */
    io_data.data = (uint8_t *)ROM_ADDR_LOCATION;
    io_data.size = 1;
    io_data.offset = 0;

    /* Calls write with a ROM address location */
    err = tfm_sst_veneer_write(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("Write should fail for an illegal location");
        return;
    }

    /* Calls read with a ROM address location */
    err = tfm_sst_veneer_read(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("Read should fail for an illegal location");
        return;
    }

    /*** Check functions against devices address location ***/

    /* Gets asset's handle with a devices address location to store asset's
     * handle
     */
    err = tfm_sst_veneer_get_handle(app_id, asset_uuid,
                                    (uint32_t *)DEV_ADDR_LOCATION);
    if (err != TFM_SST_ERR_PARAM_ERROR) {
        TEST_FAIL("Get handle should fail for an illegal location");
        return;
    }

    /* Sets data structure */
    io_data.data = (uint8_t *)DEV_ADDR_LOCATION;

    /* Calls write with a device address location */
    err = tfm_sst_veneer_write(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("Write should fail for an illegal location");
        return;
    }

    /* Calls read with a device address location */
    err = tfm_sst_veneer_read(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("Read should fail for an illegal location");
        return;
    }

    /*** Check functions against non existing address location ***/

    /* Gets asset's handle with a non existing address location to store asset's
     * handle
     */
    err = tfm_sst_veneer_get_handle(app_id, asset_uuid,
                                    (uint32_t *)NON_EXIST_ADDR_LOCATION);
    if (err != TFM_SST_ERR_PARAM_ERROR) {
        TEST_FAIL("Get handle should fail for an illegal location");
        return;
    }

    /* Sets data structure */
    io_data.data = (uint8_t *)NON_EXIST_ADDR_LOCATION;

    /* Calls write with a non-existing address location */
    err = tfm_sst_veneer_write(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("Write should fail for an illegal location");
        return;
    }

    /* Calls read with a non-existing address location */
    err = tfm_sst_veneer_read(app_id, hdl, &io_data);
    if (err != TFM_SST_ERR_ASSET_NOT_FOUND) {
        TEST_FAIL("Read should fail for an illegal location");
        return;
    }

    ret->val = TEST_PASSED;
}
