// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd
 */

#include <common.h>
#include <malloc.h>
#include <string.h>
#include <tee.h>
#include <tee/optee.h>
#include <youtube_keybox.h>
#include <linux/errno.h>
#include <uboot_aes.h>

int parse_ytsk_header(uint8_t *data, ytsk_header_t *header)
{
	if (!data || !header)
		return -EINVAL;

	memcpy(header, data, sizeof(ytsk_header_t));

	if (memcmp(header->tag, YTSK_TAG, 4) != 0) {
		printf("Error: Invalid YTSK tag\n");
		return -EINVAL;
	}

	if (header->version != YTSK_VERSION) {
		printf("Error: Unsupported YTSK version 0x%04x\n", header->version);
		return -EINVAL;
	}

	printf("YTSK Header parsed: tag=%.4s, version=0x%04x, header_size=%d, keybox_size=%d\n",
	       header->tag, header->version, header->header_size, header->keybox_size);

	return 0;
}

int decrypt_youtube_keybox(uint8_t *aes_key, uint8_t *encrypted, uint8_t *decrypted)
{
	uint8_t iv[AES_BLOCK_SIZE] = YTSK_CBC_IV;
	uint8_t aes_exp_key[256];
	uint32_t key_size_bits = 128;
	int num_aes_blocks = sizeof(youtube_keybox_t) / AES_BLOCK_SIZE;
	youtube_keybox_t *keybox;

	if (!aes_key || !encrypted || !decrypted)
		return -EINVAL;

	if (sizeof(youtube_keybox_t) % AES_BLOCK_SIZE != 0) {
		printf("Error: Keybox size not aligned to AES block size\n");
		return -EINVAL;
	}

	aes_expand_key(aes_key, key_size_bits, aes_exp_key);

	aes_cbc_decrypt_blocks(key_size_bits, aes_exp_key, iv, encrypted, decrypted, num_aes_blocks);

	keybox = (youtube_keybox_t *)decrypted;

	if (memcmp(keybox->magic, YTSK_MAGIC, 4) != 0) {
		printf("Error: Decryption failed - invalid magic: %.4s (expected: %.4s)\n",
		       keybox->magic, YTSK_MAGIC);
		return -EIO;
	}

	printf("YouTube keybox decrypted successfully.\n");

	return 0;
}

int write_youtube_keybox_to_secure_storage(uint8_t *received_data, uint32_t len)
{
	ytsk_header_t header;
	uint8_t *encrypted_keybox;
	youtube_keybox_t *keybox;
	youtube_keybox_t verify_keybox;
	uint32_t read_size = sizeof(youtube_keybox_t);
	uint32_t ret;
	uint32_t total_size;

	if (!received_data || len < sizeof(ytsk_header_t) + sizeof(youtube_keybox_t))
		return -EINVAL;

	printf("=== write_youtube_keybox_to_secure_storage ===\n");

	ret = parse_ytsk_header(received_data, &header);
	if (ret != 0) {
		printf("Error: Failed to parse YTSK header (%d)\n", ret);
		return ret;
	}

	if (header.keybox_size != sizeof(youtube_keybox_t)) {
		printf("Error: Unexpected keybox size %d (expected %d)\n",
		       header.keybox_size, (int)sizeof(youtube_keybox_t));
		return -EINVAL;
	}

	encrypted_keybox = received_data + header.header_size;
	if (encrypted_keybox + sizeof(youtube_keybox_t) > received_data + len) {
		printf("Error: Insufficient data for keybox\n");
		return -EINVAL;
	}

	keybox = malloc(sizeof(youtube_keybox_t));
	if (!keybox) {
		printf("Error: Failed to allocate memory\n");
		return -ENOMEM;
	}

	ret = decrypt_youtube_keybox(header.aes_key, encrypted_keybox, (uint8_t *)keybox);
	if (ret != 0) {
		printf("Error: Failed to decrypt keybox (%d)\n", ret);
		free(keybox);
		return ret;
	}

	total_size = sizeof(youtube_keybox_t);
	printf("Writing decrypted keybox to secure storage (%d bytes)...\n", total_size);

	ret = optee_write_keybox((uint8_t *)"youtube_keybox",
				 sizeof("youtube_keybox"),
				 (uint8_t *)keybox,
				 total_size);
	if (ret != TEE_SUCCESS) {
		printf("Error: Failed to write youtube_keybox to secure storage (0x%x)\n", ret);
		free(keybox);
		return -EIO;
	}

	printf("Successfully wrote youtube_keybox to secure storage\n");

	/* Readback verification */
	ret = optee_read_keybox((uint8_t *)"youtube_keybox",
				sizeof("youtube_keybox"),
				(uint8_t *)&verify_keybox,
				read_size);
	if (ret == TEE_SUCCESS) {
		printf("Readback verification successful\n");
		printf("cert_scope=%.64s\n", verify_keybox.cert_scope);
	} else {
		printf("Warning: Readback verification failed (0x%x)\n", ret);
	}

	free(keybox);
	return 0;
}
