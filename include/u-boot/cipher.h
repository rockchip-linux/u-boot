/* SPDX-License-Identifier:     GPL-2.0+ */
/*
 * (C) Copyright 2023 Rockchip Electronics Co., Ltd
 */

#ifndef _UBOOT_CIPHER_H
#define _UBOOT_CIPHER_H

enum CIPHER_ALGO {
	CIPHER_ALGO_DES,
	CIPHER_ALGO_AES,
	CIPHER_ALGO_SM4,

	CIPHER_ALGO_NUM,

	CIPHER_ALGO_INVALID = 0xffffffff,
};

enum CIPHER_MODE {
	CIPHER_MODE_ECB,
	CIPHER_MODE_CBC,
	CIPHER_MODE_CTS,
	CIPHER_MODE_CTR,
	CIPHER_MODE_CFB,
	CIPHER_MODE_OFB,
	CIPHER_MODE_XTS,
	CIPHER_MODE_CCM,
	CIPHER_MODE_GCM,
	CIPHER_MODE_CMAC,
	CIPHER_MODE_CBC_MAC,

	CIPHER_MODE_NUM,

	CIPHER_MODE_INVALID = 0xffffffff,

};

/* general APIs for cipher algo information */

typedef struct {
	enum CIPHER_ALGO		algo;
	enum CIPHER_MODE		mode;
	const u8	*key;
	const u8	*twk_key;
	u32		key_len;
	const u8	*iv;
	u32		iv_len;
} cipher_context;

/*
 * struct cipher_ops - Driver model for Cipher operations
 *
 * The uclass interface is implemented by all hash devices
 * which use driver model.
 */
struct cipher_ops {
	/* cipher encryption and decryption */
	int (*cipher_crypt)(struct udevice *dev, cipher_context *ctx,
			    const u8 *in, u8 *out, u32 len, bool enc);

	/* cipher mac cmac&cbc_mac */
	int (*cipher_mac)(struct udevice *dev, cipher_context *ctx,
			  const u8 *in, u32 len, u8 *tag);

	/* cipher aes ccm&gcm */
	int (*cipher_ae)(struct udevice *dev, cipher_context *ctx,
			 const u8 *in, u32 len, const u8 *aad, u32 aad_len,
			 u8 *out, u8 *tag);
};

/**
 * crypto_cipher() - Crypto cipher crypt
 *
 * @dev: crypto device
 * @ctx: cipher context
 * @in: input data buffer
 * @out: output data buffer
 * @len: input data length
 * @enc: true for encrypt, false for decrypt
 * @return 0 on success, otherwise failed
 */
int crypto_cipher(struct udevice *dev, cipher_context *ctx,
		  const u8 *in, u8 *out, u32 len, bool enc);

/**
 * crypto_mac() - Crypto cipher mac
 *
 * @dev: crypto device
 * @ctx: cipher context
 * @in: input data buffer
 * @len: input data length
 * @tag: output data buffer
 * @return 0 on success, otherwise failed
 */
int crypto_mac(struct udevice *dev, cipher_context *ctx,
	       const u8 *in, u32 len, u8 *tag);

/**
 * crypto_ae() - Crypto cipher authorization and encryption
 *
 * @dev: crypto device
 * @ctx: cipher context
 * @in: input data buffer
 * @len: input data length
 * @aad: associated data buffer
 * @aad_len: associated data length
 * @out: output data buffer
 * @tag: tag buffer
 * @return 0 on success, otherwise failed
 */
int crypto_ae(struct udevice *dev, cipher_context *ctx,
	      const u8 *in, u32 len, const u8 *aad, u32 aad_len,
	      u8 *out, u8 *tag);

const char *cipher_algo_name(enum CIPHER_ALGO algo);

const char *cipher_mode_name(enum CIPHER_MODE mode);

#endif
