/*
 * Copyright 2018, Rockchip Electronics Co., Ltd
 * qiujian, <qiujian@rock-chips.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "attestation_key.h"

#include <common.h>
#include <malloc.h>

#include "../config.h"
#include "parameter.h"

#include <optee_include/OpteeClientApiLib.h>

extern uint32_t write_to_keymaster(uint8_t *filename, uint32_t filename_size,
	uint8_t *data, uint32_t data_size);


const char* GetKeySlotStr(keymaster_algorithm_t key_type) {
	switch (key_type) {
	case KM_ALGORITHM_RSA:
		return "RSA";
	case KM_ALGORITHM_EC:
		return "EC";
	default:
		return "";
	}
}

void free_blob(AtapBlob blob) {
	if (blob.data) {
		free(blob.data);
	}
	blob.data_length = 0;
}

void free_cert_chain(AtapCertChain cert_chain) {
	unsigned int i = 0;

	for (i = 0; i < cert_chain.entry_count; ++i) {
		if (cert_chain.entries[i].data) {
			free(cert_chain.entries[i].data);
		}
		cert_chain.entries[i].data_length = 0;
	}
	memset(&cert_chain, 0, sizeof(AtapCertChain));
}

void copy_from_buf(uint8_t** buf_ptr, void* data, uint32_t data_size) {
	memcpy(data, *buf_ptr, data_size);
	*buf_ptr += data_size;
}

void copy_uint32_from_buf(uint8_t** buf_ptr, uint32_t* x) {
	copy_from_buf(buf_ptr, x, sizeof(uint32_t));
}

bool copy_blob_from_buf(uint8_t** buf_ptr, AtapBlob* blob) {
	memset(blob, 0, sizeof(AtapBlob));
	copy_uint32_from_buf(buf_ptr, &blob->data_length);

	if (blob->data_length > ATAP_BLOB_LEN_MAX) {
		return false;
	}
	if (blob->data_length) {
		blob->data = (uint8_t*) malloc(blob->data_length);
		if (blob->data == NULL) {
			return false;
		}
		copy_from_buf(buf_ptr, blob->data, blob->data_length);
	}
	return true;
}

bool copy_cert_chain_from_buf(uint8_t** buf_ptr, AtapCertChain* cert_chain) {
	uint32_t cert_chain_size = 0;
	int32_t bytes_remaining = 0;
	size_t i = 0;
	bool retval = true;

	memset(cert_chain, 0, sizeof(AtapCertChain));

	/* Copy size of cert chain, as it is a Variable field. */
	copy_from_buf(buf_ptr, &cert_chain_size, sizeof(cert_chain_size));

	if (cert_chain_size > ATAP_CERT_CHAIN_LEN_MAX) {
		return false;
	}
	if (cert_chain_size == 0) {
		return true;
	}
	bytes_remaining = cert_chain_size;
	for (i = 0; i < ATAP_CERT_CHAIN_ENTRIES_MAX; ++i) {
		if (!copy_blob_from_buf(buf_ptr, &cert_chain->entries[i])) {
			retval = false;
			break;
		}

		++cert_chain->entry_count;
		bytes_remaining -= (sizeof(uint32_t) + cert_chain->entries[i].data_length);

		if (bytes_remaining <= 0) {
			retval = (bytes_remaining == 0);
			break;
		}
	}
	if (retval == false) {
		free_cert_chain(*cert_chain);
	}
	return retval;
}

bool validate_ca_header(const uint8_t* buf,uint32_t buf_size){
	if(buf[0] != 'C' || buf[1] != 'A' || buf[2] != 0){
		printf("invalide header tag\n");
		return false;
	}

	uint32_t data_size;
	memcpy(&data_size, buf + 3, sizeof(uint32_t));

	if (data_size <= 0 || data_size > 1024 * 64){
		printf("invalide data_size:%d\n", data_size);
		return false;
	}

	uint32_t real_size;
	memcpy(&real_size, buf + 3 + sizeof(uint32_t), sizeof(uint32_t));
	if (real_size <= 0 || real_size > data_size){
		printf("invalide real_size:%d\n", real_size);
		return false;
	}
	return true;
}

uint32_t WriteKeyToStorage(keymaster_algorithm_t key_type,
		const uint8_t* key,uint32_t key_size){
	char key_file[kStorageIdLengthMax] = {0};
	snprintf(key_file, kStorageIdLengthMax, "%s.%s", kAttestKeyPrefix, GetKeySlotStr(key_type));
	write_to_keymaster((uint8_t *)key_file, strlen(key_file), (uint8_t *)key, key_size);
	return 0;
}
uint32_t WriteCertToStorage(keymaster_algorithm_t key_type, const uint8_t* cert,
                                     uint32_t cert_size, uint32_t index){
	char cert_file[kStorageIdLengthMax] = {0};
	snprintf(cert_file, kStorageIdLengthMax, "%s.%s.%d", kAttestCertPrefix,
		GetKeySlotStr(key_type), index);
	write_to_keymaster((uint8_t *)cert_file, strlen(cert_file), (uint8_t *)cert, cert_size);
	return 0;
}

uint32_t WriteChainLengthToStorage(keymaster_algorithm_t key_type, uint8_t chain_len){
	char cert_chain_length_file[kStorageIdLengthMax] = {0};
	uint8_t data = chain_len;
	uint32_t len = 1;

	snprintf(cert_chain_length_file, kStorageIdLengthMax, "%s.%s.length", kAttestCertPrefix,
		GetKeySlotStr(key_type));
	write_to_keymaster((uint8_t *)cert_chain_length_file, strlen(cert_chain_length_file), &data,len);

	return 0;
}

