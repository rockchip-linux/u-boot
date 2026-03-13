// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <stdlib.h>
#include <attestation_key.h>
#include <id_attestation.h>
#include <write_keybox.h>
#include <tee.h>
#include <tee/optee.h>
#include <youtube_keybox.h>

#define	SIZE_OF_TAG		4
#define	BOOT_FROM_EMMC	(1 << 1)
#define	WIDEVINE_TAG	"KBOX"
#define	ATTESTATION_TAG	"ATTE"
#define	ID_ATTESTATION_TAG "IDAT"
#define PLAYREADY30_TAG	"SL30"
#define YOUTUBE_SECRET_KEY "YTSK"
#define COMMON_DATA_TAG "COMM" //common data with filename

uint32_t write_keybox_to_secure_storage(uint8_t* received_data, uint32_t len)
{
	uint8_t *widevine_data;
	uint8_t *attestation_data;
	uint8_t *id_attestation_data;
	uint8_t *playready_sl30_data;
	uint8_t* youtube_key_data;
	uint8_t *common_data;
	uint32_t key_size;
	uint32_t data_size;
	int rc = 0;
	uint32_t ret;

	widevine_data = (uint8_t *)new_strstr((char *)received_data,
					      WIDEVINE_TAG, len);
	attestation_data = (uint8_t *)new_strstr((char *)received_data,
						 ATTESTATION_TAG, len);
	id_attestation_data = (uint8_t *)new_strstr((char *)received_data,
						    ID_ATTESTATION_TAG, len);
	playready_sl30_data = (uint8_t *)new_strstr((char *)received_data,
						    PLAYREADY30_TAG, len);
	youtube_key_data = (uint8_t *)new_strstr((char *)received_data,
                                                YOUTUBE_SECRET_KEY, len);
	common_data = (uint8_t *)new_strstr((char *)received_data,
						    COMMON_DATA_TAG, len);
	if (widevine_data) {
		/* widevine keybox */
		key_size = *(widevine_data + SIZE_OF_TAG);
		data_size = *(widevine_data + SIZE_OF_TAG + sizeof(key_size));

		ret = optee_write_widevine_keybox((uint8_t *)"widevine_keybox",
					       sizeof("widevine_keybox"),
					       widevine_data + SIZE_OF_TAG +
					       sizeof(key_size) + sizeof(data_size),
					       key_size,
					       widevine_data + 12 + key_size,
					       data_size);
		if (ret == TEE_SUCCESS) {
			rc = 0;
			printf("write widevine keybox to secure storage success\n");
		} else {
			rc = -EIO;
			printf("write widevine keybox to secure storage fail\n");
		}
	} else if (attestation_data) {
		/* attestation key */
		atap_result ret;

		ret = write_attestation_key_to_secure_storage(attestation_data,
							      len);
		if (ret == ATAP_RESULT_OK) {
			rc = 0;
			printf("write attestation key to secure storage success\n");
		} else {
			rc = -EIO;
			printf("write attestation key to secure storage fail\n");
		}
	} else if (id_attestation_data) {
		/* id attestation */
		ret = write_id_attestation_to_secure_storage(id_attestation_data, len);
		if (ret == ATAP_RESULT_OK) {
			rc = 0;
			printf("write id attestation success!\n");
		} else {
			rc = -EIO;
			printf("write id attestation failed\n");
		}
	} else if (playready_sl30_data) {
		/* PlayReady SL3000 root key */
		uint32_t ret;

		data_size = *(playready_sl30_data + SIZE_OF_TAG);
		ret = optee_write_keybox((uint8_t *)"PlayReady_SL3000",
					 sizeof("PlayReady_SL3000"),
					 playready_sl30_data +
					 SIZE_OF_TAG + sizeof(data_size),
					 data_size);
		if (ret == TEE_SUCCESS) {
			rc = 0;
			printf("write PlayReady SL3000 root key to secure storage success\n");
		} else {
			rc = -EIO;
			printf("write PlayReady SL3000 root key to secure storage fail\n");
		}
	} else if (youtube_key_data) {
		/* YouTube Secret Key */
		rc = write_youtube_keybox_to_secure_storage(youtube_key_data, len);
		if (rc != 0) {
			printf("write youtube keybox to secure storage fail (%d)\n", rc);
		}
	}else if (common_data) {
		/* common data with filename */
		uint32_t ret;
		uint32_t filename_len;
		char filename[32];

		// common data format according to description:
		// - first 8 bytes: tag(4 byte) + size of key(4 byte)
		// - then 32 bytes related to filename: first 4 bytes is filename length, remaining 28 bytes is filename
		// - finally: data to be written
		uint8_t *after_header = common_data + SIZE_OF_TAG + sizeof(uint32_t); // skip tag(4) + key size(4)
		uint32_t key_and_data_size = *(uint32_t*)(common_data + SIZE_OF_TAG); // get size after tag

		// The 32 bytes after header contain filename info
		uint8_t *filename_info_ptr = after_header; // point to filename info (32 bytes)

		// Extract filename length and filename from the 32-byte filename info
		filename_len = *(uint32_t*)filename_info_ptr; // first 4 bytes is filename length
		filename_len = filename_len + 1;
		strncpy(filename, (char*)(filename_info_ptr + sizeof(uint32_t)), filename_len); // next 28 bytes is filename
		filename[filename_len] = '\0'; // ensure null termination

		// Data to write is after the 32-byte filename info
		uint8_t *data_to_write = filename_info_ptr + 32; // skip 32-byte filename info
		// Calculate the actual data size - this could be the remainder of the key_and_data_size
		uint32_t data_to_write_size = key_and_data_size - 32; // subtract the 32-byte filename info
#if 0
		/* Debug: print parsed common_data info */
		printf("DEBUG: common_data(len:%d) parse info:\n", len);
		printf("  key_and_data_size:  %u\n", key_and_data_size);
		printf("  filename_len:       %u\n", filename_len);
		printf("  filename:           %s\n", filename);
		for (uint32_t i = 0; i < filename_len; i++) {
			printf("%02x ", filename[i]);
		}
		printf("  data_to_write_size: %u\n", data_to_write_size);
		printf("  data_to_write (hex):\n");
		for (uint32_t i = 0; i < data_to_write_size; i++) {
			printf("%02x ", data_to_write[i]);
			if ((i + 1) % 32 == 0)
				printf("\n");
		}
		if (data_to_write_size % 32 != 0)
			printf("\n");
#endif
		ret = optee_write_keybox((uint8_t*)filename,
			filename_len,
			data_to_write,
			data_to_write_size);
		if (ret == TEE_SUCCESS) {
			rc = 0;
			printf("write common data to secure storage success, filename: %s, size: %u\n", filename, data_to_write_size);
		} else {
			rc = -EIO;
			printf("write common data to secure storage fail\n");
		}
	}

	/* write all data to secure storage for readback check */
	if (!rc) {
		uint32_t ret;
		uint8_t *raw_data = malloc(len + sizeof(uint32_t));

		/* add raw_data_len(4 byte) in begin of raw_data */
		memcpy(raw_data, &len, sizeof(uint32_t));
		memcpy((raw_data + sizeof(uint32_t)), received_data, len);

		ret = optee_write_keybox((uint8_t *)"raw_data", sizeof("raw_data"),
					 raw_data, len + sizeof(uint32_t));
		if (ret == TEE_SUCCESS)
			rc = 0;
		else
			rc = -EIO;
		free(raw_data);
	}
	return rc;
}

uint32_t read_raw_data_from_secure_storage(uint8_t *data, uint32_t data_size)
{
	uint32_t rc;
	uint32_t key_size;
	uint8_t *read_data = malloc(1024 * 40);

	rc = optee_read_keybox((uint8_t *)"raw_data", sizeof("raw_data"),
				read_data, data_size);
	if (rc != TEE_SUCCESS)
		return 0;

	memcpy(&key_size, read_data, sizeof(uint32_t));
	memcpy(data, read_data + sizeof(uint32_t), key_size);
	rc = key_size;
	free(read_data);

	return rc;
}

char *new_strstr(const char *s1, const char *s2, uint32_t l1)
{
	uint32_t l2;

	l2 = strlen(s2);
	if (!l2)
		return (char *)s1;
	while (l1 >= l2) {
		l1--;
		if (!memcmp(s1, s2, l2))
			return (char *)s1;
		s1++;
	}
	return NULL;
}
