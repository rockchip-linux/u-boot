/* SPDX-License-Identifier:     GPL-2.0+ */
/*
 * (C) Copyright 2023 Rockchip Electronics Co., Ltd
 */

#ifndef _UBOOT_AEAD_H
#define _UBOOT_AEAD_H

enum AEAD_ALGO {
	AEAD_ALGO_DES,
	AEAD_ALGO_AES,
	AEAD_ALGO_SM4,

	AEAD_ALGO_NUM,

	AEAD_ALGO_INVALID = 0xffffffff,
};

enum AEAD_MODE {
	AEAD_MODE_CCM,
	AEAD_MODE_GCM,
};

/* general APIs for aead algo information */

/*
 * struct aead_ops - Driver model for Aead operations
 *
 * The uclass interface is implemented by all hash devices
 * which use driver model.
 */
struct aead_ops {
	int (*do_cipher)(struct udevice *dev, enum CIPHER_ALGO algo,
			 const void *ibuf, const uint32_t ilen,
			 const void *aad, const uint32_t aadlen,
			 void *obuf, uint32_t *olen,
			 void *tag, uint32_t *taglen);
};

#endif

