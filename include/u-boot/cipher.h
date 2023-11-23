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

	CIPHER_MODE_NUM,

	CIPHER_MODE_INVALID = 0xffffffff,

};

/* general APIs for cipher algo information */

/*
 * struct cipher_ops - Driver model for Cipher operations
 *
 * The uclass interface is implemented by all hash devices
 * which use driver model.
 */
struct cipher_ops {
	/* progressive operations */
	int (*do_cipher)(struct udevice *dev, enum CIPHER_ALGO algo,
			 const void *ibuf, const uint32_t ilen,
			 void *obuf, uint32_t *olen);
};

#endif
