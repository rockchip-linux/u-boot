// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd
 */

#ifndef _YOUTUBE_KEYBOX_H
#define _YOUTUBE_KEYBOX_H

#include <stdint.h>

#define YTSK_TAG			"YTSK"
#define YTSK_MAGIC			"ytub"
#define YTSK_VERSION			0x0001
#define AES_BLOCK_SIZE			16

/* CBC IV for YouTube keybox encryption/decryption */
#define YTSK_CBC_IV			{0x22, 0xA3, 0x89, 0x6A, 0xBF, 0xC7, 0x14, 0xE4, \
					 0xC6, 0x8D, 0x04, 0xA0, 0xA8, 0xEF, 0x6C, 0xC6}

/* YTSK Header - 76 bytes */
typedef struct {
	uint8_t	tag[4];			/* "YTSK" */
	uint16_t version;		/* Format version (0x0001) */
	uint16_t header_size;		/* Total header size */
	uint8_t create_time[8];		/* Timestamp */
	uint16_t keybox_size;		/* Per keybox size (176) */
	uint8_t reserved[6];		/* Reserved for future use */
	uint8_t key_data_sha256[32];	/* SHA256 of all keybox data */
	uint8_t aes_key[16];		/* AES encryption key */
	uint32_t header_crc;		/* Header CRC32 */
} __attribute__((packed)) ytsk_header_t;

/* YouTube Keybox (decrypted/storage format) - 176 bytes
 * This is plain text format after decryption from tool,
 * also format stored in secure storage
 */
typedef struct {
	uint8_t cert_scope[64];		/* Certification scope string */
	uint8_t primary_key[32];		/* Primary key */
	uint8_t backup_key1[32];		/* Backup key 1 */
	uint8_t backup_key2[32];		/* Backup key 2 */
	uint8_t current_key_index;	/* Current key index: 0=primary, 1=backup1, 2=backup2 */
	uint8_t revoked_keys;		/* Bit flags: bit0=primary, bit1=backup1, bit2=backup2 (1=revoked) */
	uint8_t reserved[6];		/* Reserved for future use */
	uint8_t magic[4];			/* "ytub" */
	uint32_t crc;				/* CRC32 of this keybox */
} __attribute__((packed)) youtube_keybox_t;

/* Function declarations */
int parse_ytsk_header(uint8_t *data, ytsk_header_t *header);
int decrypt_youtube_keybox(uint8_t *aes_key, uint8_t *encrypted, uint8_t *decrypted);
int write_youtube_keybox_to_secure_storage(uint8_t *received_data, uint32_t len);

#endif /* _YOUTUBE_KEYBOX_H */