AtapResult load_attestation_key(){

	/* get misc partition */
	const disk_partition_t *ptn = get_disk_partition(MISC_NAME);
	if (!ptn) {
		printf("misc partition not found!\n");
		return ATAP_RESULT_ERROR_PARTITION_NOT_FOUND;
	}

	/* get attestation data offset from misc partition */
	lbaint_t key_offset = ptn->start + ptn->size - 128;

	/* read ca head from attestation data offset */
	uint8_t ca_headr[512];
	if (StorageReadLba(key_offset, ca_headr, 1) != 0){
		printf("failed to read ca head from misc\n");
		return ATAP_RESULT_ERROR_BLOCK_READ;
	}

	if (!validate_ca_header(ca_headr, sizeof(ca_headr))){
		printf("ca head not found\n");
		return ATAP_RESULT_ERROR_INVALID_HEAD;
	}

	/* get attestation data size from ca head */
	uint32_t real_size;
	memcpy(&real_size, ca_headr + 3 + sizeof(uint32_t), sizeof(uint32_t));

	/* calculate real block size of attestation data */
	int real_block_num = real_size / 512;
	if (real_size % 512 != 0){
		real_block_num++;
	}

	unsigned char keybuf[64 * 1024] = {0};
	int block_num = (64 * 1024) / 512;

	/* check block size */
	if (real_block_num <= 0 || real_block_num > block_num){
		printf("invalidate real_block_num:%d\n", real_block_num);
		return ATAP_RESULT_ERROR_INVALID_BLOCK_NUM;
	}

	/* read all attestation data from misc */
	if (StorageReadLba(key_offset, keybuf, real_block_num) != 0){
		printf("failed to read misc key\n");
		return ATAP_RESULT_ERROR_BLOCK_READ;
	}

	/* read device id from buf*/
	uint32_t device_id_size = 0;
	uint8_t device_id[32] = {0};

	memcpy(&device_id_size, keybuf + 16, sizeof(uint32_t));
	if (device_id_size < 0 || device_id_size > sizeof(device_id)){
		printf("invalidate device_id_size:%d\n", device_id_size);
		return ATAP_RESULT_ERROR_INVALID_DEVICE_ID;
	}

	memcpy(device_id, keybuf + 16 + sizeof(uint32_t), device_id_size);
	printf("device_id:%s\n",device_id);

	/* read algorithm from buf */
	uint8_t *key_buf = keybuf + 16 + sizeof(uint32_t) + device_id_size;
	uint32_t algorithm;
	copy_uint32_from_buf(&key_buf, &algorithm);
	printf("\n algorithm:%d\n", algorithm);

	/* read rsa private key */
	AtapBlob key;
	if (copy_blob_from_buf(&key_buf, &key) == false){
		printf("copy_blob_from_buf failed!\n");
		return ATAP_RESULT_ERROR_BUF_COPY;
	}
	/* write rsa private key to security */
	WriteKeyToStorage(KM_ALGORITHM_RSA, key.data, key.data_length);

	/* read rsa cert chain */
	AtapCertChain certChain;
	if (copy_cert_chain_from_buf(&key_buf, &certChain) == false){
		printf("copy_cert_chain_from_buf failed!\n");
		return ATAP_RESULT_ERROR_BUF_COPY;
	}

	/* write rsa cert chain size to security */
	WriteChainLengthToStorage(KM_ALGORITHM_RSA, (uint8_t) certChain.entry_count);

	/* write rsa cert chain data to security */
	int i = 0;
	for (i = 0; i < certChain.entry_count; ++i){
		WriteCertToStorage(KM_ALGORITHM_RSA, certChain.entries[i].data,
			certChain.entries[i].data_length, i);
	}

	/* read ec algorithm */
	copy_uint32_from_buf(&key_buf, &algorithm);
	printf("\n algorithm:%d\n", algorithm);

	/* read ec private key */
	free_blob(key);
	if (copy_blob_from_buf(&key_buf, &key) == false){
		printf("copy_blob_from_buf failed!\n");
		return ATAP_RESULT_ERROR_BUF_COPY;
	}

	/* write ec private key to security */
	WriteKeyToStorage(KM_ALGORITHM_EC, key.data, key.data_length);

	/* read ec cert chain */
	free_cert_chain(certChain);
	if(copy_cert_chain_from_buf(&key_buf, &certChain) == false){
		printf("copy_cert_chain_from_buf failed!\n");
		return ATAP_RESULT_ERROR_BUF_COPY;
	}
	/* write ec cert chain size to security */
	WriteChainLengthToStorage(KM_ALGORITHM_EC, (uint8_t) certChain.entry_count);

	/* write ec cert chain to security */
	for (i = 0; i < certChain.entry_count; ++i){
		WriteCertToStorage(KM_ALGORITHM_EC, certChain.entries[i].data,
			certChain.entries[i].data_length, i);
	}

	memset(keybuf, 0, sizeof(keybuf));

	/* wipe attestation data from misc*/
	if (StorageWriteLba(key_offset, keybuf, real_block_num, 0) != 0){
		printf("StorageWriteLba failed\n");
		return ATAP_RESULT_ERROR_BLOCK_WRITE;
	}

	return ATAP_RESULT_OK;
}
