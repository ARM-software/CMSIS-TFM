/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __SST_CRYPTO_INTERFACE_H__
#define __SST_CRYPTO_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "tfm_sst_defs.h"

#define SST_KEY_LEN_BITS  128
#define SST_TAG_LEN_BYTES  16
#define SST_IV_LEN_BYTES   12

#define SST_KEY_LEN_BYTES  16

/* Union containing crypto policy implementations. The ref member provides the
 * reference implementation. Further members can be added to the union to
 * provide alternative implementations.
 */
union sst_crypto_t {
    struct {
        uint8_t tag[SST_TAG_LEN_BYTES]; /*!< MAC value of AEAD object */
        uint8_t iv[SST_IV_LEN_BYTES];   /*!< IV value of AEAD object */
    } ref;
};


/**
 * \brief Initializes the crypto engine.
 *
 * \return Returns values as described in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t sst_crypto_init(void);

/**
 * \brief Gets a key for specific app id
 *
 * \param[out] key      Cryptographic key
 * \param[in]  key_len  Key len
 *
 * \return Returns values as described in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t sst_crypto_getkey(uint8_t *key, size_t key_len);

/**
 * \brief Sets key for crypto operations
 *
 * \param[in] key      Cryptographic key
 * \param[in] key_len  Key len
 *
 * \return Returns values as described in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t sst_crypto_setkey(const uint8_t *key, size_t key_len);

/**
 * \brief Encrypts and tag the given plain text data.
 *
 * \param[in,out] crypto   Pointer to the crypto union
 * \param[in]     add      Starting address of the associated data
 * \param[in]     add_len  Length of the associated data
 * \param[in]     in       Pointer to the input data
 * \param[out]    out      Buffer pointer to store the encrypted data
 * \param[in]     len      Input and output buffer lengths
 *
 * \return Returns values as described in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t sst_crypto_encrypt_and_tag(union sst_crypto_t *crypto,
                                              const uint8_t *add,
                                              size_t add_len,
                                              const uint8_t *in,
                                              uint8_t *out,
                                              size_t len);

/**
 * \brief Decrypts and checks the given encrypted data.
 *
 * \param[in]  crypto   Pointer to the crypto union
 * \param[in]  add      Starting address of the associated data
 * \param[in]  add_len  Length of the associated data
 * \param[in]  in       Pointer to the input data
 * \param[out] out      Buffer pointer to store the encrypted data
 * \param[in]  len      Input and output buffer lengths
 *
 * \return Returns values as described in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t sst_crypto_auth_and_decrypt(const union sst_crypto_t *crypto,
                                               const uint8_t *add,
                                               size_t add_len,
                                               const uint8_t *in,
                                               uint8_t *out,
                                               size_t len);

/**
 * \brief Generates authentication MAC value for give data
 *
 * \param[in,out] crypto   Pointer to the crypto union
 * \param[in]     add      Starting address of the data to authenticate
 * \param[in]     add_len  Length of data to authenticate
 *
 * \return Returns values as described in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t sst_crypto_generate_auth_tag(union sst_crypto_t *crypto,
                                                const uint8_t *add,
                                                uint32_t add_len);

/**
 * \brief Authenticate give data against the tag
 *
 * \param[in] crypto   Pointer to the crypto union
 * \param[in] add      Starting address of the data to authenticate
 * \param[in] add_len  Length of data to authenticate
 *
 * \return Returns values as described in \ref tfm_sst_err_t
 */
enum tfm_sst_err_t sst_crypto_authenticate(const union sst_crypto_t *crypto,
                                           const uint8_t *add,
                                           uint32_t add_len);

/**
 * \brief Provides current IV value to crypto layer
 *
 * \param[in] crypto  Pointer to the crypto union
 */
void sst_crypto_set_iv(const union sst_crypto_t *crypto);

/**
 * \brief Gets a new IV value into the buffer
 *
 * \param[out] crypto  Pointer to the crypto union
 */
void sst_crypto_get_iv(union sst_crypto_t *crypto);

#ifdef __cplusplus
}
#endif

#endif /* __SST_CRYPTO_INTERFACE_H__ */
