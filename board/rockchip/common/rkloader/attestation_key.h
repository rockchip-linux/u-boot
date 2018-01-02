/*
 * Copyright 2018, Rockchip Electronics Co., Ltd
 * qiujian, <qiujian@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef ATTESTATION_KEY_H_
#define ATTESTATION_KEY_H_

#include "../config.h"

#define ATAP_BLOB_LEN_MAX 2048
#define ATAP_CERT_CHAIN_LEN_MAX 8192
#define ATAP_CERT_CHAIN_ENTRIES_MAX 8

/* Name of the attestation key file is kAttestKeyPrefix.%algorithm, where
 * algorithm is either "EC" or "RSA"
 */
#define kAttestKeyPrefix "PrivateKey"


/* Name of the attestation certificate file is kAttestCertPrefix.%algorithm.%index,
 * where index is the index within the certificate chain.
 */
#define kAttestCertPrefix "CertificateChain"

/* Maximum file name size.*/
#define kStorageIdLengthMax  64

typedef enum{
	KM_ALGORITHM_RSA = 1,
	KM_ALGORITHM_EC = 3,
}keymaster_algorithm_t;

typedef struct {
	uint8_t* data;
	uint32_t data_length;
} AtapBlob;

typedef struct {
	AtapBlob entries[ATAP_CERT_CHAIN_ENTRIES_MAX];
	uint32_t entry_count;
} AtapCertChain;

typedef enum {
	ATAP_RESULT_OK,
	ATAP_RESULT_ERROR_PARTITION_NOT_FOUND,
	ATAP_RESULT_ERROR_BLOCK_READ,
	ATAP_RESULT_ERROR_BLOCK_WRITE,
	ATAP_RESULT_ERROR_INVALID_HEAD,
	ATAP_RESULT_ERROR_INVALID_BLOCK_NUM,
	ATAP_RESULT_ERROR_INVALID_DEVICE_ID,
	ATAP_RESULT_ERROR_BUF_COPY,
	ATAP_RESULT_ERROR_STORAGE,
} AtapResult;

/* These copy serialized data from |*buf_ptr| to the output structure, and
 * increment |*buf_ptr| by [number of bytes copied]. Returns true on success,
 * false when the serialized format is invalid.
 */
void free_blob(AtapBlob blob);
void free_cert_chain(AtapCertChain cert_chain);
void copy_from_buf(uint8_t** buf_ptr, void* data, uint32_t data_size);
void copy_uint32_from_buf(uint8_t** buf_ptr, uint32_t* x);
bool copy_blob_from_buf(uint8_t** buf_ptr, AtapBlob* blob);
bool copy_cert_chain_from_buf(uint8_t** buf_ptr, AtapCertChain* cert_chain);

/* validate attestation data head. */
bool validate_ca_header(const uint8_t* buf,uint32_t buf_size);

/* write key to security storage. */
uint32_t WriteKeyToStorage(keymaster_algorithm_t key_type, const uint8_t* key, uint32_t key_size);

/* write cert to security storage. */
uint32_t WriteCertToStorage(keymaster_algorithm_t key_type, const uint8_t* cert,
                                     uint32_t cert_size, uint32_t index);

/* write cert length to security storage. */
uint32_t WriteChainLengthToStorage(keymaster_algorithm_t key_type, uint8_t chain_len);

/* load attestation key from misc partition. */
AtapResult load_attestation_key(void);

#endif	//ATTESTATION_KEY_H_
