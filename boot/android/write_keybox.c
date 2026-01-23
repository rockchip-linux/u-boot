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

#define	SIZE_OF_TAG		4
#define	BOOT_FROM_EMMC	(1 << 1)
#define	WIDEVINE_TAG	"KBOX"
#define	ATTESTATION_TAG	"ATTE"
#define	ID_ATTESTATION_TAG "IDAT"
#define PLAYREADY30_TAG	"SL30"

uint32_t write_keybox_to_secure_storage(uint8_t *received_data, uint32_t len)
{
	uint8_t *widevine_data;
	uint8_t *attestation_data;
	uint8_t *id_attestation_data;
	uint8_t *playready_sl30_data;
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
